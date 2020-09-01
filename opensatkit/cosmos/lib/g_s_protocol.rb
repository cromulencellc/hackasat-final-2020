require 'cosmos/interfaces/protocols/protocol'

module Cosmos
 
  class GSProtocol < Protocol
    BRIDGEHEADER = "L3:"
    FRAMESYNC = "\xDE\xAD\xBE\xEF"
    RADIOHEADER = "RSC:"

    BRIDGEHEADER.force_encoding('ASCII-8BIT')
    FRAMESYNC.force_encoding('ASCII-8BIT')

    def initialize(allow_empty_data = nil)
        super(allow_empty_data)
    end

    def write_data(data)
      if (data[0..3] == RADIOHEADER)
        data = super(data)
        return data
      end
      # Need to ensure we don't overflow the bridge input buffer
      # May want to do somthing better where we model the rate
      if (data.length >= 256)
        sleep(1)
      end
      # Add header
      data = super(data)
      data = BRIDGEHEADER + FRAMESYNC + data
      return(data)
    end #end write_data
 
    def read_data(data)
      # Remove the header
      header = BRIDGEHEADER + FRAMESYNC
      if ((data[0..6] == header))
        return super(data[7..-1])
      else
        # Header not found discard the packet this should never happen
        return :STOP
        # return super(data)
      end
    end #end read_data
	   
  end # class
 
end # module
