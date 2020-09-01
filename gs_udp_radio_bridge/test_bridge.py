#!/usr/bin/env python3 
# -----------------------------------------------------------------------------    
# Communicates with bridge.py via UDP, and exercises some commands and error 
# handlers.   
# -----------------------------------------------------------------------------    
import socket
import time
from signal import signal, SIGINT
import fcntl, os
import errno
import sys
import time
import random
# -----------------------------------------------------------------------------
from settings_common import *
from helpers import *
# -----------------------------------------------------------------------------
# Settings
# -----------------------------------------------------------------------------

# We default to being this boss;

MY_BOSS_ID        = 8
MY_COMMS_SETTINGS = BOSSES[MY_BOSS_ID-1]

MY_IP             = MY_COMMS_SETTINGS[0]  # ip addr
MY_RX_PORT        = MY_COMMS_SETTINGS[1]  # local rx port
BRIDGE_IP         = MY_COMMS_SETTINGS[2]
BRIDGE_RX_PORT    = MY_COMMS_SETTINGS[3]  # bridge rx port

# -----------------------------------------------------------------------------
# globals
# -----------------------------------------------------------------------------
g_quiet = True      # Show hex for all TX and RX
g_radio_bits = 0    # bridge radio query result

# -----------------------------------------------------------------------------
# socket setup
# -----------------------------------------------------------------------------

rx_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
fcntl.fcntl(rx_sock, fcntl.F_SETFL, os.O_NONBLOCK)
rx_sock.bind((MY_IP, MY_RX_PORT))
tx_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# -----------------------------------------------------------------------------
# send command to/through bridge
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
    lb = len(data)
    #if lb>16:
    #  print("      Tx: '{:s}:{:s}...' ({:d} bytes)".format(stag, data[0:16].hex(),lb))
    #else:      
    print("      Tx: '{}:{}'".format(stag,data.hex()))
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
        for i in range(0,4):
          b=msg[i]
          if b == ord(':'):
            colon_at=i
            break
        if colon_at < 2: 
          print('cmdResp() got invalid response. Tag too short. colonat {} tag {}'.format(colon_at,msg[0:colon_at].decode('utf-8')))
          return None,None,None
        tag  = msg[0:colon_at].decode('utf-8')
        if tag == 'WIN':    # ignore comms window updates
          continue
        data = msg[colon_at+1:]
        if not g_quiet:                    
          lb = len(data)
          #if lb>16:
          #  print("      Rx: '{:s}:{:s}...' ({:d} bytes) {:d} ms".format(tag, data[0:16].hex(),lb,int(msecs)))
          #else:
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
# Boss commands to OOBM
# -----------------------------------------------------------------------------
def oobmHail():
  print("  Hailing oobm:")     
  otag = 'OM'
  itag, data, msecs = cmdResp(otag,CMD_HAIL)
  indent = ' '*4
  if None in [itag,data,msecs]:
    msg = 'FAIL - No response'
  else:
    if itag == 'BR':    # if bridge doesnt know the radio, it returns fail.
      if data[-1] == 0:
        msg = 'FAIL'
      else:
        msg = 'PASS'
    elif itag != otag:
      msg = "Unexpected reply tag. Got '{}' Expected '{}'.".format(itag,otag)
    else:
      team_id = data[1]
      msg =   'TEAM ID : {}'.format(team_id+1)
  print(indent+msg)
# -----------------------------------------------------------------------------
def oobmLoopback(msg_len=250):
  otag = 'OM'

  loop_msg =CMD_LOOPBACK+bytearray(random.getrandbits(8) for _ in range(0,msg_len))

  itag, data, msecs = cmdResp(otag,loop_msg)
  indent = ' '*4
  if None in [itag,data,msecs]:
    msg = 'FAIL - No response'
  else:
    if itag == 'BR':    # if bridge doesnt know the radio, it returns fail.
      if data[-1] == 0:
        msg = 'FAIL'
      else:
        msg = 'PASS'
    elif itag != otag:
      msg = "Unexpected reply tag. Got '{}' Expected '{}'.".format(itag,otag)
    else:
      if data == loop_msg:
        msg =   '.'
      else:
        msg =   'FAIL\a'
  print(msg, end='')
  sys.stdout.flush()
# -----------------------------------------------------------------------------
# Boss commands to Radio
# -----------------------------------------------------------------------------
def radioHail():
  print("  Hailing radio:")     
  otag = 'RA'  
  itag, data, msecs = cmdResp(otag,CMD_HAIL)
  indent = ' '*4
  if None in [itag,data,msecs]:
    msg = indent + 'No response'
  else:
    if itag == 'BR':    # if bridge doesnt know the radio, it returns fail.
      if data[-1] == 0:
        msg = 'FAIL'
      else:
        msg = 'PASS'
    elif itag != otag:
      msg = "Unexpected reply tag. Got '{}' Expected '{}'.".format(itag,otag)
    else:
      team_id = data[1]
      mac = macAddrBytesToString(data[2:2+6])
      msg =   'RADIO ID : {}\n'.format(team_id+1)
      msg+= '{}RADIO MAC: {}'.format(indent,mac)
    print(indent+msg)
