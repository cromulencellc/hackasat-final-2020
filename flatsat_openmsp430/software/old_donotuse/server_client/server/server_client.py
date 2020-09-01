#!/usr/bin/env python
import socket               # Import socket module
import serial, time
import binascii
import struct
import SocketServer
import threading
import logging
import Queue
import argparse
from select import select
import sys
import parse

# Time to allow a connection to stay open (in seconds)
TIMEOUT = 20

# SLA team list
sla_team_list = list()
token_team_list = list()	

# Setup the vip message queue (allow only 1 message maximum in the queue)
vip_message_queue = Queue.Queue(1)

def init_sla_list():
	# Default to SLA pass~
	for i in range(0, 20):
		sla_team_list.append(True)

def init_token_list():
	# Default to empty strings
	for i in range(0, 20):
		token_team_list.append("")

class SLAHandler(SocketServer.BaseRequestHandler):
	def handle(self):
		try:	
			self.request.settimeout(TIMEOUT)
			self.data = self.request.recv(1024).strip()

			print "SLA {} wrote:".format(self.client_address[0])
			print self.data

			if ( self.data.find("TEAM") == 0 ):
				team_num = int(self.data[4:])
			
				if ( team_num < 20 ):
					sla_for_team = sla_team_list[team_num]

					if ( sla_for_team == True ):
						self.request.sendall('PASS')
					else:
						self.request.sendall('FAIL')

				else:
					self.request.sendall('ERROR')
			
			self.request.sendall('\n')
			self.request.close()

		except socket.timeout:
			self.request.close()

		return

class SLAServer(SocketServer.ThreadingMixIn, SocketServer.TCPServer):
	
	daemon_threads = True
	allow_reuse_address = True

	pass

class TokenHandler(SocketServer.BaseRequestHandler):
	def handle(self):
		try:
			self.request.settimeout(TIMEOUT)
			self.data = self.request.recv(1024).strip()

			print "SLA {} wrote:".format(self.client_address[0])
			print self.data

			if ( self.data.find("DEPOSIT") == 0 ):
				team_num = int(self.data[7:])
			
				if ( team_num < 20 ):
					token_for_team = token_team_list[team_num]

					self.request.sendall(token_for_team)

				else:
					self.request.sendall('ERROR')
			
			self.request.sendall('\n')
			self.request.close()
		
		except socket.timeout:
			self.request.close()
		
		return

class TokenServer(SocketServer.ThreadingMixIn, SocketServer.TCPServer):
	
	daemon_threads = True
	allow_reuse_address = True
	
	pass

class VIPHandler(SocketServer.StreamRequestHandler):
	def handle(self):
		try:
			self.request.settimeout(TIMEOUT)
			self.data = self.rfile.readline().strip()

			if ( self.data.find("vip") == 0 ):
				# Send VIP message to badge
				vip_message_queue.put( self.data )	
		
		except socket.timeout:
			self.request.close()
		
		return

class VIPServer(SocketServer.ThreadingMixIn, SocketServer.TCPServer):
	
	daemon_threads = True
	allow_reuse_address = True
	
	pass

def process_serial_line( line_buf ):
	if ( line_buf.find('SLR:') != -1 ):
		results = parse.parse('SLR: {:d}:{:d}:{}', line_buf)

		if ( results is not None ):
			round_num = results[0]
			team_num = results[1]
			pass_result = results[2]

			if ( pass_result == 'PASS' ):
				sla_team_list[team_num] = True
			else:
				sla_team_list[team_num] = False

	elif ( line_buf.find('TOK:') != -1 ):
		results = parse.parse('TOK: Token:{:d}:{}', line_buf )

		if ( results is not None ):
			team_num = results[0]
			token_string = results[1]

			token_team_list[team_num] = token_string		

