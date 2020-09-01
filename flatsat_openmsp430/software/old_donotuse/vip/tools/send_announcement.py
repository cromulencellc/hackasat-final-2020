#!/usr/bin/python

def send_announcement(msg):

		try:
			count = open("announcecount.dat", "r").read()
		except:
			count = "0000"
		intcount = int(count, 16)    
	
		intcount += 1
		count = "{0:0{1}x}".format(intcount, 4) 


		msg = "AN" + count + msg.decode('string_escape')

		out = open("announcecount.dat", "w+")
		out.write(count)
		out.close()

		return msg
