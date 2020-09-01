require 'cosmos/interfaces/protocols/length_protocol'

module Cosmos
 
  class GSProtocol2 < LengthProtocol
    BRIDGEHEADER = "L3:"
    # FRAMESYNC = "\xDE\xAD\xBE\xEF"

    BRIDGEHEADER.force_encoding('ASCII-8BIT')
    # FRAMESYNC.force_encoding('ASCII-8BIT')

    def initialize(length_bit_offset = 64,
                  length_bit_size = 16,
                  length_value_offset = 11,
                  length_bytes_per_count = 1,
                  length_endianness = 'BIG_ENDIAN',
                  discard_leading_bytes = 4,
                  sync_pattern = "0xDEADBEEF",
                  max_length = nil,
                  fill_length_and_sync_pattern = true,
                  allow_empty_data =nil)
      
      super(length_bit_offset,
            length_bit_size,
            length_value_offset,
            length_bytes_per_count,
            length_endianness,
            discard_leading_bytes,
            sync_pattern,
            max_length,
            fill_length_and_sync_pattern,
            allow_empty_data)
    end

    def write_data(data)
      # Add header
      data = super(data)
      return BRIDGEHEADER + data
    end #end write_data
 
    def read_data(data)
      header = BRIDGEHEADER
      if ((data[0..2] == header))
        return super(data[3..-1])
      else
        # Header not found discard the packet
        return :STOP
        # return super(data)
      end
    end #end read_data
	   
  end # class
 
end # module
