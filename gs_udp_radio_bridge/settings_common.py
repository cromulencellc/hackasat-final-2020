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
  ("127.0.0.1",    5000,    "127.0.0.1",    5020  ),      # team_1
  ("127.0.0.1",    5001,    "127.0.0.1",    5021  ),      # team_2
  ("127.0.0.1",    5002,    "127.0.0.1",    5022  ),      # team_3
  ("127.0.0.1",    5003,    "127.0.0.1",    5023  ),      # team_4
  ("127.0.0.1",    5004,    "127.0.0.1",    5024  ),      # team_5
  ("127.0.0.1",    5005,    "127.0.0.1",    5025  ),      # team_6
  ("127.0.0.1",    5006,    "127.0.0.1",    5026  ),      # team_7
  ("127.0.0.1",    5007,    "127.0.0.1",    5027  ),      # team_8
  ("127.0.0.1",    5008,    "127.0.0.1",    5028  ),      # team_9
  ("127.0.0.1",    5009,    "127.0.0.1",    5029  ),      # team_10
]

WATCHERS = [  
  ("127.0.0.1",    5010,    "127.0.0.1",    5030  ),      # watcher_1 of team_1
  ("127.0.0.1",    5011,    "127.0.0.1",    5031  ),      # watcher_2 of team_2
  ("127.0.0.1",    5012,    "127.0.0.1",    5032  ),      # watcher_3 of team_3
  ("127.0.0.1",    5013,    "127.0.0.1",    5033  ),      # watcher_4 of team_4
  ("127.0.0.1",    5014,    "127.0.0.1",    5034  ),      # watcher_5 of team_5
  ("127.0.0.1",    5015,    "127.0.0.1",    5035  ),      # watcher_6 of team_6
  ("127.0.0.1",    5016,    "127.0.0.1",    5036  ),      # watcher_7 of team_7
  ("127.0.0.1",    5017,    "127.0.0.1",    5037  ),      # watcher_8 of team_8
  ("127.0.0.1",    5018,    "127.0.0.1",    5038  ),      # watcher_9 of team_9
  ("127.0.0.1",    5019,    "127.0.0.1",    5039  ),      # watcher_10 of team_10
]

BOSSES = [  
  ("127.0.0.1",    5040,    "127.0.0.1",    5050  ),      # boss_1 of team_1
  ("127.0.0.1",    5041,    "127.0.0.1",    5051  ),      # boss_2 of team_2
  ("127.0.0.1",    5042,    "127.0.0.1",    5052  ),      # boss_3 of team_3
  ("127.0.0.1",    5043,    "127.0.0.1",    5053  ),      # boss_4 of team_4
  ("127.0.0.1",    5044,    "127.0.0.1",    5054  ),      # boss_5 of team_5
  ("127.0.0.1",    5045,    "127.0.0.1",    5055  ),      # boss_6 of team_6
  ("127.0.0.1",    5046,    "127.0.0.1",    5056  ),      # boss_7 of team_7
  ("127.0.0.1",    5047,    "127.0.0.1",    5057  ),      # boss_8 of team_8
  ("127.0.0.1",    5048,    "127.0.0.1",    5058  ),      # boss_9 of team_9
  ("127.0.0.1",    5049,    "127.0.0.1",    5059  ),      # boss_10 of team_10
]

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
