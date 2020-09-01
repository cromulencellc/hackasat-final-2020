#!/usr/bin/python

import binascii
import struct
import socket
import Queue
import time
import threading
import sys
import score_scraper
import send_news
import send_announcement

VIP_MSG_LENGTH = 86
VIP_SERVER = "localhost"
VIP_PORT = 8002
VIP_MSG_DELAY = 15
old_news_count = 0

# Lower numbers are higher priority
VIP_SCORE_PRI = 2
VIP_NEWS_PRI = 1 
VIP_NEWS_BL_PRI = 3
VIP_ANNOUNCE_PRI = 0
VIP_STRING_PRI  = 0
# q.put((#, msg))
vip_msg_q = Queue.PriorityQueue(10)


def checksum(msg):
	checksum = 0
	for byte in msg:
		checksum += ord(byte)
	return struct.pack(">B", checksum % 256)

def send_vip_msg(msg):

	if (len(msg) > VIP_MSG_LENGTH):
		msg = msg[0:86]

	if (len(msg) < VIP_MSG_LENGTH):
		msg = msg + "\x00" * (VIP_MSG_LENGTH-len(msg))

	message = binascii.hexlify(msg)
	message = "vip " + message + "," + binascii.hexlify(checksum(msg)).upper()
	sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	sock.connect((VIP_SERVER, VIP_PORT))
	try:
	    sock.sendall( message)
	finally:
	    sock.close()

def find_space(s, p):
	while((s[p]!=' ')and(p > 0)):
		p-=1
	return p

def SendNews(msg):
	while(len(msg)>19):
		space = find_space(msg, 19)
		if (space == 0):
			space = 19
		enc = send_news.send_news(msg[0:space] + '\n')
		vip_msg_q.put((VIP_NEWS_PRI, enc))
		if space != 19:
			msg = msg[space+1:]
		else:
			msg = msg[space:]
	if (len(msg)>0):
		enc = send_news.send_news(msg)
		vip_msg_q.put((VIP_NEWS_PRI, enc))


def SendAnnounce(msg):
	enc = ""
	while(len(msg)>14):
		space = find_space(msg, 14)
		if (space == 0):
			space = 14

		enc += msg[0:space] + '\n' 
		msg = msg[space+1:]
	if (len(msg)>0):
		enc += msg
	print enc
	msg = send_announcement.send_announcement(enc)
	vip_msg_q.put((VIP_ANNOUNCE_PRI, msg))

def ServiceQueue():
	while (True):
		item = vip_msg_q.get()
		send_vip_msg(item[1])
		time.sleep(VIP_MSG_DELAY)

def SendRoutine():
	global old_news_count
	while( True ):
		#send score
		vip_msg_q.put((VIP_SCORE_PRI, score_scraper.GetScore()))
		time.sleep(VIP_MSG_DELAY)
		#send news backlog
		msg = send_news.send_old_news(0)
		if (msg):

			print("Sending Old News 0 %s" % (msg))
			vip_msg_q.put((VIP_NEWS_BL_PRI, msg))
		else:
			print("No news 0")
		time.sleep(VIP_MSG_DELAY)	
		#send news backlog
		msg = send_news.send_old_news(1)
		if (msg):
			print("Sending Old News 1 %s" % (msg))
			vip_msg_q.put((VIP_NEWS_BL_PRI, msg))

		else:
			print("No news 1")

		time.sleep(VIP_MSG_DELAY)
		#empty slot for priority messages
		if(vip_msg_q.empty()):
			msg = send_news.send_old_news(old_news_count)
			if (msg):
				print("Sending Old News %d %s" % (old_news_count, msg))
				vip_msg_q.put((VIP_NEWS_BL_PRI, msg))
				old_news_count+=1
			else:
				print("Sending Blank Message")
				vip_msg_q.put((VIP_NEWS_BL_PRI, "blank message"))
				old_news_count = 0
		time.sleep(VIP_MSG_DELAY)

def RunCLI():
	try:
		while( True ):
			sys.stdout.write("VIP>")
			in_line = sys.stdin.readline()
			new_line = in_line.rstrip().encode()
			if (new_line == '?'):
				print "Commands: news, announce, qstat, clear, undo"
			elif (new_line == 'news?'):
				print "news <News message>"
				print "example: news Badger service released"
			elif (new_line == 'announce?'):
				print "announce <Announcement message>"
				print "example: announce First Blood goes to Blue Lotus"
			elif (new_line == 'qstat?'):
				print "Shows how many messages are in queue"
			elif (new_line == 'clear?'):
				print "Clear message queue"
			elif (new_line == 'undo?'):
				print "Removes top message from the message Queue. If you do this fast enough it may remove the last message you typed, but maybe not."
			elif (new_line.startswith('news ')):
				if (len(new_line) > 5):
					SendNews(new_line[4:])
			elif (new_line.startswith('announce ')):
				if (len(new_line) > 10):
					SendAnnounce(new_line[9:])
			elif (new_line.startswith('qstat')):
				print "Queue size: %d" % vip_msg_q.qsize()
			elif (new_line == "clear"):
				while (not vip_msg_q.empty()):
					vip_msg_q.get()
			elif (new_line == "undo"):
				if (not vip_msg_q.empty()):
					vip_msg_q.get()

	except KeyboardInterrupt:
		print "Goodbye"
	except Exception,e:
		print e



if __name__ == "__main__":

	old_news_count = 0
	t1 = threading.Thread(target=SendRoutine)
	t1.setDaemon(True)
	t1.start()
	t2 = threading.Thread(target=ServiceQueue)
	t2.setDaemon(True)
	t2.start()
	RunCLI()

	
