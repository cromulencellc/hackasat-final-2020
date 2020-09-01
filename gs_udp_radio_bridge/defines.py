# -----------------------------------------------------------------------------
# Definitions used by bridge.py, test_bridge.py, observer.py
# -----------------------------------------------------------------------------

TRANSCRIPT_FILENAME  = 'tscript.txt'  # filename or None
HAIL_TIMEOUT_SECS    = 0.5
RESP_TIMEOUT_SECS 	= 5   
DEBUG                = False          # Tons of hex to stdout

# -----------------------------------------------------------------------------
# message destination codes 
# -----------------------------------------------------------------------------

NODE_RADIO      = 0  # radio module
NODE_OOBM       = 1  # oobm application (straight passthrough)
NODE_LEON       = 2  # leon3 application (straight passthrough)
NODE_GS         = 3  # ground station. (not in protocol)

# -----------------------------------------------------------------------------
# one of these immediately follows the msg tag of a udp to a watcher to 
# indicate msg direction.
# -----------------------------------------------------------------------------
SENT_TO_GROUND      = b'\x00'
SENT_FROM_GROUND    = b'\x01'
# -----------------------------------------------------------------------------
# corresponds to common.h groundstation_rf
# -----------------------------------------------------------------------------
CMD_HAIL           =     b'\x00'   # None    7 bytes team_id and mac address
CMD_STATUS         =     b'\x01'   # None    byte - status code
CMD_LOOPBACK       =     b'\x02'   # bytes   same bytes
CMD_PKT_LOSS       =     b'\x03'   # None    little-endian float32 pkt loss %
# -----------------------------------------------------------------------------
# if a bridge command is a pass / fail, we send back the original command plus
# oneof these codes.
# -----------------------------------------------------------------------------
CODE_FAIL = b'\x00'
CODE_PASS = b'\x01'
# -----------------------------------------------------------------------------
#
# -----------------------------------------------------------------------------
