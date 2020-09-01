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

/*
** Includes
*/

#include <string.h>
#include "uart_to_ci_app.h"


/*
** Local Function Prototypes
*/

static int32 InitApp(void);
static void ProcessCommands(void);

/*
** Global Data
*/

UART_TO_CI_Class  UartToCi;
UART_TO_CI_HkPkt  UartToCiHkPkt;

#define  CMDMGR_OBJ  (&(UartToCi.CmdMgr))  /* Convenience macro */
#define  TBLMGR_OBJ  (&(UartToCi.TblMgr))  /* Convenience macro */
#define  PKTTBL_OBJ  (&(UartToCi.PktTbl))  /* Convenience macro */
#define  PKTMGR_OBJ  (&(UartToCi.PktMgr))  /* Convenience macro */

/******************************************************************************
** Function: UART_TO_CI_AppMain
**
*/
void UART_TO_CI_AppMain(void)
{

   int32  Status    = CFE_SEVERITY_ERROR;
   uint32 RunStatus = CFE_ES_APP_ERROR;

   CFE_ES_PerfLogEntry(UART_TO_CI_MAIN_PERF_ID);
   Status = CFE_ES_RegisterApp();
   CFE_EVS_Register(NULL,0,0);

   /*
   ** Perform application specific initialization
   */
   if (Status == CFE_SUCCESS) {
      
      CFE_EVS_SendEvent(UART_TO_CI_INIT_DEBUG_EID, UART_TO_CI_INIT_EVS_TYPE, "UART_TO_CI: About to call init\n");
      Status = InitApp();
   
   }

   /*
   ** At this point many flight apps use CFE_ES_WaitForStartupSync() to
   ** synchronize their startup timing with other apps. This is not
   ** needed.
   */

   if (Status == CFE_SUCCESS) RunStatus = CFE_ES_APP_RUN;

   /*
   ** Main process loop
   */
   
   CFE_EVS_SendEvent(UART_TO_CI_INIT_DEBUG_EID, UART_TO_CI_INIT_EVS_TYPE, "UART_TO_CI: About to enter loop\n");
   while (CFE_ES_RunLoop(&RunStatus))
   {

      CFE_ES_PerfLogExit(UART_TO_CI_MAIN_PERF_ID);
      OS_TaskDelay(UART_TO_CI_RUNLOOP_DELAY);
      CFE_ES_PerfLogEntry(UART_TO_CI_MAIN_PERF_ID);
      
      PKTMGR_IngestCommands();

      PKTMGR_OutputTelemetry();

      ProcessCommands();

   } /* End CFE_ES_RunLoop */


   /* Write to system log in case events not working */

   CFE_ES_WriteToSysLog("UART_TO_CI App terminating, err = 0x%08X\n", Status);

   CFE_EVS_SendEvent(UART_TO_CI_EXIT_ERR_EID, CFE_EVS_CRITICAL, "UART_TO_CI: terminating, err = 0x%08X", Status);
   
   CFE_ES_PerfLogExit(UART_TO_CI_MAIN_PERF_ID);
   CFE_ES_ExitApp(RunStatus);  /* Let cFE kill the task (and any child tasks) */

} /* End of UART_TO_CI_AppMain() */


/******************************************************************************
** Function: UART_TO_CI_NoOpCmd
**
*/

boolean UART_TO_CI_NoOpCmd(void* ObjDataPtr, const CFE_SB_MsgPtr_t MsgPtr)
{

   CFE_EVS_SendEvent (UART_TO_CI_NOOP_INFO_EID,
                      CFE_EVS_INFORMATION,
                      "UART TO CI version %d.%d received a no operation command",
                     UART_TO_CI_MAJOR_VERSION,UART_TO_CI_MINOR_VERSION);

   return TRUE;


} /* End UART_TO_CI_NoOpCmd() */


/******************************************************************************
** Function: UART_TO_CI_ResetAppCmd
**
*/

boolean UART_TO_CI_ResetAppCmd(void* ObjDataPtr, const CFE_SB_MsgPtr_t MsgPtr)
{

   CMDMGR_ResetStatus(CMDMGR_OBJ);
   TBLMGR_ResetStatus(TBLMGR_OBJ);

   PKTMGR_ResetStatus();

   return TRUE;

} /* End UART_TO_CI_ResetAppCmd() */

