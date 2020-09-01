#!/usr/bin/python

# Badge deposit script -- retrieves current rounds token

import socket               # Import socket module
import sys

team_ip_list = [
"10.5.1.2",
"10.5.2.2",
"10.5.3.2",
"10.5.4.2",
"10.5.5.2",
"10.5.6.2",
"10.5.7.2",
"10.5.8.2",
"10.5.9.2",
"10.5.10.2",
"10.5.11.2",
"10.5.12.2",
"10.5.13.2",
"10.5.14.2",
"10.5.15.2",
"10.5.16.2",
"10.5.17.2",
"10.5.18.2",
"10.5.19.2",
"10.5.20.2" ]

if __name__ == "__main__":
	
	# Check arguments
	if ( len (sys.argv) != 2 ):
		print "Need IP address"
		exit(0)

	try:
		team_num = team_ip_list.index( sys.argv[1] )
	except ValueError, e:
		print "Unknown IP address"
		exit(0)

	print "Team num: %d" % team_num
	
	s = socket.socket()

	# Badge server_client ip is 10.3.1.16
	host = "127.0.0.1"
	#host = "10.3.1.16"
	port = 8001		# Token deposit port 

	exit_status = 0
	try:
		s.connect((host, port))
		s.sendall( "DEPOSIT%d" % team_num )
		result = s.recv(1024)

		print "Team ID %d: %s\n" % (team_num, result)

		print "!!legitbs-replace-token %s" % result
	
		s.close                     # Close the socket when done
	except Exception, e:
		print "Socket exception: "
		print e
		pass

	exit( 0 )
