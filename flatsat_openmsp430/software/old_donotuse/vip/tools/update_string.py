#!/usr/bin/python

import sys
import re
import vip_message

message_file = "vip_strings.dat"

def usage():
	print "Usage: %s <string ID> Message\n" % sys.argv[0]
	exit()

def read_messages():
	messages = []
	try:
		file = open(message_file, "r")
	except:
		print "Error opening string file\n"
		exit()

	lines = file.readlines()
	for each in lines:
		match = re.search("\".*\"", each)
		if match != None:
			messages.append(match.group(0).strip("\""))
	return messages

def write_messages(string_id, old_msg, new_msg):
	file = open(message_file, "r")
	lines = file.readlines()
	new_line = lines[string_id].replace(old_msg, new_msg)
	lines[string_id] = new_line
	file.close()
	file = open(message_file, "w")
	file.write("".join(lines))

if __name__ == "__main__":

		if len(sys.argv) < 2:
			usage()

		messages = read_messages()

		string_id = int(sys.argv[1]) 
		if (string_id < 0) or (string_id > len(messages)):
			usage() 
		new_msg = sys.argv[2]
		msg = "CT" + "{0:0{1}x}".format(string_id, 4) + new_msg.decode('string_escape')
		vip_message.send_vip_msg(msg)
		print messages
		write_messages(string_id, messages[string_id], new_msg)
