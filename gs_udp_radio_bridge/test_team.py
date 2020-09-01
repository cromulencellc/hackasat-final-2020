#!/usr/bin/env python3 
# -----------------------------------------------------------------------------    
import socket
import time
from signal import signal, SIGINT
import fcntl, os
import errno
import sys
import struct
# -----------------------------------------------------------------------------
from settings_common import *
from helpers import *
# -----------------------------------------------------------------------------
# Settings
# -----------------------------------------------------------------------------

# we default to being the first team defined in the settings

MY_TEAM_ID        = 1
MY_COMMS_SETTINGS = TEAMS[MY_TEAM_ID-1]

MY_IP             = MY_COMMS_SETTINGS[0]  # ip addr
MY_RX_PORT        = MY_COMMS_SETTINGS[1]  # local rx port
BRIDGE_IP         = MY_COMMS_SETTINGS[2]
BRIDGE_RX_PORT    = MY_COMMS_SETTINGS[3]  # bridge rx port

RESP_TIMEOUT_SECS = 2    # how long we wait for other types of radio responses

# -----------------------------------------------------------------------------
# globals
# -----------------------------------------------------------------------------
g_quiet = False      # Show hex for all TX and RX

# -----------------------------------------------------------------------------
# socket setup
# -----------------------------------------------------------------------------

rx_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
fcntl.fcntl(rx_sock, fcntl.F_SETFL, os.O_NONBLOCK)
rx_sock.bind((MY_IP, MY_RX_PORT))
tx_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# -----------------------------------------------------------------------------
# send command through bridge
# -----------------------------------------------------------------------------
def cmd(tag,data,port=BRIDGE_RX_PORT):
  global g_quiet
  if isinstance(tag,str):
    stag=tag
    tag=tag.encode('utf-8')
  elif isinstance(tag,bytes):
    stag=tag.decode('utf-8')
  else:
    print('cmd(): Error, tag must be bytes or string.')
  if not g_quiet:
    print("    Tx: '{}:{}'".format(stag,data.hex()))
  msg = tag+b':'
  msg += data
  tx_sock.sendto(msg, (BRIDGE_IP, BRIDGE_RX_PORT)) # udp send
# -----------------------------------------------------------------------------
# send command, await response, with optional retries.
# -----------------------------------------------------------------------------
def cmdResp(tag,data,max_retries=0,timeout_secs=RESP_TIMEOUT_SECS,port=BRIDGE_RX_PORT):
  global g_quiet
  if isinstance(tag,str):
    stag=tag
    tag=tag.encode('utf-8')
  elif isinstance(tag,bytes):
    stag=tag.decode('utf-8')
  else:
    print('cmdResp(): Error, tag must be bytes or string.')

  tries = 0
  while tries <= max_retries:
    cmd(tag=tag,data=data)  # send command
    tries+=1
    ts = time.time()  # note start time
    while True:  # loop awaiting response
      try:
        msg = rx_sock.recv(4096)
        # got response.
        te=time.time()
        msecs = (te-ts)*1000
        if len(msg)<3:
          print('cmdResp() got invalid response. Too short.')
          return None,None,None
        colon_at = -1
        for i in range(0,5):
          b=msg[i]
          if b == ord(':'):
            colon_at=i
        if colon_at < 2: 
          print('cmdResp() got invalid response. Tag too short.')
          return None,None,None
        tag  = msg[0:colon_at].decode('utf-8')
        data = msg[colon_at+1:]
        if not g_quiet:
          print("      Rx: '{:s}:{:s}' {:d} ms".format(tag, data.hex(),int(msecs)))
        return tag, data, msecs
      except socket.error as e:
        err = e.args[0]
        if err == errno.EAGAIN or err == errno.EWOULDBLOCK:
          pass
        else:
          print('\ncmdResp(): Fatal exception: {}\n'.format(e))
          exit(-1)
      te=time.time()
      if te-ts > timeout_secs: # timeout waiting for response
        if tries>max_retries:
          return None, None, None
        else:
          break
      else:
        time.sleep(0.005)
  return None, None, None
# -----------------------------------------------------------------------------
# CCSDS Format:
#
# sync_word:  de ad be ef +
# uint16_t    stream_id (big endian) + 
# uint16_t    sequence_count (b.e.) + 
# uint16_t    packet_length (b.e.) + 
# uint8_t     data[packet_length + 1 ]
# -----------------------------------------------------------------------------
def sendCCSDS(strm, seq, payload):
  buf = b'\xde\xad\xbe\xef'
  buf += struct.pack('>H',strm)
  buf += struct.pack('>H',seq)
  lpl = len(payload)
  buf += struct.pack('>H',lpl)
  buf += payload+b'\x00'
  cmd(tag='L3', data=buf)
# -----------------------------------------------------------------------------
def main():
  global g_quiet
  
  getUtcTimestampMicros()

  print("\n*** Team UDP Simulator v0.2\n")
  print(" Team# = {}".format(MY_TEAM_ID))
  print("  Team = ('{}',{})".format(MY_IP,MY_RX_PORT))
  print("Bridge = ('{}',{})\n".format(BRIDGE_IP,BRIDGE_RX_PORT))

  '''
  
  # checking the error checking of team messages in bridge

  print('Send a valid leon3 message:')
  sendCCSDS(strm=0, seq=0, payload=b'\x00\x00\x00\x00')
  
  print('Send a bad message type:')
  cmd(tag='RA', data=b'\x00') 
  print('Send an invalid leon3 message (bad header):')
  cmd(tag='L3', data=b'\x00') # send bad CCSDS message


  print('Send a bad rsc message:')
  cmd(tag='RSC', data=b'\x00') 

  #print('Send an invalid leon3 message (payload > 2048 bytes):')
  #sendCCSDS(strm=0, seq=0, payload=b'\x00'*2049) # too big leon 3 msg

  '''
  print('Send radio bitrate + power settings')
  
  #cmd(tag='RSC', data=b'\x00\x00')  # doesnt satisfy challenge
  cmd(tag='RSC', data=b'\x00\x01') # does satisfy


  print('')    
# -----------------------------------------------------------------------------
if __name__ == "__main__":
  main()
# -----------------------------------------------------------------------------
# -----------------------------------------------------------------------------
