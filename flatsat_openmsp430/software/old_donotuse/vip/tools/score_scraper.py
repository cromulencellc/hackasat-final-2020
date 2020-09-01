#!/usr/bin/python
# Scrape the score and send to badge server

import sys
import os
import vip_message
import urllib2
import json
import struct

#SCORE_SERVER_URL = "https://live.legitbs.net/2014_finals.json"
SCORE_SERVER_URL = "file://" + os.getcwd() + "/scoreboard.json"

teams = {
	"Plaid Parliament of Pwning": 0,
    "9447": 1,
    "Reckless Abandon": 2,
    "Routards": 3,
    "raon_ASRT": 4,
    "KAIST GoN": 5,
    "shellphish": 6,
    "CodeRed": 7,
    "HITCON": 8,
    "blue-lotus": 9,
    "HackingForChiMac": 10,
    "(Mostly) Men in Black Hats": 11,
    "w3stormz": 12,
    "More Smoked Leet Chicken": 13,
    "Dragon Sector": 14,
    "[SEWorks]penthackon": 15,
    "Stratum Auhuur": 16,
    "Gallopsled": 17,
    "BalalaikaCr3w": 18,
    "binja": 19
}
    
def fetch_score():
    score = []
    connection = urllib2.urlopen(SCORE_SERVER_URL)
    data = json.loads(connection.read())
    standings = data['standings']
    standings.sort(key=lambda e:e['team'])
    standings.sort(key=lambda e:e['pos'])
    # sort the listings by position
    for each in standings:
    	score.append(teams[each['team']])
    return score

def GetScore():
    score = fetch_score()
    print "Fetched score: " + str(score)
    try:
        count = open("scorecount.dat", "r").read()
    except:
        count = "0000"
    intcount = int(count, 16)    
    intcount += 1
    count = "{0:0{1}x}".format(intcount, 4) 
    msg = ""
    for each in score:
        msg += struct.pack("B", each)
   
    out = open("scorecount.dat", "w+")
    out.write(count)
    out.close()
    
    return "SU" + count + msg

if __name__ == "__main__":
    s = GetScore()
    print s

