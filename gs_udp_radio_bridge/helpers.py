# -----------------------------------------------------------------------------
# Helper functions for bridge.py, test_bridge.py, observer.py, test_team.py
# -----------------------------------------------------------------------------
from settings_common import *
from defines import *
from datetime import timezone 
from datetime import datetime
import syslog 
# ----------------------------------------------------------------------------- 
def logCrit(s):
  syslog.syslog(syslog.LOG_CRIT, "CRITICAL "+s)
# ----------------------------------------------------------------------------- 
def logInfo(s):
  syslog.syslog(syslog.LOG_INFO, "INFO "+s)
# ----------------------------------------------------------------------------- 
def getUtcTimestampSecs():
  dt = datetime.now()   
  utc_time = dt.replace(tzinfo = timezone.utc) 
  utc_timestamp = utc_time.timestamp()   
  return int(utc_timestamp)
# ----------------------------------------------------------------------------- 
def getUtcTimestampMicros():
  dt = datetime.now()   
  utc_time = dt.replace(tzinfo = timezone.utc) 
  utc_timestamp = utc_time.timestamp()   
  return int(utc_timestamp*1000000)
# -----------------------------------------------------------------------------
def showHex(label,bytes_msg):
  if not DEBUG:
    return
  # HEX APOCALYPSE
  s='                  <{}: {}>'.format(label,bytes_msg.hex())
  print(s)
# -----------------------------------------------------------------------------
# Print a string boxed by specified char for emphasis. 
# e.g. ('@','Hello, World!',4):
#
#     @@@@@@@@@@@@@@@@@@@
#     @@ Hello, World! @@
#     @@@@@@@@@@@@@@@@@@@
# -----------------------------------------------------------------------------
def boxPrint(box_char=None, msg = None, indent=0, bell = False):
  if None in [box_char, msg]:
    return
  s = box_char*2
  m = s+'  '+msg.strip()+'  '+s
  l = box_char*len(m)
  i = ' '*indent
  s = '\n'+i+l+'\n'+i+m+'\n'+i+l
  if bell:
    s+='\a'   # ring the terminal bell too lol
  s+='\n'
  print(s)
# -----------------------------------------------------------------------------
def logMsg(msg, bell = True):
  tstamp = datetime.now().strftime("%m/%d %H:%M:%S")
  print(tstamp+' '+msg)
# -----------------------------------------------------------------------------
# For when you really want your bugs to pop.  
# -----------------------------------------------------------------------------
def errBox(msg, bell = True):
  tstamp = datetime.now().strftime("%m/%d %H:%M:%S")
  if bell:
    msg+='\a'
  print(tstamp+' '+msg)
  #boxPrint('#', tstamp+' '+msg ,0, bell=bell)
# -----------------------------------------------------------------------------
def macAddrBytesToString(mbytes):  
  radio_mac = ''
  for i,b in enumerate(mbytes):
    radio_mac+='{:02x}'.format(b)
    if i<5:
      radio_mac+=':'
  return radio_mac

# -----------------------------------------------------------------------------
