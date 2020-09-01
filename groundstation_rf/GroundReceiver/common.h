// ----------------------------------------------------------------------------
// Stuff in common between groundstation_rf and flatsat_openmsp430/client/radio
// ----------------------------------------------------------------------------
#ifndef __COMMON_H__
#define __COMMON_H__
#include <stdint.h>
// ----------------------------------------------------------------------------
// a nybble in the usb pkt format identifies the node it is to/from
// ----------------------------------------------------------------------------
#define NODE_RADIO              0       // this is us
#define NODE_OOBM               1       // management processor
#define NODE_LEON               2       // flight processor
// ----------------------------------------------------------------------------
// Oobm and LoRa 32 radio commands
// ----------------------------------------------------------------------------
//      Name                   ID     Params  Return Value
#define CMD_HAIL                0   // None   7 bytes team_id and mac address
#define CMD_STATUS              1   // None   byte - status code
#define CMD_LOOPBACK            2   // bytes  same bytes
#define CMD_PKT_LOSS            3   // None   Little endian float32 pkt loss %
//      Name                   ID     Params  Return Value
// ----------------------------------------------------------------------------
// Status codes that may be sent with CMD_STATUS
// ----------------------------------------------------------------------------
#define EC_OK             0
#define EC_CMD_UNKNOWN    1
#define EC_DEST_UNKNOWN   2
// ----------------------------------------------------------------------------
#endif // __COMMON_H__
// ----------------------------------------------------------------------------
// EOF
// ----------------------------------------------------------------------------
