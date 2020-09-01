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
from settings_common  import *                # TEAM IPs and Ports
from radio            import Radio, g_radios
from defines          import *
from helpers          import *
from sat_cmds         import *
# -----------------------------------------------------------------------------
# Globals
# -----------------------------------------------------------------------------
g_clients = []
g_teams = []
g_watchers = []
g_bosses = []

g_num_clients = 0
g_num_teams = 0
g_num_watchers = 0
g_num_bosses = 0

g_radio_bits = 0 # bit 0 set: radio 0 present. bit 1 set: radio 1 present...

# -----------------------------------------------------------------------------
class Client(threading.Thread):
  client_id = None  # 0..9: team_0 ... team_9, 10..19: watchers, 20+: bosses
  type_str = ''     # 'boss', 'team', 'watcher'
  client_name = ''  # e.g. 'team_0', 'boss_20', etc.
  ip_addr = None    # client's ip address as assigned in settings_common.py
  bridge_rx = None  # my listener port
  bridge_tx = None  # remote listener port
  rx_sock = None    # corresponding sockets
  tx_sock = None  
  running = False

  radio_id  = None  # 0...9 if we control a radio, else none.
  radio_ref = None  # Radio class reference, if we control one. Else None
  boss_radio_ref = None # boss radio class reference, whether we have seized it or not.
  default_radio_ref = None  # team clients remember their radios as a convoluted way to get to 
                            # radio X from this source file.
  watching = None  # ref to radio if we are in watcher mode

  udp_tx_queue = [] 
  # -----------
  def __init__(self, client_id = None, 
      client_name = None, ip_addr=None,
      bridge_rx=None,bridge_tx=None,
      bridge_ip=None):
    global g_clients, g_num_clients
    threading.Thread.__init__(self)
    self.client_id = client_id
    self.client_name = client_name
    self.ip_addr = ip_addr
    self.bridge_rx = bridge_rx
    self.bridge_tx = bridge_tx
    self.bridge_ip = bridge_ip
    g_clients.append(self)
    g_num_clients+=1
  # -----------

  def report(self):
    ctrlmsg = ''
    if self.type_str in ['team','boss','watcher']:
      if self.radio_id is not None:
        ctrlmsg = '(radio_{})'.format(self.radio_id+1)
      else:
        ctrlmsg = '(No radio)'
    print("{:>10s} {:<15s} {:<7d} {:<7d} {}".format(self.client_name, self.ip_addr, self.bridge_rx, self.bridge_tx,ctrlmsg))    
  # -----------
  # thread main
  # -----------
  def run(self):
    global g_running
    # setup client sockets
    self.tx_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    self.rx_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    fcntl.fcntl(self.rx_sock, fcntl.F_SETFL, os.O_NONBLOCK)

    self.rx_sock.bind((self.bridge_ip, self.bridge_rx))

    # thread mainloop
    self.running=True    
    time.sleep(0.25)
    while self.running:  
      did_stuff = False

      # check for UDP command from client
      try:
        paused = (self.type_str == 'team') and (self.radio_ref == None)
        if not paused:
          udp_msg, org    = self.rx_sock.recvfrom(4096)
          src_ip,src_port = org
          did_stuff = True
          self.handleUdpMsg(udp_msg=udp_msg,src_ip=src_ip,src_port=src_port)
      except socket.error as e:
        err = e.args[0]
        if err == errno.EAGAIN or err == errno.EWOULDBLOCK:
          pass
        else:
          errBox("{}: Socket exception: '{}'".format(self.client_name,e))
          #g_running = False
      except Exception as e:
        errBox("{}: thread exception: '{}'".format(self.client_name,str(e)))

      # check if we have any UDP's to send.
      if len(self.udp_tx_queue):
        did_stuff = True
        tag,msg = self.udp_tx_queue[0]
        self.udp_tx_queue = self.udp_tx_queue[1:]
        self.sendUdp(tag,msg)

      if not did_stuff:
        time.sleep(0.01)

    # cleaning up
    self.report()
    self.rx_sock.close()
    self.tx_sock.close() 
  # -----------    
  # todo: put a mutex around this
  # ----------- 
  def printMsg(self,msg,indent=0):
    m = '{}: {}'.format(self.client_name,str(msg))
    logMsg(m, bell = False)
  # -----------
  # This client got a udp message in the bridge. Check orginating IP address,
  # separate tag from data, and delegate for proper client type.
  # -----------  
  def handleUdpMsg(self,udp_msg,src_ip,src_port):   
    # require correct IP address
    if src_ip != self.ip_addr:
      m="{}: Bad IP! (expected '{}', got '{}')".format(
        self.client_name, self.ip_addr, src_ip)
      errBox(m)
      logInfo(m)
      return
    # parse msg tag   
    msg_sz = len(udp_msg)
    if msg_sz<3:
      errBox("{}: WARN: handleUdpMsg(): Couldn't parse tag. Msg too short.".format(self.client_name))      
      return
    colon_at = -1
    for i in range(0,4):
      if chr(udp_msg[i])==':':
        colon_at = i
        break
    if colon_at<0:
      errBox("{}: WARN: handleUdpMsg(): Couldn't parse tag. Missing colon.".format(self.client_name))
      return

    # get tag and data
    tag  = udp_msg[0:colon_at].decode('utf-8').upper()
    data = udp_msg[colon_at+1:]
    if DEBUG:
      if len(data)>32:
        self.printMsg("Rx : '{}:{}... ({} more bytes)'".format(tag,data[0:32].hex(),len(data)-32))
      else:
        self.printMsg("Rx : '{}:{}'".format(tag,data.hex()))

    if self.type_str in ['team', 'watcher']:
      if tag == 'RSC':
        # handle uint8_t comms_bitrate and uint8_t comms_power
        # for the initial comms restoration challenge
        if len(data)==2:
          r = self.default_radio_ref
          r.bitrate = data[0]
          r.power   = data[1]
          if (not data[0]) and data[1]:
            logCrit('{} solved RADIO BRINGUP challenge!'.format(self.client_name))
          self.printMsg('RSC command. bitrate: {}  power: {}'.format(r.bitrate, r.power))
          self.sendUdp(tag,data)
          # cc all watchers
          if self.radio_ref is not None:
            for watcher in self.radio_ref.watcher_refs:
              watcher.sendUdp(tag,data)
        else:
          errBox("{}: Sent RSC command without two byte of args. payload: '{}' !!".format(self.client_name,data.hex()))
      else:
        if tag != 'L3':             
          m="{}: Bad message tag: '{}' !!".format(self.client_name,tag)
          errBox(m)
          logInfo(m)
          return
        # handle L3 udp msg from team
        if self.type_str == 'team':
          self.teamParseUdp(tag, data)
        else:
          self.watcherParseUdp(tag, data)
    elif self.type_str == 'boss':     # UDP from Boss:
      self.bossParseUdp(tag, data)
    else:
      errBox("{}: Unknown client type: '{}'".format(self.client_name,self.type_str))
    return  
  # -----------
  # If we get control of a radio, it sends us its number and class reference
  # -----------  
  def setRadioRef(self,radio_id,radio_ref):
    if DEBUG:
      if radio_id is None:      
        print('                  <{} lost radio_{}>'.format(self.client_name,self.radio_id+1))
      else:
        print('                  <{} has radio_{}>'.format(self.client_name,radio_id+1))
    self.radio_id = radio_id
    self.radio_ref = radio_ref
    if (self.type_str == 'team') and (self.default_radio_ref is None):
      self.default_radio_ref = radio_ref
  # -----------
  # send response to remote client  
  # -----------
  def sendUdp(self,tag,msg):
    if self.type_str != 'watcher':
      if DEBUG:
        if len(msg)>32:
          self.printMsg(" Tx: '{}:{}... ({} more bytes)'".format(tag,msg[0:32].hex(),len(msg)-32))          
        else:
          self.printMsg(" Tx: '{}:{}'".format(tag,msg.hex()))

    msg = tag.encode('utf-8')+b':'+msg
    self.tx_sock.sendto(msg, (self.ip_addr, self.bridge_tx))  
  # -----------
  # called by radio each second to report countdown till
  # the next comms window.  (or 0 if in said window)
  # -----------
  def commsWindowStatus(self, timer_val):
    # steve wants it reported even if we're not doing them
    #if not DO_ORBITAL_BLACKOUT:
    #  return
    ret_val = 0
    if timer_val >=  ORBIT_COMMS_WINDOW_SECS:  # if in blackout
      bw = (ORBIT_PERIOD_SECS - ORBIT_COMMS_WINDOW_SECS) # size of blackout period in secs
      w = (timer_val - ORBIT_COMMS_WINDOW_SECS)   # how far into that period we are
      ret_val = bw - w
    # big-endiun uin616_t
    #print('NEXT COMMS WINDOW IN {}s'.format(ret_val))
    ret_bytes = struct.pack('>H',int(ret_val))
    self.udp_tx_queue.append(('WIN',ret_bytes))
  # -----------  
  # callback from radio - received a complete uart or leon packet. 
  # Called once per packet.
  # -----------    
  def rxComplete( self,
                  radio_id=None,
                  uart_pkt=None,
                  leon_pkt=None, 
                  client_name=None, 
                  as_watcher=False):

    if uart_pkt is None:
      tag='L3'
      msg = leon_pkt
    else:
      if as_watcher or (self.type_str == 'watcher'):  
        return # watchers only get L3 messages
      src_node = uart_pkt[1]&0x0f
      if src_node == NODE_RADIO:
        tag='RA'
      elif src_node == NODE_OOBM:
        tag='OM'
      msg = uart_pkt[4:]

    self.udp_tx_queue.append((tag, msg))

  # -----------
  def isValidRadioID(self,i):
    global g_radio_bits
    fmsg = "{} wants radio_{}, but it's not ".format(self.client_name,i)
    if not isinstance(i,int) or (i<0) or (i>9):
      fmsg += 'valid.'
      errBox(fmsg)
      return False
    if g_radio_bits&(1<<i):
      return True
    fmsg += "connected."
    errBox(fmsg)
    return False
