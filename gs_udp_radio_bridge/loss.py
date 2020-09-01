#!/usr/bin/env python3
# -----------------------------------------------------------------------------
#
# loss.py
#
# Enumerates the USB-connected radios and outputs continuous packet-loss
# percentages for them.
#
# -----------------------------------------------------------------------------
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
import struct
from copy import deepcopy
import math
# -----------------------------------------------------------------------------
from settings_common  import *                # TEAM IPs and Ports
from defines          import *
from helpers          import *
# -----------------------------------------------------------------------------
# Globals
# -----------------------------------------------------------------------------
g_running = True
g_radios = {}
# -----------------------------------------------------------------------------
# For each USB UART that has 'CP2102' in its description, try hailing a radio.
# If we get back the expected response (team # and mac address) then set up a 
# Radio class for it and add to global dict of connected radios.
# -----------------------------------------------------------------------------
def enumerateRadios(callback_ref = None):
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
          r = Radio(
            radio_id=radio_id,
            radio_mac=radio_mac,
            ser_dev=p.device,
            ser_handle=ser,
            callback_ref = callback_ref)
          r.report(2)
      except Exception as e:
        print("      enumerateRadios(): Exception: '{}'".format(e))
# -----------------------------------------------------------------------------
#
# Radio class  -  Handles USB_UART stuff, one instance per connected radio.
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

  last_secs = 0

  trashed_uart_bytes = b''

  uart_pkt_rx_queue = []  # holds incoming uart packets
  callback_ref = None
  # -----------
  def __init__(self,radio_id,radio_mac,ser_dev,ser_handle,callback_ref):
    global g_radios    
    threading.Thread.__init__(self)    
    self.radio_id = int(radio_id)
    self.radio_mac = radio_mac
    self.ser_dev = ser_dev      # e.g. '/dev/ttyUSB0'
    self.ser_handle = ser_handle
    self.callback_ref = callback_ref
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

      # check if a second has passed
      secs = getUtcTimestampSecs()
      if secs != self.last_secs:
        self.last_secs = secs
        self.perSecondEvents()

      if not did_stuff:         # If we're idle,
        time.sleep(0.01)        # sleep a bit,        

    # thread loop end. final cleanup.

    self.report()
    self.ser_handle.close()
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
  # -----------    
  def gotUartPacketFromRadio(self,uart_pkt):
    
    #print('got uart pkt sz: 0x{:04x}: {}'.format(uart_packet_sz,uart_pkt.hex()))

    if uart_pkt[1]&0x0f == NODE_LEON:
      pass 
    else:
      # non-leon3 message. send as-is to client and watchers.
      self.callback_ref(radio_id=self.radio_id,uart_pkt=uart_pkt)

  # -----------    
  # things updated each ~ second
  # -----------      
  def perSecondEvents(self):
    pass

  # -----------
  # Add bytes to the tx-to-radio buffer. 
  # (unless we're a team and we're in a comms blackout.)
  # -----------  
  def sendMessage(self,tag,cmd,payload=None):
    if tag == 'RA':
      dest_nyb = NODE_RADIO
    elif tag == 'OM':
      dest_nyb = NODE_OOBM
    else:
      print('FATAL: Unhandled msg tag {}.'.format(tag))
      g_running = False
      return

    data = cmd
    if not payload is None:
      data+=payload
    lp = len(data)
    H = (lp>>8)&0xff
    L = lp&0xff

    uart_pkt = bytes([0xa5, 0x50|dest_nyb, H, L]) 
    uart_pkt += data
    self.tx_buf+=uart_pkt
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
# -----------------------------------------------------------------------------
# Functions
# -----------------------------------------------------------------------------
def handleCtrlC(signal_received, frame):
  global g_running
  g_running = False
# -----------------------------------------------------------------------------
g_update = True
g_loss_arr = [0,0,0,0,0,0,0,0,0]
g_time_last_update = 0
g_num_responses = 0
g_num_radios = 0
# -----------------------------------------------------------------------------
def radioCallback(radio_id,uart_pkt):
  global g_num_responses
  #print ('got callback for radio_{}'.format(radio_id+1))
  if (uart_pkt[1]&0x0f) == NODE_RADIO:
    plen = struct.unpack('>H',uart_pkt[2:4])[0]
    cmd  = uart_pkt[4]
    if cmd == 3:
      loss  = struct.unpack('<f',uart_pkt[5:9])[0]
      if math.isnan(loss):
        loss = 0
      # python makes list operations atomic
      g_loss_arr[radio_id] = loss
      g_num_responses += 1

# -----------------------------------------------------------------------------      
def showLoss():
  global g_radios,g_loss_arr
  s=''
  for t in g_radios:
    r=g_radios[t]
    q='{}: {:3.4f}  '.format(t+1,g_loss_arr[t])
    s+='{:13s}'.format(q)
  print(s,end='\r')
# -----------------------------------------------------------------------------      
def main():
  global g_update, g_time_last_update, g_num_responses, g_num_radios

  try:
    nv = os.nice(0) 
    if nv == 0:
      nv = os.nice(-1000) 
      print("  INFO: os.nice set to: {}\n".format(nv))
  except:
    print("  WARN: Couldn't decrease process nice value.\n"+\
          "        If needed, try running as root.\a\a\n")

  enumerateRadios(radioCallback)

  g_num_radios = len(g_radios)
  if not g_num_radios:
    print('\n  *** No radios found. Sry.')
  else:
    # I dont always do stuff like this, I swear:
    if g_num_radios == 1:
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

    # get ready to go into mainloop
    signal(SIGINT, handleCtrlC)
    m='{} Team{} {} ready to rock.  <Ctrl-C Quit>'.format(g_num_radios,s,a)
    boxPrint('-',m,2)

    # mainloop
    while g_running:
      t = getUtcTimestampSecs()
      dt = t - g_time_last_update      
      if (dt > 20) or (g_num_responses==g_num_radios):
        g_time_last_update = t
        g_num_responses = 0
        g_update = True

      if g_update:
        g_update=False
        showLoss()
        for t in g_radios:
          r=g_radios[t]
          r.sendMessage('RA',CMD_PKT_LOSS)

      time.sleep(0.25)

    # cleanup radios

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
