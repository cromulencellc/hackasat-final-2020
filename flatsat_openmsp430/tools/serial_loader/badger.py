#!/usr/bin/python

import serial, time
import binascii
import struct
import argparse

def read_data_file( filename ):
	file_data = ""
	with open( filename, "rb" ) as f:
		byte = f.read(1)
		while byte != b"":
			file_data += byte
			byte = f.read(1)

	return file_data

def open_serial(port):

	ser = serial.Serial()
	ser.port = port
	ser.baudrate = 115200
	ser.timeout = 0             #non-block read

	ser.writeTimeout = 2     #timeout for write

	try: 
		ser.open()
	except Exception, e:
		print "error opening serial port: " + str(e)
		exit()
	return ser

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

def flash_board(port, filename):
	ser = open_serial(port)

	if not ser.isOpen():
		exit()

	file_data = read_data_file(filename)

	try:
		ser.flushInput() #flush input buffer, discarding all its contents
		ser.flushOutput()#flush output buffer, aborting current output   

		print "Connecting to badge\n"
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
			print "[ERROR]Badge not ready\n" 
			ser.close()
			exit(1)
		else:
			print "Got READY, out=%s\n" % out

		file_lines = (line.rstrip('\n') for line in file_data.splitlines())

		print "Sending program\n"
		file_checksum = 0
		count = 0
		for line in file_lines:
			if ( len(line) < 4 ):
				continue

			ser_writesafe_line( ser, line.lstrip() )

			value =  struct.unpack( '>H', binascii.a2b_hex( line.lstrip() ) )[0] 
			file_checksum = file_checksum + value	
			print "Line %s  value: %04x\n" % (line.lstrip(), value)

			count = count + 1

		print "Sent %d lines.\n" % count
		print "File checksum: 0x%08x\n" % (file_checksum & 0xffffffff)

		# SEND CRC
		checksum_bin_data = struct.pack( '>I', file_checksum )

		ser_writesafe_line( ser, binascii.b2a_hex( checksum_bin_data ) )

		time.sleep(0.5)

		out = ""
		while ser.inWaiting() > 0:
			out += ser.read(1)

		print "Out: %s\n" % out

		ser.close()

	except Exception, e1:
		print "error communicating...: " + str(e1)



if __name__ == "__main__":
	parser = argparse.ArgumentParser(description='Badger Programming Script')
	parser.add_argument('filename', metavar='filename', 
                   help='Mem file to flash onto the badge')
	parser.add_argument('-p', '--port', default='/dev/ttyUSB0', help='Sets the serial port (default=/dev/ttyUSB0)')
	program_args = vars(parser.parse_args())
	flash_board(program_args['port'], program_args['filename'])

