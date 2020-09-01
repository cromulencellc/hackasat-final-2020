# encoding: ascii-8bit

require 'cosmos/conversions/conversion'

module Cosmos

  class BitOnOffConversion < Conversion

    def initialize(bit_mask)
      super()
      @bit_mask = Integer(bit_mask)
      @converted_type = :STRING
      @converted_bit_size = 24
    end

    def call(value, packet, buffer)
      switch_status = packet.read('SWITCH_STATUS', :RAW, buffer)
      if (switch_status & @bit_mask) != 0
        'ON'
      else
        'OFF'
      end
    end

    def to_config(read_or_write)
      "    #{read_or_write}_CONVERSION #{self.class.name.class_name_to_filename} #{sprintf("0x%08X", @bit_mask)}\n"
    end

  end # class

end # module Cosmos
