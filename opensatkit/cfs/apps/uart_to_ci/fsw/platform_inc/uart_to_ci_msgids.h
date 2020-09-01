/*
** Purpose: Define platform configurations for the UART Telemetry Output Command Ingest application
**
** Notes:
**   None
**
** License:
**   Modified from code written by David McComas, licensed under the copyleft GNU
**   General Public License (GPL). 
** 
**
** References:
**   1. OpenSatKit Object-based Application Developer's Guide.
**   2. cFS Application Developer's Guide.
**
*/
#ifndef _uart_to_ci_msgids_
#define _uart_to_ci_msgids_


/*
** Command Message IDs
*/

#define  UART_TO_CI_CMD_MID      (0x19D7)
#define  UART_TO_CI_SEND_HK_MID  (0x19D8)

/*
** Telemetry Message IDs
*/

#define  UART_TO_CI_HK_TLM_MID        (0x09DC)
#define  UART_TO_CI_DATA_TYPE_TLM_MID (0x09DD)

#endif /*_uart_to_ci_msgids_*/
