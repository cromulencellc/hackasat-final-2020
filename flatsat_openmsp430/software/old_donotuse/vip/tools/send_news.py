#!/usr/bin/python
import time


def send_old_news(num):
	try:
		newslog = open("newslog.dat", "r").readlines()
	except:
		newslog = []
	if len(newslog) > num:
		return newslog[num]
	elif (len(newslog) > 0):
		return newslog[len(newslog)-1]
	else:
		return None

def send_news(msg):
	try:
		count = open("newscount.dat", "r").read()
	except:
		count = "0000"
	
	intcount = int(count, 16)    
	msg = msg.strip()
	if (msg == "clear"):
		intcount += 128
		msg = " "
	else:
		intcount += 1
	count = "{0:0{1}x}".format(intcount, 4) 
	msg = "NM" + count + '[' + time.strftime("%H:%M") + ']' + msg
	out = open("newscount.dat", "w+")
	out.write(count)
	out.close()
	try:
		out = open("newslog.dat", "r")
		data = out.read()	
		out.close()
	except:
		data = ""
	out = open("newslog.dat", "w+")
	out.write(msg + "\n" + data)
	out.close()
	return msg


