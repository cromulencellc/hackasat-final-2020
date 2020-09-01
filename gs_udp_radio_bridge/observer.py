#!/usr/bin/env python3 
# -----------------------------------------------------------------------------    
import socket
import time
from signal import signal, SIGINT
import fcntl, os
import errno
import sys
from datetime import timezone 
from datetime import datetime
import struct 

from defines import *
from settings_common import *
# -----------------------------------------------------------------------------
# Settings
# -----------------------------------------------------------------------------
OBSERVER_IP          = '127.0.0.1'
OBSERVER_UDP_PORT    = 5017  # listening for UDP from bridge on this port
BRIDGE_IP = "127.0.0.1"
BRIDGE_RX_PORT = 5037

tx_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
g_quiet = True
# -----------------------------------------------------------------------------
g_running = True
# -----------------------------------------------------------------------------
class udpListener:
  rx_sock   = None
  opened    = False
  # ---
  def __init__(self):
    self.rx_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
      fcntl.fcntl(self.rx_sock, fcntl.F_SETFL, os.O_NONBLOCK)
      self.rx_sock.bind((OBSERVER_IP, OBSERVER_UDP_PORT))
      self.opened = True
    except:
      self.opened = False
  # ---
  def close(self):
    if self.opened:
      self.rx_sock.close()
      self.opened = False
  # ---
  def run(self):
    try:
      msg = self.rx_sock.recv(4096) # get udp msg
      return msg
    except socket.error as e:
      err = e.args[0]
      if err == errno.EAGAIN or err == errno.EWOULDBLOCK:
        return None
      else:
        print('\n*** rx_skt.recvfrom() FATAL: {}\n'.format(e))
        exit(-1)
# -----------------------------------------------------------------------------
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
def handleCtrlC(signal_received, frame):
  global g_running
  g_running = False        
# -----------------------------------------------------------------------------    
def main():
  global g_running
  print("\n*** Observer Listener v0.2")
  listener = udpListener()
  signal(SIGINT, handleCtrlC)
  
  l='  -- Logging comms transactions <Ctrl-C Quit> --'
  b='  '+('-'*(len(l)-2))

  print('')
  print(b)
  print(l)
  print(b)
  print('')
  j=0
  while g_running:         # Wanted to turn on oobm loopback mode here,
    msg = listener.run()
    if not msg is None:
      colon_at = -1
      for i in range(0,5):
        if msg[i]==ord(':'):
          colon_at=i
          break
      if colon_at>0:
        tag=msg[0:colon_at].decode('utf-8')
        msg = msg[colon_at+1:]
        print("{}:'{}'".format(tag,msg.hex()))
      j+=1
      if j==5:
        j=0
        print('Send radio bitrate + power settings')
        cmd(tag='RSC', data=b'\x00\x01') # does satisfy        

  print('\nClosing.')
  listener.close()
  print('')    
# -----------------------------------------------------------------------------
if __name__ == "__main__":
  main()
# -----------------------------------------------------------------------------
# -----------------------------------------------------------------------------
