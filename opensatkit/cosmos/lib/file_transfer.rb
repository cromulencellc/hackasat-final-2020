###############################################################################
# File Transfer
#
# Notes:
#   1. Abstract file transfer services so different protocols can be used
#      Currently hard coded for TFTP.
#
# License:
#   Written by David McComas, licensed under the copyleft GNU General Public
#   License (GPL).
#
###############################################################################

require 'osk_global'
require 'tftp'

module Osk

   class FileTransfer

      def initialize
      end
  
      def get(flt_filename, gnd_filename)
         raise NotImplementedError
      end # get()

      def put (gnd_filename, flt_filename)
         raise NotImplementedError
      end # put()
 
   end # Class FileTransfer
   
   class TftpFileTransfer < FileTransfer

      attr_reader :tftp
  
      def initialize(ip_addr = Osk::COSMOS_IP_ADDR)
         @tftp = TFTP.new(ip_addr)
      end
  
      def get(flt_filename, gnd_filename, tlm_timeout = Osk::TFTP_GET_TIMEOUT)
  
         got_file = true
         # TFTP uses UDP directly without cmd interface so can't use cmd counters to verify execution
         get_file_cnt = tlm("TFTP HK_TLM_PKT GET_FILE_COUNT")
         seq_cnt = tlm("TFTP HK_TLM_PKT CCSDS_SEQUENCE")
         @tftp.getbinaryfile(flt_filename, gnd_filename)
         wait("TFTP HK_TLM_PKT GET_FILE_COUNT == #{get_file_cnt}+1", tlm_timeout)  # Delay until get file count increments or timeout
         if (tlm("TFTP HK_TLM_PKT CCSDS_SEQUENCE") == seq_cnt)
            prompt ("No telemetry received to verify the error. Verify connection and telemetry output filter table.");
            got_file = false  
         end
      
         return got_file 
    
      end # get()

      def put (gnd_filename, flt_filename, tlm_timeout = Osk::TFTP_PUT_TIMEOUT)
  
         put_file = true
         # TFTP uses UDP directly without cmd interface so can't use cmd counters to verify execution
         put_file_cnt = tlm("TFTP HK_TLM_PKT PUT_FILE_COUNT")
         seq_cnt = tlm("TFTP HK_TLM_PKT CCSDS_SEQUENCE")
         @tftp.putbinaryfile(gnd_filename, flt_filename)
         wait("TFTP HK_TLM_PKT PUT_FILE_COUNT == #{put_file_cnt}+1", tlm_timeout)  # Delay until put file count increments or timeout
         if (tlm("TFTP HK_TLM_PKT CCSDS_SEQUENCE") == seq_cnt)
            prompt ("No telemetry received to verify the error. Verify connection and telemetry output filter table.");
            put_file = false  
         end
      
         return put_file 
    
      end # put()

   end # Class TftpFileTransfer

   class CFDPFileTransfer < FileTransfer

      attr_reader :cfdp
  
  
      def get(flt_filename, gnd_filename, tlm_timeout = Osk::CFDP_GET_TIMEOUT)
  
         got_file = true
         get_file_cnt = tlm("CF HK_TLM_PKT TOTAL_SENT")
         seq_cnt = tlm("CF HK_TLM_PKT CCSDS_SEQUENCE")
         cmd("CF","PLAYBACK_FILE","SRC_FILENAME" => flt_filename, "DEST_FILENAME" => gnd_filename)
         wait("CF HK_TLM_PKT TOTAL_SENT == #{get_file_cnt}+1", tlm_timeout)  # Delay until put file count increments or timeout
         if (tlm("CF HK_TLM_PKT CCSDS_SEQUENCE") == seq_cnt)
            prompt ("No telemetry received to verify the error. Verify connection and telemetry output filter table.");
            got_file = false  
         end
      
         return got_file 
    
      end # get()

      def put(gnd_filename, flt_filename, tlm_timeout = Osk::CFDP_PUT_TIMEOUT)
         put_file = true
         put_file_cnt = tlm("CF HK_TLM_PKT TOTAL_RECEIVED")
         seq_cnt = tlm("CF HK_TLM_PKT CCSDS_SEQUENCE")
         cmd("CFDP","SEND_FILE","SRCFILENAME" => gnd_filename,"DSTFILENAME" => flt_filename)
         wait("CF HK_TLM_PKT TOTAL_RECEIVED == #{put_file_cnt}+1", tlm_timeout)  # Delay until put file count increments or timeout
         if (tlm("CF HK_TLM_PKT CCSDS_SEQUENCE") == seq_cnt)
            prompt ("No telemetry received to verify the error. Verify connection and telemetry output filter table.");
            put_file = false  
         end
         return put_file
      end # put()

   end # Class CFDPFileTransfer

end # Module Osk


  
