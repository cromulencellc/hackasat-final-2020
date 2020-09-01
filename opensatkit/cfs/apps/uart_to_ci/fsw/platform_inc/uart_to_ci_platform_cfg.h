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

#ifndef _uart_to_ci_platform_cfg_
#define _uart_to_ci_platform_cfg_

/*
** Includes
*/

#include "uart_to_ci_mission_cfg.h"
#include "uart_to_ci_msgids.h"
#include "uart_to_ci_perfids.h"

/******************************************************************************
** UART Telemetry Output Command Ingest UART_TO_CI Application Macros
*/

#define  UART_TO_CI_RUNLOOP_DELAY      500  /* Delay in milliseconds */
#define  UART_TO_CI_RUNLOOP_MSG_READ   10   /* Max messages read in main loop iteration */
#define  UART_TO_CI_DEF_PKTTBL_FILE_NAME "/cf/uart_to_pkt_tbl.json"


#endif /* _uart_to_ci_platform_cfg_ */
