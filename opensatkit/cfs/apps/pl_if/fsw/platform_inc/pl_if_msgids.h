/*
** Purpose: Define message IDs for the Pl_if application
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
#ifndef _pl_if_msgids_
#define _pl_if_msgids_

/*
** Command Message IDs
*/

#define  PL_IF_CMD_MID        0x19D9
#define  PL_IF_SEND_HK_MID    0x19DE

/*
** Telemetry Message IDs
*/

#define  PL_IF_TLM_HK_MID           0x09DE
#define  PL_IF_PL_STATUS_MID        0x09E4

#endif /* _pl_if_msgids_ */
