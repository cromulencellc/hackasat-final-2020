/*
** Purpose: Define the UART Telemetry Output Command Ingestapplication. This app
**          receives telemetry packets from the software bus and uses its
**          packet table to determine whether packets should be sent over
**          a UART.  Commmands are received over UART and sent out as SB messages.
**
**
** License:
**   Modified from code written by David McComas, licensed under the copyleft GNU
**   General Public License (GPL). 
**
** References:
**   1. OpenSatKit Object-based Application Developer's Guide.
**   2. cFS Application Developer's Guide.
**
*/
#ifndef _uart_to_ci_app_
#define _uart_to_ci_app_

/*
** Includes
*/

#include "app_cfg.h"
#include "pkttbl.h"
#include "pktmgr.h"

/*
** Macro Definitions
*/

#define UART_TO_CI_INIT_APP_INFO_EID    (UART_TO_CI_BASE_EID + 0)
#define UART_TO_CI_NOOP_INFO_EID        (UART_TO_CI_BASE_EID + 1)
#define UART_TO_CI_EXIT_ERR_EID         (UART_TO_CI_BASE_EID + 2)
#define UART_TO_CI_INVALID_MID_ERR_EID  (UART_TO_CI_BASE_EID + 3)

#define UART_TO_CI_TOTAL_EID  4


/*
** Type Definitions
*/

typedef struct
{

   CMDMGR_Class CmdMgr;
   TBLMGR_Class TblMgr;
   PKTTBL_Class PktTbl;
   PKTMGR_Class PktMgr;

   CFE_SB_PipeId_t CmdPipe;

} UART_TO_CI_Class;

typedef struct
{

   uint8    Header[CFE_SB_TLM_HDR_SIZE];

   /*
   ** CMDMGR Data
   */
   uint16   ValidCmdCnt;
   uint16   InvalidCmdCnt;

   /*
   ** PKTTBL Data
   */

   uint8    PktTblLastLoadStatus;
   uint8    SpareAlignByte;
   uint16   PktTblAttrErrCnt;

   /*
   ** CI Data
   */

   uint32   RecvCmdCnt;
   uint32   RecvCmdErrCnt;
   uint32   SentTlmCnt;
   uint32   BaudRate;

} OS_PACK UART_TO_CI_HkPkt;

#define UART_TO_CI_TLM_HK_LEN sizeof (UART_TO_CI_HkPkt)

/*
** Exported Data
*/

extern UART_TO_CI_Class  UartToCi;


/*
** Exported Functions
*/

/******************************************************************************
** Function: UART_TO_CI_AppMain
**
*/
void UART_TO_CI_AppMain(void);


/******************************************************************************
** Function: UART_TO_CI_NoOpCmd
**
** Notes:
**   1. Function signature must match the CMDMGR_CmdFuncPtr definition
**
*/
boolean UART_TO_CI_NoOpCmd(void* ObjDataPtr, const CFE_SB_MsgPtr_t MsgPtr);


/******************************************************************************
** Function: UART_TO_CI_ResetAppCmd
**
** Notes:
**   1. Function signature must match the CMDMGR_CmdFuncPtr definition
**
*/
boolean UART_TO_CI_ResetAppCmd(void* ObjDataPtr, const CFE_SB_MsgPtr_t MsgPtr);


#endif /* _uart_to_app_ */
