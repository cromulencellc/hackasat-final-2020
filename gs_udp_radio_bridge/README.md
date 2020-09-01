# gs_udp_radio_bridge

### Enumerates the USB-connected radios, sets up a UDP socket for each, and then acts as a bridge. 

### The following settings are in settings_common.py:

```
# -----------------------------------------------------------------------------
# Common config file for bridge,py, test_bridge.py, etc.
# -----------------------------------------------------------------------------

# For challenges 1 & 2:

Z_SPIN_PERIOD_SECS        = 75
Z_SPIN_COMMS_WINDOW_SECS  = 15

DO_ORBITAL_BLACKOUT       = True       # set True for release build!!!1

ORBIT_PERIOD_SECS         = 60 * 15
ORBIT_COMMS_WINDOW_SECS   = 60 * 7.5

  # client         client   bridge          bridge        client
  # IP             rx       IP              rx            name 
  # ---------      ----     ----------      ----          --------
TEAMS = [ 
  ("127.0.0.1",    5000,    "127.0.0.1",    5020  ),      # team_0
  ("127.0.0.1",    5001,    "127.0.0.1",    5021  ),      # team_1
  ("127.0.0.1",    5002,    "127.0.0.1",    5022  ),      # team_2
  ("127.0.0.1",    5003,    "127.0.0.1",    5023  ),      # team_3
  ("127.0.0.1",    5004,    "127.0.0.1",    5024  ),      # team_4
  ("127.0.0.1",    5005,    "127.0.0.1",    5025  ),      # team_5
  ("127.0.0.1",    5006,    "127.0.0.1",    5026  ),      # team_6
  ("127.0.0.1",    5007,    "127.0.0.1",    5027  ),      # team_7
  ("127.0.0.1",    5008,    "127.0.0.1",    5028  ),      # team_8
  ("127.0.0.1",    5009,    "127.0.0.1",    5029  ),      # team_9
]

WATCHERS = [  
  ("127.0.0.1",    5010,    "127.0.0.1",    5030  ),      # watcher_0 of team_0
  ("127.0.0.1",    5011,    "127.0.0.1",    5031  ),      # watcher_1 of team_1
  ("127.0.0.1",    5012,    "127.0.0.1",    5032  ),      # watcher_2 of team_2
  ("127.0.0.1",    5013,    "127.0.0.1",    5033  ),      # watcher_3 of team_3
  ("127.0.0.1",    5014,    "127.0.0.1",    5034  ),      # watcher_4 of team_4
  ("127.0.0.1",    5015,    "127.0.0.1",    5035  ),      # watcher_5 of team_5
  ("127.0.0.1",    5016,    "127.0.0.1",    5036  ),      # watcher_6 of team_6
  ("127.0.0.1",    5017,    "127.0.0.1",    5037  ),      # watcher_7 of team_7
  ("127.0.0.1",    5018,    "127.0.0.1",    5038  ),      # watcher_8 of team_8
  ("127.0.0.1",    5019,    "127.0.0.1",    5039  ),      # watcher_9 of team_9
]

BOSSES = [  
  ("127.0.0.1",    5040,    "127.0.0.1",    5050  ),      # boss_0 of team_0
  ("127.0.0.1",    5041,    "127.0.0.1",    5051  ),      # boss_1 of team_1
  ("127.0.0.1",    5042,    "127.0.0.1",    5052  ),      # boss_2 of team_2
  ("127.0.0.1",    5043,    "127.0.0.1",    5053  ),      # boss_3 of team_3
  ("127.0.0.1",    5044,    "127.0.0.1",    5054  ),      # boss_4 of team_4
  ("127.0.0.1",    5045,    "127.0.0.1",    5055  ),      # boss_5 of team_5
  ("127.0.0.1",    5046,    "127.0.0.1",    5056  ),      # boss_6 of team_6
  ("127.0.0.1",    5047,    "127.0.0.1",    5057  ),      # boss_7 of team_7
  ("127.0.0.1",    5048,    "127.0.0.1",    5058  ),      # boss_8 of team_8
  ("127.0.0.1",    5049,    "127.0.0.1",    5059  ),      # boss_8 of team_9
]

```