# -----------------------------------------------------------------------------
def radioLoopback( msg_len=250):
  #print("  Loopback msg radio:")
  otag = 'RA'
  loop_msg =CMD_LOOPBACK+bytearray(random.getrandbits(8) for _ in range(0,msg_len))
  #loop_msg =CMD_LOOPBACK+bytearray(0 for _ in range(0,msg_len))
  itag, data, msecs = cmdResp(otag,loop_msg)
  indent = ' '*4
  if None in [itag,data,msecs]:
    msg = 'FAIL - No response'
  else:
    if itag == 'BR':    # if bridge doesnt know the radio, it returns fail.
      if data[-1] == 0:
        msg = 'FAIL'
      else:
        msg = 'PASS'
    elif itag != otag:
      msg = "Unexpected reply tag. Got '{}' Expected '{}'.".format(itag,otag)
    else:
      if data == loop_msg:
        msg =   '.'
      else:
        msg =   'FAIL\a'
  print(msg, end='')
  sys.stdout.flush()

# -----------------------------------------------------------------------------
# Boss commands to Bridge
# -----------------------------------------------------------------------------
def bridgeQueryConnectedRadios(): # returns b'BR:Q' + uint16_t
  global g_radio_bits
  print("  Querying Bridge:")
  tag, data, msecs = cmdResp('BR',b'Q')    # ask bridge for radio list.
  if None in [tag,data,msecs]:
    return
  ld = len(data)
  if (ld<3) or (tag!='BR') or (data[0]!=ord('Q')):
    print('    Unexpected response to a radio query command. {:d} {:s} 0x{:02x}'.format(ld,tag,data[0]))
  else:
    # this is an occupancy map where bit 0 is radio 0, etc.
    g_radio_bits = (data[1]<<8) + data[2]
    print('    Response:')
    if g_radio_bits==0:
      print('      No radios! Sry.')
    else:
      for i in range(0,16):
        if g_radio_bits&(1<<i):
          print('      FOUND radio_{}!'.format(i+1))
# -----------------------------------------------------------------------------
def bridgeSeizeRadio():
  global g_radio_bits  
  print("  Seizing radio:") 
  msg = b'S'
  # ask bridge to seize radio
  tag, data, msecs = cmdResp('BR',msg)    
  if None in [tag,data,msecs]:
    return
  ld = len(data)
  if (ld<2) or (tag!='BR') or (data[0]!=ord('S')):
    print('    Unexpected response to a seize command. {:d} {:s} 0x{:02x}'.format(ld,tag,data[0]))
    return False
  else:
    if data[1]==1:
      print('    PASS')
      return True
  print('    FAIL')
  return False
# -----------------------------------------------------------------------------
def bridgeUnseizeRadio():
  global g_radio_bits  
  print("  Unseizing radio:") 
  msg = b'U'
  # ask bridge to unseize radio
  tag, data, msecs = cmdResp('BR',msg)    
  if None in [tag,data,msecs]:
    return
  ld = len(data)
  if (ld<2) or (tag!='BR') or (data[0]!=ord('U')):
    print('    Unexpected response to an unseize command. {:d} {:s} 0x{:02x}'.format(ld,tag,data[0]))
    return False
  else:
    if data[1]==1:
      print('    PASS')
      return True
  print('    FAIL')
  return False

# -----------------------------------------------------------------------------
def main():
  global g_quiet
  
  getUtcTimestampMicros()

  print("\n*** UDP to Radio Bridge Tester v0.2\n")
  print("  Boss = ('{}',{})".format(MY_IP,MY_RX_PORT))
  print("Bridge = ('{}',{})\n".format(BRIDGE_IP,BRIDGE_RX_PORT))

  bridgeQueryConnectedRadios()   # updates g_radio_bits

  g_quiet=True

  if bridgeSeizeRadio():        # seize and hail radio_0
    radioHail()
    oobmHail()
    bridgeUnseizeRadio()
  
  # craig comms debug
  if bridgeSeizeRadio():
    print("  Loopback testing 1800-byte packets with radio module:\n    ",end='')
    for i in range(0,8):
      radioLoopback(1800)    
    print()
    print("  Loopback testing 1800-byte packets with the OOBM:\n    ",end='')
    for i in range(0,65536):
      oobmLoopback(1800)    
    print()
    bridgeUnseizeRadio()
  
  print('')    
# -----------------------------------------------------------------------------
if __name__ == "__main__":
  main()
# -----------------------------------------------------------------------------
# -----------------------------------------------------------------------------