# -----------------------------------------------------------------------------
class Team(Client):
  # -----------
  def __init__(self, client_id=None):
    global g_num_teams, g_teams, g_radio_bits
    
    # make a note of which radios we found. A boss can query this value.
    g_radio_bits |= (1<<client_id)

    self.type_str = 'team'
    Client.__init__(
      self, 
      client_id=client_id,
      client_name='team_'+str(client_id+1),
      ip_addr=TEAMS[client_id][0],         # use network info from settings_common.py
      bridge_rx=TEAMS[client_id][3],
      bridge_tx=TEAMS[client_id][1],
      bridge_ip=TEAMS[client_id][2]
      )
    g_teams.append(self)
    g_num_teams+=1
  # -----------
  def teamParseUdp(self,tag,data):
    r=self.radio_ref    
    # not sure if this needs to be staged in radio,
    # but received packets are there staged to leon_buf_rx, so .
    r.leon_buf_tx = deepcopy(data)
    if (not r is None) and (not r.seized):
      # sanity check leon packet (its length being < 2048, anyway.)
      lpl = r.leonPacketLenTx()
      if lpl:
        l = len(data)
        H = (l>>8)&0xff
        L = l&0xff
        uart_pkt = bytes([0xa5, 0x50|NODE_LEON, H, L]) 
        uart_pkt += data        
        r.sendMessage(uart_pkt,as_team=True)  # send 

        sid_h = data[4]
        sid_l = data[5]
        stream_id = (sid_h<<8)|sid_l
        cmd_name = getSatCmdName(stream_id)

        m = "{}: Sent '{}'".format(self.client_name,cmd_name)
        logMsg(m)
        logInfo(m)
      else:
        m = '{}: Sent UDP with invalid CCSDS packet!'.format(self.client_name)
        errBox(m)
        logInfo(m)
    else:
      # supposedly my udp listener is paused.
      errBox('{}: Sent UDP while paused, and it got through!'.format(self.client_name))
      pass

  # -----------
