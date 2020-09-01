/*
** Purpose: Define message IDs for the Ephem application
**
** Notes:
**   None
**
** License:
**   Written by David McComas, licensed under the copyleft GNU General
**   Public License (GPL).
**
** References:
**   1. OpenSatKit Object-based Application Developer's Guide.
**   2. cFS Application Developer's Guide.
**
*/
#ifndef _ephem_msgids_
#define _ephem_msgids_

/*
** Command Message IDs
*/

#define  EPHEM_CMD_MID        0x19DC
#define  EPHEM_SEND_HK_MID    0x19DD

/*
** Telemetry Message IDs
*/

#define  EPHEM_TLM_HK_MID     0x09E2
#define  EPHEM_TLM_EPHEM_MID  0x09E3

#endif /* _ephem_msgids_ */