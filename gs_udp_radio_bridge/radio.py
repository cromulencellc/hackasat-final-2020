# -----------------------------------------------------------------------------
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
import struct
from copy import deepcopy
# -----------------------------------------------------------------------------
import random
random.seed()
# -----------------------------------------------------------------------------
from defines          import *
from helpers          import *
from settings_common  import *    # TEAM IPs and Ports
# -----------------------------------------------------------------------------
g_radios = {}
# -----------------------------------------------------------------------------
# For each USB UART that has 'CP2102' in its description, try hailing a radio.
# If we get back the expected response (team # and mac address) then set up a 
# Radio class for it and add to global dict of connected radios.
# -----------------------------------------------------------------------------
def enumerateRadios(only_this_radio = None):
  ports = comports()
  print('  Enumerating local radios:')
  for p in ports:
    print('    {:60s}'.format(str(p)))
    if 'CP2102' in p.description: # assuming all radios use this usb uart    
      try:
        # send CMD_HAIL        
        ser = serial.Serial(p.device, 115200, timeout=0,write_timeout=0.01,exclusive=True)
        ser.write(b'\xA5\x50\x00\x01\x00')
        ser.flush()
        # Await response, with timeout. No retries currently.
        ts = time.time()
        rxbuf = b''
        lrx = 0
        while True:
          in_len = ser.in_waiting                    
          if in_len:
            rxbuf += ser.read(in_len)
            while len(rxbuf) and rxbuf[0]!=0xA5:
              rxbuf = rxbuf[1:]
            lrx = len(rxbuf)
            if lrx>1 and rxbuf[1]!=0x50:
              rxbuf = rxbuf[1:]             # msg not for us.
              continue
            if lrx >= 11:                   # got full response
              break 
          te = time.time()
          if te-ts > HAIL_TIMEOUT_SECS:
            break                           # timeout expired
        # done waiting
        if lrx == 0:
          print("      No response")
          ser.close()
        elif lrx<12:
          print("      Invalid USB response: '{}'".format(rxbuf.hex()))
          ser.close()
        else:           
          # got radio id & mac address.
          cmd_code = rxbuf[4]
          radio_id = rxbuf[5]
          radio_mac = macAddrBytesToString(rxbuf[6:6+6])
          # make radio/team instance and store ref in global dict          
          #print('      id: {:d}, MAC:{:s}'.format(radio_id,radio_mac))
          if (only_this_radio is None) or (only_this_radio == radio_id):
            r = Radio(
              radio_id=radio_id,
              radio_mac=radio_mac,
              ser_dev=p.device,
              ser_handle=ser)
            r.report(2)
          else:
            print('      Found radio_{}, but we were asked to only do radio_{}.'.format(radio_id+1, only_this_radio))
      except Exception as e:
        print("      enumerateRadios(): Exception: '{}'".format(e))
