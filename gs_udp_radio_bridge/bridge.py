#!/usr/bin/env python3
# -----------------------------------------------------------------------------
#
# bridge.py
#
# Enumerates the USB-connected radios, sets up UDP client listener classes for 
# each Team, Watcher, and Boss, and then acts as a bridge.
#
# -----------------------------------------------------------------------------
# Types of Bridge Clients: (Enforced based on assigned IP)
# -----------------------------------------------------------------------------
#
# Teams:
# -----
#   - Can only send/recv messages of type L3 and RSC, thru/to their own radio, 
#     with their own flatsat. 
#   - The src IP of a message is checked against the assigned IP for that team.
#
# Bosses:
# ------
#   - One per team.
#   - Can send any message type besides RSC.  (OM, L3, RA, BR)
#   - Can seize their Team's radio:
#       . While siezed, normal team-radio comms semi-gracefully paused.
#       . When Boss is done, it tells the bridge to release the radio.
#
# Watchers:
# --------
#   - One per team.
#   - Receive the same L3 messages as their team receives
#   - Can only send/recv messages of type L3 and RSC, thru/to their own radio, 
#     with their own flatsat. 
#   - Not subject to boss radio seizure or comms blackouts
#
# -----------------------------------------------------------------------------
# UDP Message format, delimited by end of UDP message:
# ---------------------------------------------------
#
#  Command : <ascii_tag><bytes>
#  Response: <same_tag><bytes>
#
#  Team Tags:
#  ---------
#   'L3:' (Leon3 ): Cosmos CCSDS telemetry and ground commands.
#                   (Only with assigned flatsat and only if src IP matches.)
#     'RSC:' initial bringup challenge - Must lower bitrate and increase power.
#        uint8_t bitrate;    # 0:low (more reliable), 1:high (normal)
#        uint8_t power;      # 0:low  (power saving), 1:high (more_reliable)
#
#  Watcher:
#  -------
#    Get CC'ed on incoming L3 messages for their assigned team.
#
#  Boss Tags:
#  ---------
#    'L3:'  (Leon3 ): Cosmos CCSDS telemetry and ground commands.
#      (For protocol see radio.py)
#    'OM:'  (MSP430): Oob management (L3 Reset, Reflash.)
#      (For protocol see radio.py)
#    'BR:'  (Bridge): 
#      Boss Commands to Bridge:
#        'BR:S': Seize radio from team:
#          Bridge returns orig. msg with added byte:  
#            'BR:S',0x00 for fail
#            'BR:S',0x01 for pass
#        'BR:U': Unseize radio; unpause team comms:
#          Bridge returns orig. msg with added byte:  
#            'BR:U',0x00 for fail
#            'BR:U',0x01 for pass
#
#
# -----------------------------------------------------------------------------
import socket
import serial
from serial.tools.list_ports import comports
import threading
import time
from signal import signal, SIGINT
import fcntl, os
import errno
from datetime import datetime
import base64
import sys

# -----------------------------------------------------------------------------
from settings_common  import *                # TEAM IPs and Ports
from defines          import *
from helpers          import *
from radio            import  Radio, g_radios, enumerateRadios
from client           import  Client, Watcher, Boss, Team, g_clients, g_teams,\
                              g_bosses,g_watchers,g_num_clients
# -----------------------------------------------------------------------------
# Globals
# -----------------------------------------------------------------------------
g_running = True
# -----------------------------------------------------------------------------
# Functions
# -----------------------------------------------------------------------------
def handleCtrlC(signal_received, frame):
  global g_running
  g_running = False
# -----------------------------------------------------------------------------    
def usageDie():
  print('\nUsage: bridge.py <team number 1..9>\n')
  exit(-1)                          
# -----------------------------------------------------------------------------      
def main():
  only_this_radio=None

  if len(sys.argv)!=2:
    usageDie()
  else:
    try:
      only_this_radio = int(sys.argv[1])
      if (only_this_radio<1) or (only_this_radio>9):
        usageDie()
    except:
      usageDie()
    if (only_this_radio is None) or (only_this_radio<0) or (only_this_radio>9):
      usageDie()      

  print("\n*** UDP to Radio Bridge v0.2\n")

  try:
    nv = os.nice(0) 
    if nv == 0:
      nv = os.nice(-1000) 
      print("  INFO: os.nice set to: {}\n".format(nv))
  except:
    print("  WARN: Couldn't decrease process nice value.\n"+\
          "        If needed, try running as root.\a\a\n")

  enumerateRadios(only_this_radio=only_this_radio-1)

  num_radios = len(g_radios)
  if not num_radios:
    print('\n  *** No radios found. Sry.')
  else:
    # I dont always do stuff like this, I swear:
    if num_radios == 1:
      s = ''
      a = 'is'
    else:
      s = 's'
      a = 'are'
    print('\n  Starting radio thread{}:'.format(s))
    for t in g_radios:
      r=g_radios[t]
      r.start()
      while not r.running:
        pass
      # make a team for each radio and add hook it to get radio callbacks
      c=Team(client_id=r.radio_id)
      g_radios[t].setDefaultClientRef(client_ref=c)

    # make a watcher and boss for each radio
    for t in g_radios:

      r=g_radios[t]

      ip_addr, bridge_tx, bridge_ip, bridge_rx = WATCHERS[t]
      w = Watcher(
              ip_addr=ip_addr,
              bridge_rx=bridge_rx,
              bridge_tx=bridge_tx,
              radio_id = t,
              radio_ref =r,
              bridge_ip=bridge_ip)                    
      r.addWatcherRef(w)

      ip_addr, bridge_tx, bridge_ip, bridge_rx = BOSSES[t]      
      b = Boss(
        ip_addr=ip_addr,
        bridge_rx=bridge_rx,
        bridge_tx=bridge_tx,
        boss_radio_ref=r,
        radio_id = t,
        bridge_ip=bridge_ip)      

    print('\n  Starting client thread{}:'.format('' if g_num_clients == 1 else 's'))
    print("{:>10s} {:<15s} {:<7s} {:<7s} {}".format('    ','          ','Bridge ','Bridge ','Radio'))
    print("{:>10s} {:<15s} {:<7s} {:<7s} {}".format('Name','IP Address','Rx Port','Tx Port','Status'))
    print("{:>10s} {:<15s} {:<7s} {:<7s} {}".format('----','----------','-------','-------','------'))

    for client in g_clients:
      client.start()
      client.report()

    # get ready to go into mainloop
    signal(SIGINT, handleCtrlC)
    m='{} Team{} {} ready to rock.  <Ctrl-C Quit>'.format(num_radios,s,a)
    boxPrint('-',m,2)

    # mainloop
    while g_running: 
      time.sleep(0.25)

    # cleanup clients and radios

    print('\n  Ending client thread{}:'.format('' if g_num_clients == 1 else 's'))
    for client in g_clients: 
      client.running = False
      client.join()

    print('\n  Ending radio thread{}:'.format(s))
    for rn in g_radios: 
      r=g_radios[rn]  
      r.running = False
      r.join()

  print('')
# -----------------------------------------------------------------------------
if __name__ == "__main__":
  main()
# -----------------------------------------------------------------------------
# -----------------------------------------------------------------------------