<p>todo: writeup ^ </p>

### Bridge client types (Enforced based on assigned IP)

```
Teams:
-----
  - Can only send/recv messages of type L3 and RSC, thru/to their own radio, 
    with their own flatsat. 
  - The src IP of a message is checked against the assigned IP for that team.

Bosses:
------
  - One per team.
  - Can send any message type besides RSC.  (OM, L3, RA, BR)
  - Can seize their Team's radio:
      . While siezed, normal team-radio comms semi-gracefully paused.
      . When Boss is done, it tells the bridge to release the radio.

Watchers:
--------
  - One per team.
  - Receive the same L3 messages as their team receives
  - Can only send/recv messages of type L3 and RSC, thru/to their own radio, 
    with their own flatsat. 
  - Not subject to boss radio seizure or comms blackouts
```
### One round of team commands / telemetry:

    Ground App    UDP -->
    Bridge        USB_UART 115.2kbps -->
    Heltec Radio  LoRa -->
    OOBM CPU      UART 19.2kbps -->
    Leon3 CPU     UART 19.2kbps -->
    OOBM CPU      LoRa -->
    Heltec Radio  USB_UART 115.2kbps -->
    Bridge        UDP -->
    Ground App

### The UDP packet format:

```

Command : <ascii_tag><bytes>
Response: <same_tag><bytes>

  Team Tags:
  ---------
   'L3:' (Leon3 ): Cosmos CCSDS telemetry and ground commands.
                   (Only with assigned flatsat and only if src IP matches.)
     'RSC:' initial bringup challenge - Must lower bitrate and increase power.
        uint8_t bitrate;    # 0:low (more reliable), 1:high (normal)
        uint8_t power;      # 0:low  (power saving), 1:high (more_reliable)

  Watcher:
  -------
    Get CC'ed on incoming L3 messages for their assigned team.

  Boss Tags:
  ---------
    'L3:'  (Leon3 ): Cosmos CCSDS telemetry and ground commands.
      (For protocol see radio.py)
    'OM:'  (MSP430): Oob management (L3 Reset, Reflash.)
      (For protocol see radio.py)
    'BR:'  (Bridge): 
      Boss Commands to Bridge:
        'BR:S': Seize radio from team:
          Bridge returns orig. msg with added byte:  
            'BR:S',0x00 for fail
            'BR:S',0x01 for pass
        'BR:U': Unseize radio; unpause team comms:
          Bridge returns orig. msg with added byte:  
            'BR:U',0x00 for fail
            'BR:U',0x01 for pass

```

### Protocol buffering and sanity checking

<p>All data coming to the bridge from the radio is buffered until a complete radio protocol frame is received. For nodes OOBM and Radio, the frame is then sent via UDP to ground. For Leon3 messages, additional buffering is done to ensure a whole CCSDS frame has arrived before it is passed on via UDP.</p>
<p>UDP messages from Teams must contain a full CCSDS packet (with payload <= 2048 bytes) to be sent along to the radio.</p>

# test_team.py and test_bridge.py

<p>test_team is a bridge client of type Team and sends some test messages.</p>
<p>test_bridge is a bridge client of type Boss. It queries the bridge regarding how many radios are connected, and it sends various message types to exercise the software.</p>

### Responses and round-trip times are logged to screen, e.g.