# -----------------------------------------------------------------------------
#
# Radio class  -  Handles USB_UART stuff, one instance per connected radio.
#
# By default, radio_x is connected with client class team_x, but that 
# connection can be paused so a client class Boss can use the uart.
#
# Whichever client has control will get callbacks as packets are sent
# and received.
#
# Also, zero or more client class Watcher can be hooked up, and they also
# get called on pkt tx/rx.
#
# UDP messages have a particular format:
#
# Incoming messages from teams are always b'L3:' + < msg bytes > 
# Outgoing responses to teams are the same format.
# The end-of-udp-packet is treated as the message delimiter, and the
# radio number is implied.
#
# This format is interchanged with a different one when it is going through
# USB to the radio modules:
#
#  [ 0xA5, 0x5_, 0xHH, 0xLL, ... ]
# 
#  - Where _ is the node # (0: LoRa radio, 1: Manager CPU, 2: Flight CPU)
#  - HH & LL are the uint16_t length of payload to follow, and 
#  - ... are optional payload bytes
#
# In the case of types 0 and 1, the above format is the native command 
# set for the node, including the responses, and they go as individual
# UDP packets.
#
# For Flight (Leon3) CPU node 3, the above format is used just thru
# the radio, but that gets stripped off at each end, and a third 
# protocol comes into play.
#
# CCSDS format with sync word:
# ---------------------------
#
#   0xDE 0xAD 0xBE 0xEF  (sync word)  
#   uint16_t  stream_id (big-endian)  
#   uint16_t  seq_count (b.e.)        
#   uint16_t  packet_length (b.e.)    
#   uint8_t   payload[packet_length+1]
#
# Each Radio has a leon_buf_tx and leon_buf_rx for staging complete packets
# before they are dispatched via uart or UDP.
#
# These bufs are also parsed for challenge-related introspection on telemetry
# and commands.
#
# -----------------------------------------------------------------------------
class Radio(threading.Thread):
  radio_id = None          # 0,1,2...9
  radio_mac = None
  ser_dev = None      # e.g. '/dev/ttyUSB0'
  ser_handle = None
  rx_buf = b''
  tx_buf = b''

  running = False           # thread mainloop running

  seizing = False            # true while a boss is seizing the uart
  seized  = False            # true while a boss has the uart
  releasing = False         # true while boss releasing uart back to default

  default_client_ref = None # the default (team) client ref for this radio
  new_client_ref =None      # a boss client class while seizing radio
  client_ref = None         # whatever client class has control of the radio.
  watcher_refs = []         # any number of watcher client classes

  leon_buf_tx = b''         # staging of outbound CCSDS packets
  leon_buf_rx = b''         # staging of inbound CCSDS packets

  trashed_leon_bytes = b''
  trashed_uart_bytes = b''

  # challenge related things:

  bitrate = 1       # for initial comms restoration challenge
  power = 0         # Teams will get short comms windows until they set bitrate 0, power 1 
  spin_degraded_t = random.randrange(0,Z_SPIN_PERIOD_SECS)    # settings common.py
  precession_blackout = True    # comms are currently degraded due to spin
  
  orbital_blackout = True   # comms are currently degraded due to orbit
  orbital_t = 0             # upcounter with rollover each orbit.
  last_secs = 0

  uart_pkt_rx_queue = []  # holds incoming uart packets
  leon_pkt_rx_queue = []  # holds incoming leon packets

  # -----------
  def __init__(self,radio_id,radio_mac,ser_dev,ser_handle):
    global g_radios    
    threading.Thread.__init__(self)    
    self.radio_id = int(radio_id)
    self.radio_mac = radio_mac
    self.ser_dev = ser_dev      # e.g. '/dev/ttyUSB0'
    self.ser_handle = ser_handle
    self.ser_handle.timeout = 0
    self.ser_handle.write_timeout = 0
    self.ser_handle.inter_byte_timeout = 0.001
    g_radios[self.radio_id]=self
  # -----------
  # printout this radio's id, mac address, and device path
  # e.g. "    radio_7: 24:6f:28:77:90:a8 '/dev/ttyUSB0'"
  # -----------  
  def report(self,indent=0):
    print((' '*indent)+"    radio_{:d}: {:s} '{:s}'".format(
      self.radio_id+1,
      self.radio_mac,
      self.ser_dev))
  # -----------
  # print a status message labelled with radio name.
  # todo: put a mutex around this
  # -----------    
  def printMsg(self,msg):
    logMsg('radio_{} {}'.format(self.radio_id+1,str(msg)))
  # -----------    
  # thread mainloop
  # -----------    
  def run(self):
    global g_running

    self.report()
    self.running=True
    time.sleep(0.25)

    while self.running:        
      did_stuff = False

      # if have bytes to send to radio
      out_len = len(self.tx_buf)      
      if out_len:
        did_stuff = True
        usb_tx = deepcopy(self.tx_buf)
        self.tx_buf=b''
        self.ser_handle.write(usb_tx)
        showHex('usb_tx',usb_tx)

      # if received bytes from radio
      in_len = self.ser_handle.in_waiting
      if in_len:                                # have bytes back from radio
        did_stuff = True
        usb_rx = self.ser_handle.read(in_len)
        self.rx_buf += usb_rx
        self.gotBytesFromRadio()
        showHex('usb_rx',usb_rx)        
      
      # if received any full uart packets
      if len(self.uart_pkt_rx_queue):
        did_stuff = True
        uart_pkt = deepcopy(self.uart_pkt_rx_queue[0])
        self.uart_pkt_rx_queue = self.uart_pkt_rx_queue[1:]
        self.gotUartPacketFromRadio(uart_pkt)

      # if received any full leon packets
      if len(self.leon_pkt_rx_queue):
        did_stuff = True
        leon_pkt = deepcopy(self.leon_pkt_rx_queue[0])
        self.leon_pkt_rx_queue = self.leon_pkt_rx_queue[1:]
        self.gotLeonPacketFromRadio(leon_pkt)

      # check if a second has passed
      secs = getUtcTimestampSecs()
      if secs != self.last_secs:
        self.last_secs = secs
        self.perSecondEvents()

      if not did_stuff:         # If we're idle,
        time.sleep(0.01)        # sleep a bit,        
        self.checkBossSeizure() # handle if boss is trying seize/unseize the radio

    # thread loop end. final cleanup.

    self.report()
    self.ser_handle.close()
  # -----------
  # boss can take over a team's radio. We wait until there are no tx/rx bytes
  # in buffer before doing the switch.
  # -----------    
  def checkBossSeizure(self):
    lt = len(self.tx_buf) 
    lr = len(self.rx_buf)
    if (lt == 0) and (lr == 0):
      if self.seizing:          
        self.client_ref.setRadioRef(radio_id=None,radio_ref=None)
        self.client_ref = self.new_client_ref
        self.client_ref.setRadioRef(radio_id=self.radio_id,radio_ref=self)
        self.seizing=False
        self.seized = True
      elif self.releasing:          
        self.client_ref.setRadioRef(radio_id=None,radio_ref=None)  
        self.client_ref = self.default_client_ref
        self.client_ref.setRadioRef(radio_id=self.radio_id,radio_ref=self)            
        self.releasing=False
        self.seized = False    
  # -----------    
  # Called whenever we get bytes from radio. Check if
  # we got any full UART packets (A5 5x ...) and if so, enqueue in
  # a uart packer rx buffer.
  # -----------
  def gotBytesFromRadio(self):
    uart_packet_sz = self.uartPacketLen()
    while uart_packet_sz:  
      # chop latest uart packet from uart rx buf
      # and add to uart packet rx queue.
      uart_pkt = deepcopy(self.rx_buf[0:uart_packet_sz])
      self.rx_buf = self.rx_buf[uart_packet_sz:]
      self.uart_pkt_rx_queue.append(uart_pkt)
      # check for another
      uart_packet_sz = self.uartPacketLen() 
  # -----------  
  # handles incoming uart packats from radio.
  #
  # For non-leon3 messages, we send them as we get them.
  #
  # For leon3 messages, do additional buffering until we get 
  # a full CCSDS packet before putting it in leon rx queue for
  # sending it to client.
  # -----------    
  def gotUartPacketFromRadio(self,uart_pkt):
    
    #print('got uart pkt sz: 0x{:04x}: {}'.format(uart_packet_sz,uart_pkt.hex()))

    if uart_pkt[1]&0x0f == NODE_LEON:
      self.leon_buf_rx+=uart_pkt[4:]
      leon_packet_sz = self.leonPacketLen()
      while leon_packet_sz > 0: # while have full leon packets
        # chop them off and enqueue
        leon_pkt = deepcopy(self.leon_buf_rx[0:leon_packet_sz])
        self.leon_buf_rx = self.leon_buf_rx[leon_packet_sz:]
        self.leon_pkt_rx_queue.append(leon_pkt)
        leon_packet_sz = self.leonPacketLen()
    else:
      # non-leon3 message. send as-is to client and watchers.
      if (not self.client_ref is None):
        self.client_ref.rxComplete(radio_id=self.radio_id, uart_pkt=uart_pkt)
      for watcher in self.watcher_refs:
        watcher.rxComplete(radio_id=self.radio_id, uart_pkt=uart_pkt,client_name=self.client_ref.client_name,as_watcher=True)
  # -----------    
  # received a leon packet. pass to controlling client and watchers.
  # -----------    
  def gotLeonPacketFromRadio(self, leon_pkt):
    if not self.client_ref is None:
      if not (self.precession_blackout or (self.orbital_blackout and DO_ORBITAL_BLACKOUT)):   # settings_common.py
        self.client_ref.rxComplete(radio_id=self.radio_id, leon_pkt=leon_pkt)
    for watcher in self.watcher_refs:
      watcher.rxComplete(radio_id=self.radio_id, leon_pkt=leon_pkt,client_name=self.client_ref.client_name,as_watcher=True)

  # -----------    
  # things updated each ~ second:
  #
  # 1. Orbital comms window
  #
  # 2. Initially, the flatsat is precessing and causing comms
  # to go out for some seconds each precession period.
  # The team needs to decrease comms 'bitrate' and increase 
  # comms 'power'.  These don't really do anything except
  # cause us to turn off the blackouts.
  # -----------      
  def perSecondEvents(self):

    # do precession stuff

    if self.power and not self.bitrate:   # if they solved challenge 1,
      if self.precession_blackout:                # just now,
        self.printMsg('precession blackout END')  # print status msg,
        self.precession_blackout = False          # end comms disruptions.

    self.spin_degraded_t += 1                       # precession timer
    if self.spin_degraded_t >= Z_SPIN_PERIOD_SECS:  # rollover val in settings_common.py
      self.spin_degraded_t = 0                      
    if self.spin_degraded_t < Z_SPIN_COMMS_WINDOW_SECS:  # if in precession comms window
      if self.precession_blackout:
        self.printMsg('precession blackout END')
        self.precession_blackout = False            
    else:                                     # In precession blackout.
      if (not self.power) or self.bitrate:    # (unless they solved challenge 1)
        if not self.precession_blackout:
          self.printMsg('precession blackout START')
          self.precession_blackout = True

    # do orbital comms window stuff

    self.orbital_t += 1
    if self.orbital_t >= ORBIT_PERIOD_SECS:   # rollover val, settings_common.py
      self.orbital_t = 0
    if self.orbital_t < ORBIT_COMMS_WINDOW_SECS:   # comms seconds per orbital period
      if self.orbital_blackout and DO_ORBITAL_BLACKOUT:
        m='orbital blackout END'
        self.printMsg(m)
        logInfo('team_{} : '.format(self.radio_id+1)+m)
        self.orbital_blackout = False            
    else:
      if (not self.orbital_blackout) and DO_ORBITAL_BLACKOUT:
        m='orbital blackout START'
        self.printMsg(m)
        logInfo('team_{} : '.format(self.radio_id+1)+m)
        self.orbital_blackout = True
    
    # report secs until next orbital comms window
    # to the team and watchers.
    countdown = int(self.orbital_t)
    if not self.client_ref is None:
      self.client_ref.commsWindowStatus(countdown)
    for watcher in self.watcher_refs:
      watcher.commsWindowStatus(countdown)

  # -----------
  # A team reporting for duty. (passes a ref to itself)
  # -----------
  def setDefaultClientRef(self, client_ref):
    self.default_client_ref = client_ref
    self.client_ref = client_ref
    # we pass the client a ref to self too.
    self.client_ref.setRadioRef(self.radio_id,self)
  # -----------
  # A boss client wants this uart
  # -----------
  def seizeUart(self, boss_ref):
    if self.seized or self.seizing:   # already taken.
      if boss_ref == self.new_client_ref: # If taken by this boss,
        return True                   # consider it a success,
      return False                    # else fail.
    self.new_client_ref = boss_ref
    self.seizing = True
    return True    
  # -----------
  # A boss client is done with this uart
  # -----------  
  def unseizeUart(self):
    if not self.seized:   # not taken
      return True    # count as success
    self.releasing = True
    return True
  # -----------
  # Add bytes to the tx-to-radio buffer. 
  # (unless we're a team and we're in a comms blackout.)
  # -----------  
  def sendMessage(self,uart_pkt,as_team=False):
    if as_team:
      if self.orbital_blackout and DO_ORBITAL_BLACKOUT:
        return
      if self.precession_blackout:
        return
    self.tx_buf+=uart_pkt
  # -----------
  # Adding a watcher to this radio
  # -----------  
  def addWatcherRef(self, watcher_ref):
    self.watcher_refs.append(watcher_ref)
  # -----------
  # returns packet length if theres a whole uart frame in buffer, else return 0
  # -----------
  def uartPacketLen(self):
    # If usb rx buf doesn't begin with valid header (A5 5X),
    # trash bytes until either find one or run out of bytes.
    lb = len(self.rx_buf)
    while (lb>=2) and ((self.rx_buf[0]!=0xa5) or ((self.rx_buf[1]&0xf0) != 0x50)):
      self.trashed_uart_bytes+=bytes([self.rx_buf[0]])
      self.rx_buf=self.rx_buf[1:]
      lb-=1
    if (lb>2) and len(self.trashed_uart_bytes):
      if lb:
        d='but'
        n=''
      else:
        d='and'
        n="n't"
      msg='radio_{} WARN: Had to trash {} uart_rx bytes ({}), {} we did{} find a valid header'.format(
              self.radio_id+1,
              len(self.trashed_uart_bytes),
              self.trashed_uart_bytes.hex(),
              d,n)
      errBox(msg)
      self.trashed_uart_bytes = b''
    # check if have minimum bytecount needed to be a packet
    if lb<5:
      return 0
    # parse payload length
    pl = (self.rx_buf[2]<<8) | self.rx_buf[3]   
    # sanity check
    if pl>2048:
      msg = 'radio_{} WARN: uart_rx payload length {} sanity check fail'.format(self.radio_id+1,pl)
      errBox(msg)
      self.rx_buf=self.rx_buf[1:]
      return 0
    # have enough bytes and valid header?
    if lb>=pl+4:
      return pl+4   # return full packet length
    return 0
  # -----------
  # If there is a full CCSDS packet, it's length is returned, else 0.
  # 
  # CCSDS Format:
  #
  # sync_word:  de ad be ef +
  # uint16_t    stream_id (big endian) + 
  # uint16_t    sequence_count (b.e.) + 
  # uint16_t    packet_length (b.e.) + 
  # uint8_t     data[packet_length + 1 ]
  # -----------  
  def leonPacketLen(self):
    while 1:
      if len(self.leon_buf_rx)<1:
        return 0
      if self.leon_buf_rx[0]!=0xDE:
        # out of sync. trash bytes until back in sync
        self.trashed_leon_bytes+=bytes([self.leon_buf_rx[0]])
        self.leon_buf_rx = self.leon_buf_rx[1:]
        continue
      if len(self.leon_buf_rx)<2:
        return 0
      if self.leon_buf_rx[1]!=0xAD:
        self.trashed_leon_bytes+=bytes([self.leon_buf_rx[0]])
        self.leon_buf_rx = self.leon_buf_rx[1:]
        continue
      if len(self.leon_buf_rx)<3:
        return 0
      if self.leon_buf_rx[2]!=0xBE:
        self.trashed_leon_bytes+=bytes([self.leon_buf_rx[0]])
        self.leon_buf_rx = self.leon_buf_rx[1:]
        continue
      if len(self.leon_buf_rx)<4:
        return 0
      if self.leon_buf_rx[3]!=0xEF:
        self.trashed_leon_bytes+=bytes([self.leon_buf_rx[0]])
        self.leon_buf_rx = self.leon_buf_rx[1:]
        continue
      if len(self.leon_buf_rx)<10:
        return 0
      stream_id =     (self.leon_buf_rx[4]<<8)+self.leon_buf_rx[5]
      seq_ct =        (self.leon_buf_rx[6]<<8)+self.leon_buf_rx[7]
      payload_len =   (self.leon_buf_rx[8]<<8)+self.leon_buf_rx[9]      
      if payload_len > (1024+1024):
        msg = 'radio_{} WARN: CCSDS payload length {} sanity check fail'.format(self.radio_id+1,payload_len)
        errBox(msg)
        self.trashed_leon_bytes+=bytes([self.leon_buf_rx[0]])          
        self.leon_buf_rx = self.leon_buf_rx[1:]
        continue
      # got deadbeef at start of buf and a sane packet len
      if len(self.trashed_leon_bytes):
        msg='radio_{} WARN: CCSDS {} unhandled byte(s): {}>'.format(
              self.radio_id+1,
              len(self.trashed_leon_bytes),
              self.trashed_leon_bytes.hex())
        errBox(msg)        
        self.trashed_leon_bytes=b''
      if len(self.leon_buf_rx)>=payload_len+11:
        if DEBUG:
          payload_hex = self.leon_buf_rx[10:10+payload_len+1].hex()
          print("                  <CCSDS: PACKET (strm 0x{:04x} seq 0x{:04x} payload[{:d}] {:s})>".format(
                      stream_id, seq_ct, payload_len, payload_hex))
        return payload_len+11
      return 0      
    return 0
  # -----------  
  def leonPacketLenTx(self):
    while 1:
      if len(self.leon_buf_tx)<1:
        return 0
      if self.leon_buf_tx[0]!=0xDE:
        # out of sync. trash bytes until back in sync
        self.trashed_leon_bytes+=bytes([self.leon_buf_tx[0]])
        self.leon_buf_tx = self.leon_buf_tx[1:]
        continue
      if len(self.leon_buf_tx)<2:
        return 0
      if self.leon_buf_tx[1]!=0xAD:
        self.trashed_leon_bytes+=bytes([self.leon_buf_tx[0]])
        self.leon_buf_tx = self.leon_buf_tx[1:]
        continue
      if len(self.leon_buf_tx)<3:
        return 0
      if self.leon_buf_tx[2]!=0xBE:
        self.trashed_leon_bytes+=bytes([self.leon_buf_tx[0]])
        self.leon_buf_tx = self.leon_buf_tx[1:]
        continue
      if len(self.leon_buf_tx)<4:
        return 0
      if self.leon_buf_tx[3]!=0xEF:
        self.trashed_leon_bytes+=bytes([self.leon_buf_tx[0]])
        self.leon_buf_tx = self.leon_buf_tx[1:]
        continue
      if len(self.leon_buf_tx)<10:
        return 0
      stream_id =     (self.leon_buf_tx[4]<<8)+self.leon_buf_tx[5]
      seq_ct =        (self.leon_buf_tx[6]<<8)+self.leon_buf_tx[7]
      payload_len =   (self.leon_buf_tx[8]<<8)+self.leon_buf_tx[9]
      if payload_len > (1024+1024):
        print('                  <CCSDS WARN: payload_length {} fails sanity check>'.format(payload_len))
        self.trashed_leon_bytes+=bytes([self.leon_buf_tx[0]])          
        self.leon_buf_tx = self.leon_buf_tx[1:]
        continue
      # got deadbeef at start of buf and a sane packet len
      if len(self.trashed_leon_bytes):
        print('                  <CCSDS WARN: {} unhandled byte(s): {}>'.format(
              len(self.trashed_leon_bytes),
              self.trashed_leon_bytes.hex()))
        self.trashed_leon_bytes=b''
      if len(self.leon_buf_tx)>=payload_len+11:
        if DEBUG:
          payload_hex = self.leon_buf_tx[10:10+payload_len+1].hex()
          print("                  <CCSDS: PACKET (strm 0x{:04x} seq 0x{:04x} payload[{:d}] {:s})>".format(
                      stream_id, seq_ct, payload_len, payload_hex))
        return payload_len+11
      return 0      
    return 0    
  # -----------  
    '''
      team_0:  Tx: 'L3:deadbeef09e3dc800091000f62ced0db00000000323032302d30362d32342d30... (124 more bytes)'
                  <usb_rx: d0fffe10405345b4a1020a1cc09297f2c0c9799040baa552001ee00b7d4c26e8... (56 bytes)>
                  <usb_rx: a5520025deadbeef0805c71d0025000f62d000cf000033e0ffff002000002090... (270 bytes)>
                  <CCSDS WARN: 292 unhandled byte(s): 00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000041c34247907ffe1041c34255d0fffe10405345b4a1020a1cc09297f2c0c900000000000000000000000000000000000000000000000000000000000000000000000000000041c34247907ffe1041c34255d0fffe10405345b4a1020a1cc09297f2c0c9799040bae00b7d4c26e84002a4ff0a5f12d4c01c46f3877201d9bff3bc8600000000000000000000000000000000000000000000000000000000000000000000000000000041c34247907ffe1041c34255d0fffe10405345b4a1020a1cc09297f2c0c9799040bae00b7d4c26e84002a4ff0a5f12d4c01c46f3877201d9bff3bc869a9d38ad>
                  <CCSDS WARN: 103 unhandled byte(s): 00000000000000000000000000000000000000000000000000000000000000000000000000000041c34247907ffe1041c34255d0fffe10405345b4a1020a1cc09297f2c0c9799040bae00b7d4c26e84002a4ff0a5f12d4c01c46f3877201d9bff3bc869a9d38ad>
                  <CCSDS: PACKET (strm 0x0805 seq 0xc71d len 37 payload 000f62d000cf000033e0ffff00200000209000cb3000000f...)>
    team_0:  Tx: 'L3:0000000000000000000000000000000000000000000000000000000000000000... (16 more bytes)'
                  <CCSDS WARN: 55 unhandled byte(s): c34255d0fffe10405345b4a1020a1cc09297f2c0c9799040bae00b7d4c26e84002a4ff0a5f12d4c01c46f3877201d9bff3bc869a9d38ad>
                  <CCSDS: PACKET (strm 0x0805 seq 0xc71d len 37 payload 000f62d000cf000033e0ffff00200000209000cb3000000f...)>

    '''