class SerialBadgeConnection:
	
	def Run( self, serial_port ):
		ser = serial.Serial( )
		ser.port = serial_port
		ser.baudrate = 115200
		ser.bytesize = serial.EIGHTBITS
		ser.parity = serial.PARITY_NONE
		ser.stopbits = serial.STOPBITS_ONE

		ser.timeout = 0

		ser.xonxoff = False
		ser.rtscts = False
		ser.dsrdtr = False

		select_timeout = 0.01

		sys.stdout.write('Server> ')
		sys.stdout.flush()

		line_buf = ""

		try:
			ser.open()
			ser.flushInput()
			ser.flushOutput()

			if ( ser.isOpen == False ):
				print "Failed to open serial console"
				return

			while ( True ):
				rlist, _, _ = select([sys.stdin], [], [], 0.1 )

				if sys.stdin in rlist:
					in_line = sys.stdin.readline()
					new_line = in_line.rstrip().encode()

					print "Sent: %s" % new_line
				
					ser.flush()
					ser.write( new_line + '\n' )
					#time.sleep(0.01)
					#ser.write('\n')
					ser.flush()

					sys.stdout.write('Server> ')
					sys.stdout.flush()
					# Serial port stuffz
				
				while ( ser.inWaiting() > 0 ):
					char = ser.read(size=1)

					if ( char == '\n' ):
						# Process the serial line
						process_serial_line( line_buf )

						print "Line: %s\n" % line_buf

						line_buf = ""	
						sys.stdout.write('Server> ')
						sys.stdout.flush()
					else:	
						line_buf += char 

				ser.write('\n')
				ser.flush()	

				# SEND VIP messages to the badge if they are available
				try:
					vip_message = vip_message_queue.get_nowait()
				except Queue.Empty:
					# Ignore
					pass
				else:
					# Process VIP message -- send to badge
					sys.stdout.write('[VIP] ' + vip_message.encode() + '\n')
					sys.stdout.flush()

					ser.write( vip_message.encode() + '\n' )
					ser.flush()
				
				# Run serial connection to server badge
				#if ( token_deposit_queue.get_nowait() is not None ):
				#	print 'TODO: Send deposit to server'

				# Check for input
				
					

		except Exception, e:
			ser.close()
			print "Serial Error : " + str(e)
			raise e
			return

		ser.close()
		return


if __name__ == "__main__":
	parser = argparse.ArgumentParser(description='Badge SLA laptop client')
	parser.add_argument('-p', '--port', default='/dev/ttyUSB0', help='Sets the serial port')
	program_args = vars(parser.parse_args())
	

	# Initialize the SLA poll results
	init_sla_list()
	init_token_list()

	# Run SLA poll server
	sla_address = ( '0.0.0.0', 8000 )

	sla_server = SLAServer( sla_address, SLAHandler )
	sla_ip, sla_port = sla_server.server_address

	t = threading.Thread(target=sla_server.serve_forever)
	t.setDaemon( True ) # don't hang on exit
	t.start()

	logger = logging.getLogger('client')
	logger.info( 'SLA server on %s:%s', sla_ip, sla_port )

	# Run Token Server
	token_address = ( '0.0.0.0', 8001 )
	
	token_server = TokenServer( token_address, TokenHandler )
	token_ip, token_port = token_server.server_address

	t2 = threading.Thread(target=token_server.serve_forever)
	t2.setDaemon(True)
	t2.start()

	logger.info( 'Token server on %s:%s', token_ip, token_port )

	# Run VIP Server
	vip_address = ( '0.0.0.0', 8002 )
	
	vip_server = VIPServer( vip_address, VIPHandler )
	vip_ip, vip_port = vip_server.server_address

	t3 = threading.Thread(target=vip_server.serve_forever)
	t3.setDaemon(True)
	t3.start()
	
	logger.info( 'VIP server on %s:%s', token_ip, token_port )
	

	# Run serial connection
	ser_connection = SerialBadgeConnection()
	ser_connection.Run( program_args['port'] )

	
	sla_server.socket.close()
	token_server.socket.close()
