module LaunchSIP
    # open our database
    raise "Wrong database type" if SETTINGS["database_type"] != "postgresql"
    DB = Sequel.postgres(
        SETTINGS["database_path"],
        host: SETTINGS["database_host"],
        user: SETTINGS["database_user"],
        password: SETTINGS["database_password"]
    )

    # create table for user accounts
    DB.create_table?("accounts") do
        primary_key :id
        varchar :name, { :size => 32, :unique => true, :null => false }
        varchar :password, { :size => 64 }
        timestamp :created_at, { :default => Sequel::CURRENT_TIMESTAMP, :null => false }
    end

    # create table for notes
    DB.create_table?("notes") do
        primary_key :id
        integer :user, { :null => false }
        varchar :title
        varchar :data
        bool :public
        timestamp :created_at, { :default => Sequel::CURRENT_TIMESTAMP, :null => false }
    end

    # create table for images
    DB.create_table?("images") do
        primary_key :id
        varchar :path, { :null => false }
        timestamp :created_at, { :default => Sequel::CURRENT_TIMESTAMP, :null => false }
    end

    # create table for satellites
    DB.create_table?("satellites") do
        primary_key :id
        varchar :name
        varchar :source_ip
        varchar :notes
        timestamp :last_seen, { :null => false }
        timestamp :created_at, { :default => Sequel::CURRENT_TIMESTAMP, :null => false }
    end

    # create table for EYASSAT_IF HK_TLM_PKT in eyessat_if_tlm.txt
    DB.create_table?("telemetry_app") do
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
    DB.create_table?("telemetry_internal") do
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
    DB.create_table?("telemetry_adcs") do
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
    DB.create_table?("telemetry_temperature") do
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
        #timestamp :updated_at, { :null => false }
        varchar :source_ip, { :null => false }
    end

    # create table for EYASSAT_IF POWER in eyessat_if_tlm.txt
    DB.create_table?("telemetry_power") do
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
    DB.create_table?("telemetry_payload") do
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
    DB.create_table?("telemetry_ci") do
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
    DB.create_table?("telemetry_ephemeris_app") do
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
    DB.create_table?("telemetry_ephemeris") do
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

    # auto-generate classes from the model
    class Account < Sequel::Model; end
    class Note < Sequel::Model; end
    class Image < Sequel::Model; end
    class Satellite < Sequel::Model; end
    class TelemetryApp < Sequel::Model(:telemetry_app); end
    class TelemetryInternal < Sequel::Model(:telemetry_internal); end
    class TelemetryADCS < Sequel::Model(:telemetry_adcs); end
    class TelemetryTemperature < Sequel::Model(:telemetry_temperature); end
    class TelemetryPower < Sequel::Model(:telemetry_power); end
    class TelemetryPayload < Sequel::Model(:telemetry_payload); end
    class TelemetryCI < Sequel::Model(:telemetry_ci); end
    class TelemetryEphemerisApp < Sequel::Model(:telemetry_ephemeris_app); end
    class TelemetryEphemeris < Sequel::Model(:telemetry_ephemeris); end
end
