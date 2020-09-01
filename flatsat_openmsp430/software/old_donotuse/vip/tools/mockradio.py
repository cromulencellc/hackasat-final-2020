#!/usr/bin/python
#Temporary mock radio test layer
import socket              
import serial
import SocketServer
import threading
import Queue
import binascii
import struct



msgQueue = Queue.Queue()

class ConnectHandler(SocketServer.BaseRequestHandler):
	def handle(self):
		try:	
			self.request.settimeout(30)
			self.data = self.request.recv(1024)
			print("RECEIVED: %d" % len(self.data))
			msgQueue.put(self.data)

		except socket.timeout:
			self.request.close()

		return

class Server(SocketServer.ThreadingMixIn, SocketServer.TCPServer):
	
	daemon_threads = True
	allow_reuse_address = True

	pass


def checksum(msg):
	checksum = 0
	for byte in msg:
		checksum += ord(byte)
	return struct.pack(">B", checksum % 256)


if __name__ == "__main__":

	server = Server( ("localhost", 8000), ConnectHandler )
	
	t = threading.Thread(target=server.serve_forever)
	t.setDaemon( True ) # don't hang on exit
	t.start()

	ser = serial.Serial("/dev/ttyUSB1", 115200, timeout=0, parity=serial.PARITY_NONE, rtscts=0)
	
	try:
		ser.open()
		if ( ser.isOpen == False ):
			print "Failed to open serial console"
			exit()

	except Exception, e:
		print "Serial Error : " + str(e)
		exit()

	while ( True ):
		if not (msgQueue.empty()):
			msg = msgQueue.get()
			
			if (len(msg)< 86):
				msg += "\x00" *((86 - len(msg)))
			cs = checksum(msg)
			msg = binascii.hexlify(msg)
			msg = "VP:" + msg + "," + binascii.hexlify(cs)
			print "Sending: " + msg + "\n"
			ser.write(msg)
			msgQueue.task_done()
		# Print output if present 
			print ser.read(1024)		

		


	server.socket.close()