# -----------------------------------------------------------------------------
class Watcher(Client):
  # -----------
  def __init__(self,ip_addr=None,bridge_rx=None,bridge_tx=None,radio_id=None,radio_ref=None,bridge_ip=None):
    global g_num_watchers, g_watchers
    self.type_str = 'watcher'
    if not radio_id is None:
      self.radio_id = radio_id
    self.radio_ref = radio_ref
    self.default_radio_ref = radio_ref
    Client.__init__(
      self,
      client_id = g_num_watchers+10,
      client_name = 'watcher_{}'.format(radio_id+1),      
      ip_addr=ip_addr,
      bridge_rx=bridge_rx,
      bridge_tx=bridge_tx,
      bridge_ip=bridge_ip
      )

    g_watchers.append(self)
    g_num_watchers+=1
  # -----------
  def watcherParseUdp(self,tag,data):
    r=self.radio_ref    
    # not sure if this needs to be staged in radio,
    # but received packets are there staged to leon_buf_rx, so .
    r.leon_buf_tx = deepcopy(data)
    if (not r is None): # and (not r.seized):
      # sanity check leon packet (its length being < 2048, anyway.)
      lpl = r.leonPacketLenTx()
      if lpl:
        l = len(data)
        H = (l>>8)&0xff
        L = l&0xff
        uart_pkt = bytes([0xa5, 0x50|NODE_LEON, H, L]) 
        uart_pkt += data        
        r.sendMessage(uart_pkt,as_team=False)  # send 
      else:
        errBox('{}: Sent UDP with invalid CCSDS packet!'.format(self.client_name))
    else:
      # supposedly my udp listener is paused.
      errBox('{}: Sent UDP while paused, and it got through!'.format(self.client_name))
      pass  