/******************************************************************************
** Function: UART_TO_CI_SendHousekeepingPkt
**
*/
void UART_TO_CI_SendHousekeepingPkt(void)
{

   /*
   ** UART_TO_CI Data
   */

   UartToCiHkPkt.ValidCmdCnt   = UartToCi.CmdMgr.ValidCmdCnt;
   UartToCiHkPkt.InvalidCmdCnt = UartToCi.CmdMgr.InvalidCmdCnt;

   /*
   ** TBLMGR Data
   */

   UartToCiHkPkt.PktTblLastLoadStatus  = UartToCi.PktTbl.LastLoadStatus;
   UartToCiHkPkt.PktTblAttrErrCnt      = UartToCi.PktTbl.AttrErrCnt;

   /*
   ** CI Data
   */
  UartToCiHkPkt.RecvCmdCnt = UartToCi.PktMgr.RecvCmdCnt;
  UartToCiHkPkt.RecvCmdErrCnt = UartToCi.PktMgr.RecvCmdErrCnt;
  UartToCiHkPkt.SentTlmCnt = UartToCi.PktMgr.SentTlmCnt;

  UartToCiHkPkt.BaudRate = (uint32)UartToCi.PktMgr.config.baudRate;

   CFE_SB_TimeStampMsg((CFE_SB_Msg_t *) &UartToCiHkPkt);
   CFE_SB_SendMsg((CFE_SB_Msg_t *) &UartToCiHkPkt);

} /* End UART_TO_CI_SendHousekeepingPkt() */


