#!/usr/bin/python

import subprocess

file = open("vipnamelist.txt", "r")
names = file.readlines()
args = []
for each in names:
	name = each.strip()
	name_und = name.replace(' ', '_')

	arg1 = "VIP_NAME=\"\\\"" + name + "\\\"\"" 
	arg2 = "NAME=vip_badge_" + name_und
	print args
	subprocess.call(["rm", "rain.o"])
	subprocess.call(["make", arg1, arg2])
	subprocess.call(["rm", "rain.o"])