require "cosmos"
require "cosmos/interfaces/tcpip_client_interface"
require "cosmos/tools/cmd_tlm_server/background_task"
require "socket"
require "sequel"
require "pg"

# this background task connects to a pre-defined router within cosmos that has forwarded telemetry messages
# it also connects to a pre-defined database where those telemetry messages should be stored
# a failed router connection is fatal (because cosmos is likely screwed up at this point)
# a failed database connection triggers a retry (because the network could be wonky, but otherwise okay)

#$telemetry_db_url = "postgres://telemeter:BolbophogDathaygathocIjAgBofEttefsOulHem@bk-has-db.mlb.cromulence.com/space_registrar_production"
$telemetry_db_url = "postgres://telemeter:password@localhost/telemetry_testing"

module Cosmos
    class TelemetryDb < BackgroundTask
        def call
            loop do
                begin
                    # connect to our telemetry router
                    self.tlm_connect
                    Logger.instance.info("Connected to telemetry router")

                    # connect to our database
                    self.db_connect
                    Logger.instance.info("Connected to telemetry database")

                    # read and process packets of telemetry data
                    loop do
                        pkt = @tlm.read
                        case pkt.target_name
                        when "EYASSAT_IF"
                            case pkt.packet_name
                            when "HK_TLM_PKT"
                                self.db_insert_telemetry_app(pkt)
                            when "INTERNAL"
                                self.db_insert_telemetry_internal(pkt)
                            when "ADCS"
                                self.db_insert_telemetry_adcs(pkt)
                            when "TEMPERATURE"
                                self.db_insert_telemetry_temperature(pkt)
                            when "POWER"
                                self.db_insert_telemetry_power(pkt)
                            end
                        when "PL_IF"
                            self.db_insert_telemetry_payload(pkt) if pkt.packet_name == "HK_TLM_PKT"
                        when "UART_TO_CI"
                            self.db_insert_telemetry_ci(pkt) if pkt.packet_name == "HK_TLM_PKT"
                        when "EPHEM"
                            case pkt.packet_name
                            when "HK_TLM_PKT"
                                self.db_insert_telemetry_ephemeris_app(pkt)
                            when "EPHEM_PKT"
                                self.db_insert_telemetry_ephemeris(pkt)
                            end
                        end
                    end

                    # clean up and disconnect
                    self.db_disconnect
                    self.tlm_disconnect
                rescue Errno::ECONNREFUSED
                    # connection to router was lost, retry
                    Logger.instance.error("Connection to router was lost, giving up")
                    return
                rescue Sequel::DatabaseConnectionError
                    # connection to database could not be established, retry
                    Logger.instance.error("Connection to database could not be established, retrying...")
                    sleep(5)
                    next
                rescue PG::UnableToSend
                    # connection to database was lost, retry
                    Logger.instance.error("Connection to database was lost, retrying...")
                    sleep(5)
                    next
                rescue => e
                    # some other error occurred, retry
                    Loger.instance.error("Failed while reading telemetry with exception class #{e.class}")
                    File.open("/tmp/fail", "a") do |f|
                        f.write("#{e.class}\n")
                        f.write("#{e.message}\n")
                        f.write("#{e.backtrace}\n")
                    end
                    sleep(5)
                    next
                end
            end
        end

        def read_int(pkt, name)
            # read the field from the packet
            val = pkt.read(name)

            # handle all the weird state values cosmos can return from reading an integer
            # (you should be able to do "pkt.read(name, :RAW) i think? but, it doesn't seem to work...)
            if val == "ON"
                return 1
            elsif val == "OFF"
                return 0
            elsif val == "TRUE"
                return 1
            elsif val == "FALSE"
                return 0
            elsif val == "POS"
                return 1
            elsif val == "NEG"
                return -1
            else
                return val
            end
        end

        def read_float(pkt, name)
            # read the field from the packet
            val = pkt.read(name)

            # handle NaNs before they reach the database
            if not val.nan?
                return val
            else
                return 0.0
            end
        end

        def read_string(pkt, name)
            # read the field from the packet
            val = pkt.read(name)

            # convert to ASCII because we often get bad unicode and I dunno what else to do with it
            return val.encode("ASCII", "UTF-8", invalid: :replace, undef: :replace)
        end

        def tlm_connect
            if @tlm == nil or not @tlm.read_allowed?
                @tlm = Cosmos::TcpipClientInterface.new("localhost", 2055, 2055, 10.0, nil, "PREIDENTIFIED")
                @tlm.connect
            end
        end

        def tlm_disconnect
            @tlm.disconnect
        end

        def db_connect
            @db = Sequel.connect($telemetry_db_url)  # see top of file for definition
            self.db_init
        end

        def db_disconnect
            @db.disconnect
        end

        def db_init
			# create table for EYASSAT_IF HK_TLM_PKT in eyessat_if_tlm.txt
			@db.create_table?("telemetry_app") do
                primary_key :id
		        integer :packet_identifier
		        integer :packet_sequence_counter
		        integer :tlm_seconds
		        integer :tlm_subseconds
		        integer :command_valid_count
		        integer :command_error_count
		        integer :last_table_action
		        integer :last_table_status
		        integer :example_object_exec_count
		        integer :command_buffer_head
		        integer :command_buffer_tail
		        integer :padding
		        timestamp :created_at, { :default => Sequel::CURRENT_TIMESTAMP, :null => false }
                timestamp :updated_at, { :null => false }
                varchar :source_ip, { :null => false }
		    end

			# create table for EYASSAT_IF INTERNAL in eyessat_if_tlm.txt
			@db.create_table?("telemetry_internal") do
                primary_key :id
		        integer :packet_identifier
		        integer :packet_sequence_counter
		        integer :tlm_seconds
		        integer :tlm_subseconds
		        varchar :time_string
		        varchar :simulator_call_sign
		        integer :packet_id
		        integer :delay_in_seconds
		        integer :power_board_telemetry
		        integer :adcs_board_telemetry
		        integer :experiment_board_telemetry
		        integer :command_timeout
		        integer :padding
		        timestamp :created_at, { :default => Sequel::CURRENT_TIMESTAMP, :null => false }
                timestamp :updated_at, { :null => false }
                varchar :source_ip, { :null => false }
		    end

			# create table for EYASSAT_IF ADCS in eyessat_if_tlm.txt
			@db.create_table?("telemetry_adcs") do
                primary_key :id
		        integer :packet_identifier
		        integer :packet_sequence_counter
		        integer :tlm_seconds
		        integer :tlm_subseconds
		        varchar :time_string
		        varchar :sim_call_sign
		        integer :packet_id
		        integer :torque_x
		        integer :torque_y
		        integer :control_mode
		        integer :pad_bits
		        integer :top_sun_sensor
		        integer :bottom_sun_sensor
		        integer :pwm
		        float :yaw_angle_sun
		        float :yaw_angle_magnetometer
		        float :pitch_angle_magnetometer
		        float :roll_angle_magnetometer
		        float :magnetometer_x
		        float :magnetometer_y
		        float :magnetometer_z
		        float :acceleterometer_x
		        float :acceleterometer_y
		        float :acceleterometer_z
		        float :angular_rotation_x
		        float :angular_rotation_y
		        float :angular_rotation_z
		        float :actual_wheel_speed
		        float :commanded_wheel_speed
		        float :wheel_angular_momentum
		        float :integration_time
		        float :commanded_yaw_angle
		        float :pointing_error
		        float :deadband
		        float :extra_pwm
		        float :p_coefficient
		        float :i_coefficient
		        float :p_coefficient2
		        float :scaling_factor
		        timestamp :created_at, { :default => Sequel::CURRENT_TIMESTAMP, :null => false }
                timestamp :updated_at, { :null => false }
                varchar :source_ip, { :null => false }
		    end

			# create table for EYASSAT_IF TEMPS in eyessat_if_tlm.txt
			@db.create_table?("telemetry_temperature") do
                primary_key :id
		        integer :packet_identifier
		        integer :packet_sequence_counter
		        integer :tlm_seconds
		        integer :tlm_subseconds
		        varchar :time_string
		        varchar :simulator_call_sign
		        integer :packet_id
		        float :dh_board_temp
		        float :experiment_temp
		        float :reference_temp
		        float :thermal_panel_a_temp
		        float :thermal_panel_b_temp
		        float :base_rods_temp
		        float :top_rods_temp
		        float :top_pipe_temp
		        timestamp :created_at, { :default => Sequel::CURRENT_TIMESTAMP, :null => false }
                timestamp :updated_at, { :null => false }
                varchar :source_ip, { :null => false }
		    end

			# create table for EYASSAT_IF POWER in eyessat_if_tlm.txt
			@db.create_table?("telemetry_power") do
                primary_key :id
		        integer :packet_identifier
		        integer :packet_sequence_counter
		        integer :tlm_seconds
		        integer :tlm_subseconds
		        varchar :time_string
		        varchar :simulator_call_sign
		        integer :packet_id
		        integer :separation_status
		        integer :pad
		        integer :switch_status_register
		        float :battery_voltage
		        float :battery_current
		        float :solar_array_voltage
		        float :solar_array_current
		        float :main_bus_current
		        float :bus_voltage_5v
		        float :bus_current_5v
		        float :bus_voltage_3p3v
		        float :bus_current_3p3v
		        float :battery_temperature
		        float :solar_array_1_temperature
		        float :solar_array_2_temperature
		        integer :power_on_3p3v
		        integer :adcs_power
		        integer :experiment_power
		        integer :heater_1_power
		        integer :heater_2_power
		        timestamp :created_at, { :default => Sequel::CURRENT_TIMESTAMP, :null => false }
                timestamp :updated_at, { :null => false }
                varchar :source_ip, { :null => false }
		    end

			# create table for PL_IF HK_TLM_PKT in pl_if_tlm.txt
			@db.create_table?("telemetry_payload") do
                primary_key :id
		        integer :packet_identifier
		        integer :packet_sequence_counter
		        integer :tlm_seconds
		        integer :tlm_subseconds
		        integer :command_valid_count
		        integer :command_error_count
		        integer :last_table_action
		        integer :last_table_status
		        integer :payload_object_exec_count
		        timestamp :created_at, { :default => Sequel::CURRENT_TIMESTAMP, :null => false }
                timestamp :updated_at, { :null => false }
                varchar :source_ip, { :null => false }
		    end

			# create table for UART_TO_CLI HK_TLM_PKT in uart_to_ci_tlm.txt
			@db.create_table?("telemetry_ci") do
                primary_key :id
		        integer :packet_identifier
		        integer :packet_sequence_counter
		        integer :tlm_seconds
		        integer :tlm_subseconds
		        integer :command_count
		        integer :error_count
		        integer :last_load_status
		        integer :spare_byte
		        integer :last_load_attr_errors
		        integer :recv_command_count
		        integer :recv_command_error_count
		        integer :baud_rate
		        timestamp :created_at, { :default => Sequel::CURRENT_TIMESTAMP, :null => false }
                timestamp :updated_at, { :null => false }
                varchar :source_ip, { :null => false }
		    end

			# create table for EPHEM HK_TLM_PKT in ephem_tlm.txt
			@db.create_table?("telemetry_ephemeris_app") do
                primary_key :id
		        integer :packet_identifier
		        integer :packet_sequence_counter
		        integer :tlm_seconds
		        integer :tlm_subseconds
		        integer :command_valid_count
		        integer :command_error_count
		        integer :last_table_action
		        integer :last_table_status
		        integer :example_object_exec_count
		        timestamp :created_at, { :default => Sequel::CURRENT_TIMESTAMP, :null => false }
                #timestamp :updated_at, { :null => false }
                varchar :source_ip, { :null => false }
		    end

			# create table for EPHEM EPHEM_PKT in ephem_tlm.txt
			@db.create_table?("telemetry_ephemeris") do
                primary_key :id
				integer :packet_identifier
				integer :packet_sequence_counter
				integer :tlm_subseconds
				integer :tlm_seconds
				integer :padding
				varchar :time_string
				float :absolute_time_offset
				float :absolute_time_epoch
				float :absolute_time
				float :x_position
				float :y_position
				float :z_position
				float :x_velocity
				float :y_velocity
				float :z_velocity
		        timestamp :created_at, { :default => Sequel::CURRENT_TIMESTAMP, :null => false }
                #timestamp :updated_at, { :null => false }
                varchar :source_ip, { :null => false }
			end
		end

        def db_insert_telemetry_app(pkt)
            # still need to identify the packet, despite the fact that it's marked PREIDENTIFIED
            pkt = System.telemetry.identify_and_define_packet(pkt, "HK_TLM_PKT")

            # insert new row into appropriate table
            @db[:telemetry_app].insert({
                :packet_identifier => self.read_int(pkt, "CCSDS_STREAMID"),
                :packet_sequence_counter => self.read_int(pkt, "CCSDS_SEQUENCE"),
                :tlm_seconds => self.read_int(pkt, "CCSDS_SECONDS"),
                :tlm_subseconds => self.read_int(pkt, "CCSDS_SUBSECS"),
                :command_valid_count => self.read_int(pkt, "CMD_VALID_COUNT"),
                :command_error_count => self.read_int(pkt, "CMD_ERROR_COUNT"),
                :last_table_action => self.read_int(pkt, "LAST_TBL_ACTION"),
                :last_table_status => self.read_int(pkt, "LAST_TBL_STATUS"),
                :example_object_exec_count => self.read_int(pkt, "EXOBJ_EXEC_CNT"),
                :command_buffer_head => self.read_int(pkt, "CMD_BUFFER_HEAD"),
                :command_buffer_tail => self.read_int(pkt, "CMD_BUFFER_TAIL"),
                :padding => self.read_int(pkt, "PAD"),
                :updated_at => Sequel::CURRENT_TIMESTAMP,
                :source_ip => Socket.ip_address_list.find { |ai| ai.ipv4? && !ai.ipv4_loopback? }.ip_address
            })
        end

        def db_insert_telemetry_internal(pkt)
            # still need to identify the packet, despite the fact that it's marked PREIDENTIFIED
            pkt = System.telemetry.identify_and_define_packet(pkt, "INTERNAL")

            # insert new row into appropriate table
            @db[:telemetry_internal].insert({
                :packet_identifier => self.read_int(pkt, "CCSDS_STREAMID"),
                :packet_sequence_counter => self.read_int(pkt, "CCSDS_SEQUENCE"),
                :tlm_seconds => self.read_int(pkt, "CCSDS_SECONDS"),
                :tlm_subseconds => self.read_int(pkt, "CCSDS_SUBSECS"),
                :time_string => self.read_string(pkt, "TIME_STRING"),
                :simulator_call_sign => self.read_string(pkt, "CALL_SIGN"),
                :packet_id => self.read_int(pkt, "PACKET_ID"),
                :delay_in_seconds => self.read_int(pkt, "TLM_DELAY"),
                :power_board_telemetry => self.read_int(pkt, "PWR_TLM"),
                :adcs_board_telemetry => self.read_int(pkt, "ADCS_TLM"),
                :experiment_board_telemetry => self.read_int(pkt, "EXP_TLM"),
                :command_timeout => self.read_int(pkt, "CMD_TIMEOUT"),
                :padding => self.read_int(pkt, "PAD"),
                :updated_at => Sequel::CURRENT_TIMESTAMP,
                :source_ip => Socket.ip_address_list.find { |ai| ai.ipv4? && !ai.ipv4_loopback? }.ip_address
            })
        end

        def db_insert_telemetry_adcs(pkt)
            # still need to identify the packet, despite the fact that it's marked PREIDENTIFIED
            pkt = System.telemetry.identify_and_define_packet(pkt, "ADCS")

            # insert new row into appropriate table
            @db[:telemetry_adcs].insert({
                :packet_identifier => self.read_int(pkt, "CCSDS_STREAMID"),
                :packet_sequence_counter => self.read_int(pkt, "CCSDS_SEQUENCE"),
                :tlm_seconds => self.read_int(pkt, "CCSDS_SECONDS"),
                :tlm_subseconds => self.read_int(pkt, "CCSDS_SUBSECS"),
                :time_string => self.read_string(pkt, "TIME_STRING"),
                :sim_call_sign => self.read_string(pkt, "CALL_SIGN"),
                :packet_id => self.read_int(pkt, "PACKET_ID"),
                :torque_x => self.read_int(pkt, "X_ROD"),
                :torque_y => self.read_int(pkt, "Y_ROD"),
                :control_mode => self.read_int(pkt, "CTRL_MODE"),
                :pad_bits => self.read_int(pkt, "PAD"),
                :top_sun_sensor => self.read_int(pkt, "SUN_TOP"),
                :bottom_sun_sensor => self.read_int(pkt, "SUN_BOTTOM"),
                :pwm => self.read_int(pkt, "PWM"),
                :yaw_angle_sun => self.read_float(pkt, "SUN_YAW_ANG"),
                :yaw_angle_magnetometer => self.read_float(pkt, "YAW"),
                :pitch_angle_magnetometer => self.read_float(pkt, "PITCH"),
                :roll_angle_magnetometer => self.read_float(pkt, "ROLL"),
                :magnetometer_x => self.read_float(pkt, "MAG_X"),
                :magnetometer_y => self.read_float(pkt, "MAG_Y"),
                :magnetometer_z => self.read_float(pkt, "MAG_Z"),
                :acceleterometer_x => self.read_float(pkt, "ACC_X"),
                :acceleterometer_y => self.read_float(pkt, "ACC_Y"),
                :acceleterometer_z => self.read_float(pkt, "ACC_Z"),
                :angular_rotation_x => self.read_float(pkt, "ROT_X"),
                :angular_rotation_y => self.read_float(pkt, "ROT_Y"),
                :angular_rotation_z => self.read_float(pkt, "ROT_Z"),
                :actual_wheel_speed => self.read_float(pkt, "ACT_WHEEL_SPD"),
                :commanded_wheel_speed => self.read_float(pkt, "CMD_WHEEL_SPD"),
                :wheel_angular_momentum => self.read_float(pkt, "WHEEL_ANG_MOM"),
                :integration_time => self.read_float(pkt, "DELTA_T"),
                :commanded_yaw_angle => self.read_float(pkt, "YAW_CMD"),
                :pointing_error => self.read_float(pkt, "POINTING_ERROR"),
                :deadband => self.read_float(pkt, "DEADBAND"),
                :extra_pwm => self.read_float(pkt, "EXTRA"),
                :p_coefficient => self.read_float(pkt, "KP"),
                :i_coefficient => self.read_float(pkt, "KI"),
                :p_coefficient2 => self.read_float(pkt, "KD"),
                :scaling_factor => self.read_float(pkt, "DEADBAND_SCALE_FACTOR"),
                :updated_at => Sequel::CURRENT_TIMESTAMP,
                :source_ip => Socket.ip_address_list.find { |ai| ai.ipv4? && !ai.ipv4_loopback? }.ip_address
            })
        end

        def db_insert_telemetry_temperature(pkt)
            # still need to identify the packet, despite the fact that it's marked PREIDENTIFIED
            pkt = System.telemetry.identify_and_define_packet(pkt, "TEMPERATURE")

            # insert new row into appropriate table
            @db[:telemetry_temperature].insert({
                :packet_identifier => self.read_int(pkt, "CCSDS_STREAMID"),
                :packet_sequence_counter => self.read_int(pkt, "CCSDS_SEQUENCE"),
                :tlm_seconds => self.read_int(pkt, "CCSDS_SECONDS"),
                :tlm_subseconds => self.read_int(pkt, "CCSDS_SUBSECS"),
                :time_string => self.read_string(pkt, "TIME_STRING"),
                :simulator_call_sign => self.read_string(pkt, "CALL_SIGN"),
                :packet_id => self.read_int(pkt, "PACKET_ID"),
                :dh_board_temp => self.read_float(pkt, "DH_TEMP"),
                :experiment_temp => self.read_float(pkt, "EXP_TEMP"),
                :reference_temp => self.read_float(pkt, "REF_TEMP"),
                :thermal_panel_a_temp => self.read_float(pkt, "PANEL_A_TEMP"),
                :thermal_panel_b_temp => self.read_float(pkt, "PANEL_B_TEMP"),
                :base_rods_temp => self.read_float(pkt, "BASE_TEMP"),
                :top_rods_temp => self.read_float(pkt, "TOP_A_TEMP"),
                :top_pipe_temp => self.read_float(pkt, "TOP_B_TEMP"),
                :updated_at => Sequel::CURRENT_TIMESTAMP,
                :source_ip => Socket.ip_address_list.find { |ai| ai.ipv4? && !ai.ipv4_loopback? }.ip_address
            })
        end

        def db_insert_telemetry_power(pkt)
            # still need to identify the packet, despite the fact that it's marked PREIDENTIFIED
            pkt = System.telemetry.identify_and_define_packet(pkt, "POWER")

            # insert new row into appropriate table
            @db[:telemetry_power].insert({
                :packet_identifier => self.read_int(pkt, "CCSDS_STREAMID"),
                :packet_sequence_counter => self.read_int(pkt, "CCSDS_SEQUENCE"),
                :tlm_seconds => self.read_int(pkt, "CCSDS_SECONDS"),
                :tlm_subseconds => self.read_int(pkt, "CCSDS_SUBSECS"),
                :time_string => self.read_string(pkt, "TIME_STRING"),
                :simulator_call_sign => self.read_string(pkt, "CALL_SIGN"),
                :packet_id => self.read_int(pkt, "PACKET_ID"),
                :separation_status => self.read_int(pkt, "SEP_STATUS"),
                :pad => self.read_int(pkt, "PAD"),
                :switch_status_register => self.read_int(pkt, "SWITCH_STATUS"),
                :battery_voltage => self.read_float(pkt, "V_BATT"),
                :battery_current =>self.read_float(pkt, "I_BATT"),
                :solar_array_voltage => self.read_float(pkt, "V_SA"),
                :solar_array_current => self.read_float(pkt, "I_SA"),
                :main_bus_current => self.read_float(pkt, "I_MB"),
                :bus_voltage_5v => self.read_float(pkt, "V_5V"),
                :bus_current_5v => self.read_float(pkt, "I_5V"),
                :bus_voltage_3p3v => self.read_float(pkt, "V_3V"),
                :bus_current_3p3v => self.read_float(pkt, "I_3V"),
                :battery_temperature => self.read_float(pkt, "BATT_TEMP"),
                :solar_array_1_temperature => self.read_float(pkt, "SA1_TEMP"),
                :solar_array_2_temperature => self.read_float(pkt, "SA2_TEMP"),
                :power_on_3p3v => self.read_int(pkt, "PWR_3V"),
                :adcs_power => self.read_int(pkt, "PWR_ADCS"),
                :experiment_power => self.read_int(pkt, "PWR_EXP"),
                :heater_1_power => self.read_int(pkt, "PWR_HTR1"),
                :heater_2_power => self.read_int(pkt, "PWR_HTR2"),
                :updated_at => Sequel::CURRENT_TIMESTAMP,
                :source_ip => Socket.ip_address_list.find { |ai| ai.ipv4? && !ai.ipv4_loopback? }.ip_address
            })
        end

        def db_insert_telemetry_payload(pkt)
            # still need to identify the packet, despite the fact that it's marked PREIDENTIFIED
            pkt = System.telemetry.identify_and_define_packet(pkt, "HK_TLM_PKT")

            # insert new row into appropriate table
            @db[:telemetry_payload].insert({
                :packet_identifier => self.read_int(pkt, "CCSDS_STREAMID"),
                :packet_sequence_counter => self.read_int(pkt, "CCSDS_SEQUENCE"),
                :tlm_seconds => self.read_int(pkt, "CCSDS_SECONDS"),
                :tlm_subseconds => self.read_int(pkt, "CCSDS_SUBSECS"),
                :command_valid_count => self.read_int(pkt, "CMD_VALID_COUNT"),
                :command_error_count => self.read_int(pkt, "CMD_ERROR_COUNT"),
                :last_table_action => self.read_int(pkt, "LAST_TBL_ACTION"),
                :last_table_status => self.read_int(pkt, "LAST_TBL_STATUS"),
                :payload_object_exec_count => self.read_int(pkt, "PLIF_OBJ_EXEC_CNT"),
                :updated_at => Sequel::CURRENT_TIMESTAMP,
                :source_ip => Socket.ip_address_list.find { |ai| ai.ipv4? && !ai.ipv4_loopback? }.ip_address
            })
        end

        def db_insert_telemetry_ci(pkt)
            # still need to identify the packet, despite the fact that it's marked PREIDENTIFIED
            pkt = System.telemetry.identify_and_define_packet(pkt, "HK_TLM_PKT")

            # insert new row into appropriate table
            @db[:telemetry_ci].insert({
                :packet_identifier => self.read_int(pkt, "CCSDS_STREAMID"),
                :packet_sequence_counter => self.read_int(pkt, "CCSDS_SEQUENCE"),
                :tlm_seconds => self.read_int(pkt, "CCSDS_SECONDS"),
                :tlm_subseconds => self.read_int(pkt, "CCSDS_SUBSECS"),
                :command_count => self.read_int(pkt, "CMD_VALID_COUNT"),
                :error_count => self.read_int(pkt, "CMD_ERROR_COUNT"),
                :last_load_status => self.read_int(pkt, "LAST_TBL_LOAD_STATUS"),
                :spare_byte => self.read_int(pkt, "SPARE_BYTE"),
                :last_load_attr_errors => self.read_int(pkt, "LAST_TBL_LOAD_ATTR_ERRS"),
                :recv_command_count => self.read_int(pkt, "RECV_CMD_CNT"),
                :recv_command_error_count => self.read_int(pkt, "RECV_CMD_ERR_CNT"),
                :baud_rate => self.read_int(pkt, "BAUD_RATE"),
                :updated_at => Sequel::CURRENT_TIMESTAMP,
                :source_ip => Socket.ip_address_list.find { |ai| ai.ipv4? && !ai.ipv4_loopback? }.ip_address
            })
        end

        def db_insert_telemetry_ephemeris_app(pkt)
            # still need to identify the packet, despite the fact that it's marked PREIDENTIFIED
            pkt = System.telemetry.identify_and_define_packet(pkt, "HK_TLM_PKT")

            # insert new row into appropriate table
            @db[:telemetry_ephemeris_app].insert({
                :packet_identifier => self.read_int(pkt, "CCSDS_STREAMID"),
                :packet_sequence_counter => self.read_int(pkt, "CCSDS_SEQUENCE"),
                :tlm_seconds => self.read_int(pkt, "CCSDS_SECONDS"),
                :tlm_subseconds => self.read_int(pkt, "CCSDS_SUBSECS"),
                :command_valid_count => self.read_int(pkt, "CMD_VALID_COUNT"),
                :command_error_count => self.read_int(pkt, "CMD_ERROR_COUNT"),
                :last_table_action => self.read_int(pkt, "LAST_TBL_ACTION"),
                :last_table_status => self.read_int(pkt, "LAST_TBL_STATUS"),
                :example_object_exec_count => self.read_int(pkt, "EXOBJ_EXEC_CNT"),
                #:updated_at => Sequel::CURRENT_TIMESTAMP,
                :source_ip => Socket.ip_address_list.find { |ai| ai.ipv4? && !ai.ipv4_loopback? }.ip_address
            })
        end

        def db_insert_telemetry_ephemeris(pkt)
            # still need to identify the packet, despite the fact that it's marked PREIDENTIFIED
            pkt = System.telemetry.identify_and_define_packet(pkt, "EPHEM_PKT")

            # insert new row into appropriate table
            @db[:telemetry_ephemeris].insert({
                :packet_identifier => self.read_int(pkt, "CCSDS_STREAMID"),
                :packet_sequence_counter => self.read_int(pkt, "CCSDS_SEQUENCE"),
                :tlm_seconds => self.read_int(pkt, "CCSDS_SECONDS"),
                :tlm_subseconds => self.read_int(pkt, "CCSDS_SUBSECS"),
                :padding => self.read_int(pkt, "PAD"),
                :time_string => self.read_string(pkt, "TIME_STRING"),
                :absolute_time_offset => self.read_float(pkt, "ABSOLUTE_TIME_OFFSET"),
                :absolute_time_epoch => self.read_float(pkt, "ABSOLUTE_TIME_EPOCH"),
                :absolute_time => self.read_float(pkt, "ABSOLUTE_TIME"),
                :x_position => self.read_float(pkt, "POSN_X"),
                :y_position => self.read_float(pkt, "POSN_Y"),
                :z_position => self.read_float(pkt, "POSN_Z"),
                :x_velocity => self.read_float(pkt, "VELN_X"),
                :y_velocity => self.read_float(pkt, "VELN_Y"),
                :z_velocity => self.read_float(pkt, "VELN_Z"),
                #:updated_at => Sequel::CURRENT_TIMESTAMP,
                :source_ip => Socket.ip_address_list.find { |ai| ai.ipv4? && !ai.ipv4_loopback? }.ip_address
            })
        end
    end
end

# uncomment the following two lines to run this manually (e.g. while using a replay)
#lol = Cosmos::TelemetryDb.new
#lol.call