/******************************************************************************
** Function: InitApp
**
*/
static int32 InitApp(void)
{
   int32 Status = CFE_SUCCESS;

   CFE_EVS_SendEvent(UART_TO_CI_INIT_DEBUG_EID, UART_TO_CI_INIT_EVS_TYPE, "UART_TO_CI_InitApp() Entry\n");

   /*
   ** Initialize 'entity' objects
   */

   PKTTBL_Constructor(PKTTBL_OBJ, PKTMGR_GetTblPtr, PKTMGR_LoadTbl, PKTMGR_LoadTblEntry);
   PKTMGR_Constructor(PKTMGR_OBJ, PKTMGR_PIPE_NAME, PKTMGR_PIPE_DEPTH);

   /*
   ** Initialize application managers
   */

   CFE_SB_CreatePipe(&UartToCi.CmdPipe, UART_TO_CI_CMD_PIPE_DEPTH, UART_TO_CI_CMD_PIPE_NAME);
   CFE_SB_Subscribe(UART_TO_CI_CMD_MID, UartToCi.CmdPipe);
   CFE_SB_Subscribe(UART_TO_CI_SEND_HK_MID, UartToCi.CmdPipe);

   CFE_EVS_SendEvent(UART_TO_CI_INIT_DEBUG_EID, UART_TO_CI_INIT_EVS_TYPE, "UART_TO_CI_InitApp() Before CMDMGR calls\n");
   CMDMGR_Constructor(CMDMGR_OBJ);
   CMDMGR_RegisterFunc(CMDMGR_OBJ, CMDMGR_NOOP_CMD_FC,            NULL,       UART_TO_CI_NoOpCmd,        0);
   CMDMGR_RegisterFunc(CMDMGR_OBJ, CMDMGR_RESET_CMD_FC,           NULL,       UART_TO_CI_ResetAppCmd,    0);

   CMDMGR_RegisterFunc(CMDMGR_OBJ, UART_TO_CI_PKT_TBL_LOAD_CMD_FC,    TBLMGR_OBJ, TBLMGR_LoadTblCmd,         TBLMGR_LOAD_TBL_CMD_DATA_LEN);
   CMDMGR_RegisterFunc(CMDMGR_OBJ, UART_TO_CI_PKT_TBL_DUMP_CMD_FC,    TBLMGR_OBJ, TBLMGR_DumpTblCmd,         TBLMGR_DUMP_TBL_CMD_DATA_LEN);

   CMDMGR_RegisterFunc(CMDMGR_OBJ, UART_TO_CI_ADD_PKT_CMD_FC,         PKTMGR_OBJ, PKTMGR_AddPktCmd,          PKKTMGR_ADD_PKT_CMD_DATA_LEN);
   CMDMGR_RegisterFunc(CMDMGR_OBJ, UART_TO_CI_REMOVE_PKT_CMD_FC,      PKTMGR_OBJ, PKTMGR_RemovePktCmd,       PKKTMGR_REMOVE_PKT_CMD_DATA_LEN);
   CMDMGR_RegisterFunc(CMDMGR_OBJ, UART_TO_CI_REMOVE_ALL_PKTS_CMD_FC, PKTMGR_OBJ, PKTMGR_RemoveAllPktsCmd,   0);
   CMDMGR_RegisterFunc(CMDMGR_OBJ, UART_TO_CI_ENABLE_OUTPUT_CMD_FC,   PKTMGR_OBJ, PKTMGR_EnableTlmOutputCmd, PKKTMGR_ENABLE_OUTPUT_CMD_DATA_LEN);
   CMDMGR_RegisterFunc(CMDMGR_OBJ, UART_TO_CI_DISABLE_OUTPUT_CMD_FC,  PKTMGR_OBJ, PKTMGR_DisableTlmOutputCmd, PKKTMGR_DISABLE_OUTPUT_CMD_DATA_LEN);
   CMDMGR_RegisterFunc(CMDMGR_OBJ, UART_TO_CI_SET_RATE_CMD_FC,        PKTMGR_OBJ, PKTMGR_SetRateCmd,          PKKTMGR_SET_RATE_CMD_DATA_LEN);



   CFE_EVS_SendEvent(UART_TO_CI_INIT_DEBUG_EID, UART_TO_CI_INIT_EVS_TYPE, "UART_TO_CI_InitApp() Before TBLMGR calls\n");
   TBLMGR_Constructor(TBLMGR_OBJ);
   TBLMGR_RegisterTblWithDef(TBLMGR_OBJ, PKTTBL_LoadCmd, PKTTBL_DumpCmd, UART_TO_CI_DEF_PKTTBL_FILE_NAME);

   CFE_SB_InitMsg(&UartToCiHkPkt, UART_TO_CI_HK_TLM_MID, UART_TO_CI_TLM_HK_LEN, TRUE);

   /*
   ** Application startup event message
   */

   Status = CFE_EVS_SendEvent(UART_TO_CI_INIT_APP_INFO_EID,
                              CFE_EVS_INFORMATION,
                              "UART_TO_CI Initialized. Version %d.%d.%d.%d",
                              UART_TO_CI_MAJOR_VERSION,
                              UART_TO_CI_MINOR_VERSION,
                              UART_TO_CI_REVISION,
                              UART_TO_CI_MISSION_REV);

   return(Status);

} /* End of InitApp() */

/******************************************************************************
** Function: ProcessCommands
**
*/
static void ProcessCommands(void)
{

   int32           Status;
   CFE_SB_Msg_t*   CmdMsgPtr;
   CFE_SB_MsgId_t  MsgId;

   Status = CFE_SB_RcvMsg(&CmdMsgPtr, UartToCi.CmdPipe, CFE_SB_POLL);

   if (Status == CFE_SUCCESS)
   {

      MsgId = CFE_SB_GetMsgId(CmdMsgPtr);

      switch (MsgId)
      {
         case UART_TO_CI_CMD_MID:
            CMDMGR_DispatchFunc(CMDMGR_OBJ, CmdMsgPtr);
            break;

         case UART_TO_CI_SEND_HK_MID:
            UART_TO_CI_SendHousekeepingPkt();
            break;

         default:
            CFE_EVS_SendEvent(UART_TO_CI_INVALID_MID_ERR_EID, CFE_EVS_ERROR,
                              "Received invalid command packet,MID = 0x%4X",MsgId);

            break;

      } /* End Msgid switch */

   } /* End if SB received a packet */

} /* End ProcessCommands() */


/* end of file */