```
ubu@ubuntu:~/has/gs_udp_radio_bridge$ ./test_bridge.py 

*** UDP to Radio Bridge Tester v0.2

  Querying Bridge:
    Response:
      FOUND radio_0!
  Seizing radio_0:
    Tx: 'BR:5330'
      Rx: 'BR:533001' 35 ms
        PASS
  Hailing radio_0:
    Tx: 'RA0:00'
      Rx: 'RA0:0000246f287790a8' 467 ms
        RADIO ID : 0
        RADIO MAC: 00:24:6f:28:77:90
  Unseizing radio_0:
    Tx: 'BR:5530'
      Rx: 'BR:553001' 16 ms
        PASS
  Seizing radio_1:
    Tx: 'BR:5331'
      Rx: 'BR:533100' 5 ms
        FAIL

ubu@ubuntu:~/has/gs_udp_radio_bridge$ ./test_team.py 

*** Team UDP Simulator v0.2

Send a valid leon3 message:
    Tx: 'L3:deadbeef0000000000040000000000'
Send a bad message type:
    Tx: 'RA:00'
Send an invalid leon3 message (bad header):
    Tx: 'L3:00'

ubu@ubuntu:~/has/gs_udp_radio_bridge$ 
```

### bridge.py during that session:

```
ubu@ubuntu:~/has/gs_udp_radio_bridge$ ./bridge.py 

*** UDP to Radio Bridge v0.2

  Enumerating local radios:
    /dev/ttyS0 - ttyS0                                          
    /dev/ttyUSB1 - CP2102 USB to UART Bridge Controller         
      radio_0: 24:6f:28:77:90:a8 '/dev/ttyUSB1'

  Starting radio thread:
    radio_0: 24:6f:28:77:90:a8 '/dev/ttyUSB1'
                  <team_0 has radio_0>

  Starting client threads:
                           Bridge  Bridge  Radio
      Name IP Address      Rx Port Tx Port Status
      ---- ----------      ------- ------- ------
    team_0 127.0.0.1       5020    5000    (radio_0)
 watcher_0 127.0.0.1       5030    5010    
 watcher_1 127.0.0.1       5031    5011    
    boss_0 127.0.0.1       5032    5012    (No radio)
    boss_1 127.0.0.1       5033    5013    (No radio)

  -----------------------------------------------
  --  1 Team is ready to rock.  <Ctrl-C Quit>  --
  -----------------------------------------------

    boss_0: Rx : 'BR:51'
    boss_0:  Tx: 'BR:510001'
    boss_0: Rx : 'BR:5330'
                  <team_0 lost radio_0>
                  <boss_0 has radio_0>
    boss_0:  Tx: 'BR:533001'
    boss_0: Rx : 'RA0:00'
                  <usb_tx: a550000100>
                  <usb_rx: a55000080000246f287790a8>
    boss_0:  Tx: 'RA0:0000246f287790a8'
    boss_0: Rx : 'BR:5530'
                  <boss_0 lost radio_0>
                  <team_0 has radio_0>
    boss_0:  Tx: 'BR:553001'
    boss_0: Rx : 'BR:5331'

#####################################################
##  boss_0 wants radio_1, but it's not connected.  ##
#####################################################

    boss_0:  Tx: 'BR:533100'
    team_0: Rx : 'L3:deadbeef0000000000040000000000'
                  <CCSDS strm 0x0000 seq 0x0000 len 4 payload 0000000000>
    team_0: Rx : 'RA:00'

########################################
##  team_0: Bad message tag: 'RA' !!  ##
########################################
                  <usb_tx: a552000fdeadbeef0000000000040000000000>

    team_0: Rx : 'L3:00'

###################################################
##  team_0: Sent UDP with invalid CCSDS packet!  ##
###################################################

^C
  Ending client threads:
    team_0 127.0.0.1       5020    5000    (radio_0)
 watcher_0 127.0.0.1       5030    5010    
 watcher_1 127.0.0.1       5031    5011    
    boss_0 127.0.0.1       5032    5012    (No radio)
    boss_1 127.0.0.1       5033    5013    (No radio)

  Ending radio thread:
    radio_0: 24:6f:28:77:90:a8 '/dev/ttyUSB1'

ubu@ubuntu:~/has/gs_udp_radio_bridge$ 
```
# observer.py (WIP)

<p>This is an example code that receives the same L3: messages as team_0.</p>

