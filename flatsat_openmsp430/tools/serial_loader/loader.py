#!/usr/bin/python

import serial, time
import binascii
import struct

def read_data_file( filename ):
	file_data = ""
	with open( filename, "rb" ) as f:
		byte = f.read(1)
		while byte != b"":
			file_data += byte
			byte = f.read(1)

	return file_data

#initialization and open the port
#possible timeout values:
#    1. None: wait forever, block call
#    2. 0: non-blocking mode, return immediately
#    3. x, x is bigger than 0, float allowed, timeout block call

ser = serial.Serial()
ser.port = "/dev/ttyUSB2"
ser.baudrate = 115200
ser.bytesize = serial.EIGHTBITS #number of bits per bytes
ser.parity = serial.PARITY_NONE #set parity check: no parity
ser.stopbits = serial.STOPBITS_ONE #number of stop bits

#ser.timeout = None          #block read

ser.timeout = 0             #non-block read

#ser.timeout = 2              #timeout block read

ser.xonxoff = False     #disable software flow control
ser.rtscts = False     #disable hardware (RTS/CTS) flow control
ser.dsrdtr = False       #disable hardware (DSR/DTR) flow control

ser.writeTimeout = 2     #timeout for write

try: 

    ser.open()

except Exception, e:

    print "error open serial port: " + str(e)

    exit()

def ser_writesafe_line( ser, data ):
	pos = 0

	while pos < len(data):
		ser.write(data[pos])
		ser.flush()

		while ser.read(1) != '>':
			pass
	
		pos = pos + 1

	ser.write( '\n' )
	ser.flush()

	while ser.read(1) != '\n':
		pass


if ser.isOpen():

    file_data = read_data_file( "data.mem" )

    try:
        ser.flushInput() #flush input buffer, discarding all its contents
        ser.flushOutput()#flush output buffer, aborting current output 

                     #and discard all that is in buffer

	print "Sending program.\n"
	out = ""
	while ser.inWaiting() > 0:
            out += ser.read(1)
	print "Out: %s\n" % out

        #write data
        ser.write("PROG\n")
	ser.flush()
        time.sleep(0.5)  #give the serial port sometime to receive the data

	# Wait for ready
	out = ""
	while ser.inWaiting() > 0:
		out += ser.read(1)

	if ( out.find('READY') == -1 ):
		print "[ERROR]Out is %s\n" % out
		ser.close()
		exit(1)
	else:
		print "Got READY, out=%s\n" % out

	file_lines = (line.rstrip('\n') for line in file_data.splitlines())

    	file_checksum = 0
	count = 0
	for line in file_lines:
		if ( len(line) < 4 ):
			continue
	
		ser_writesafe_line( ser, line.lstrip() )

		'''	
		while ser.read(1) != '>':
			ser.write( line.lstrip()[pos] )
			pos += 1

		ser.write( '\n' )
		ser.flush()
			
		# Do nothing
		while ser.read(1) != '\n':
			pass
		'''

		value =  struct.unpack( '>H', binascii.a2b_hex( line.lstrip() ) )[0] 
		file_checksum = file_checksum + value	
		print "Line %s  value: %04x\n" % (line.lstrip(), value)
		
		count = count + 1

	print "Sent %d lines.\n" % count
	print "File checksum: 0x%08x\n" % (file_checksum & 0xffffffff)

	# SEND CRC
	checksum_bin_data = struct.pack( '>I', file_checksum )
	
	ser_writesafe_line( ser, binascii.b2a_hex( checksum_bin_data ) )
	#ser.write( binascii.b2a_hex( checksum_bin_data ) + '\n' )
	#ser.flush()

	time.sleep(0.5)

	out = ""
	while ser.inWaiting() > 0:
		out += ser.read(1)

	print "Out: %s\n" % out

        ser.close()

    except Exception, e1:
        print "error communicating...: " + str(e1)

else:
    print "cannot open serial port "