# -----------------------------------------------------------------------------
class Boss(Client):
  # -----------
  def __init__(self,ip_addr=None,bridge_rx=None,bridge_tx=None,boss_radio_ref=None,radio_id=None,bridge_ip=None):
    global g_num_bosses,g_bosses
    self.type_str = 'boss'
    self.boss_radio_ref = boss_radio_ref
    self.radio_id = radio_id
    Client.__init__(
      self,
      client_id = g_num_bosses+20,
      client_name = 'boss_{}'.format(radio_id+1),
      ip_addr=ip_addr,
      bridge_rx=bridge_rx,
      bridge_tx=bridge_tx,
      bridge_ip=bridge_ip      
      )
    g_bosses.append(self)
    g_num_bosses+=1
  # -----------
  def haveRadio(self):
    if self.boss_radio_ref is None:
      return False
    if self.boss_radio_ref.client_ref == self:
      return True

    return False
  # -----------  
  def bossParseUdp(self,tag,data):
    
    global g_radio_bits
    
    l=0   # get data len
    try:
      l = len(data)
    except:
      return  # no data

    if tag=='BR':               # remote client is addressing bridge:

      if l==0:                  # all bridge cmds have arguments
        return      
      c0 = None # get first ascii byte of bridge command as unicode
      try:
        c0 = chr(data[0])
      except:
        return  # invalid ascii character

      if c0=='Q':    # b'BR:Q' query for connected radios: returns occupancy map
        HH = (g_radio_bits>>8)&0xff
        LL = g_radio_bits&0xff
        self.sendUdp(tag,b'Q'+bytes([HH,LL]))
      elif c0 in ['S','U']:   #  Sieze or Unseize team's radio
        if l!=1:
          return                # incomplete argument

        if c0=='S':   # b'BR:S' : seize radio
          res = self.boss_radio_ref.seizeUart(self)
          if not res:  # seizure failed
            #errBox('  radio_{} is denying seizure.'.format(radio_id+1))
            self.sendUdp(tag,data+CODE_FAIL)
            return
          else:            
            while not self.boss_radio_ref.seized: # wait for seizure to happen
              pass
            self.sendUdp(tag,data+CODE_PASS)
            return
        elif c0=='U':                   # b'BR:U' : unseize radio
          res = self.boss_radio_ref.unseizeUart()
          if not res:  # seizure failed
            #errBox('  radio_{} is denying unseizure.'.format(radio_id+1))
            self.sendUdp(tag,data+CODE_FAIL)
            return
          else:
            #errBox('  radio_{} is allowing unseizure.'.format(radio_id+1))
            # wait for un seizure to happen
            while self.boss_radio_ref.releasing:
              pass
            self.sendUdp(tag,data+CODE_PASS)
            return

      else:
        errBox('{}: Sent invalid cmd to Bridge.'.format(self.client_name))                     
        return                  # unknown bridge command        
    else:                       # remote client addressing RA, OM, or L3      
      if tag not in ['RA','OM','L3']:
        errBox("{}: Sent unknown msg tag: '{:s}'".format(self.client_name,tag))
        return

      # make sure we control this radio
      if not self.haveRadio():
        errBox("{}: Can't use radio until it's seized.".format(self.client_name))
        if tag!= 'L3':
          self.sendUdp("BR",data+CODE_FAIL)
        return

      # make uart packet
      dest_node = NODE_OOBM
      if tag=='RA':
        dest_node = NODE_RADIO
      elif tag=='L3':
        dest_node = NODE_LEON
      pl = len(data)
      HH = (pl>>8)&0xff
      LL = pl&0xff
      uart_pkt = bytes([0xA5, 0x50|dest_node, HH, LL])
      uart_pkt+=data 
      # stick packet in outgoing buffer of radio_x      
      self.boss_radio_ref.sendMessage(uart_pkt)
      return
  # -----------
