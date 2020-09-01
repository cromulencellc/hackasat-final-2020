/* 
** Purpose: Implement a Chal4 application.
**
** Notes:
**   None
**
** License:
**   Template written by David McComas and licensed under the GNU
**   Lesser General Public License (LGPL).
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
#include "chal4_app.h"


/*
** Local Function Prototypes
*/

static int32 InitApp(void);
static void ProcessCommands(void);

/*
** Global Data
*/

CHAL4_Class  Chal4;
CHAL4_HkPkt  Chal4HkPkt;
CHAL4_ShellOutputPkt Chal4ShellOutputPkt;

/*
** Convenience Macros
*/

#define  CMDMGR_OBJ (&(Chal4.CmdMgr))
#define  EX_OBJ     (&(Chal4.ExObj))

/******************************************************************************
** Function: CHAL4_AppMain
**
*/
void CHAL4_AppMain(void)
{

   int32  Status    = CFE_SEVERITY_ERROR;
   uint32 RunStatus = CFE_ES_APP_ERROR;


   CFE_ES_PerfLogEntry(CHAL4_MAIN_PERF_ID);
   Status = CFE_ES_RegisterApp();
   CFE_EVS_Register(NULL,0,0);


   /*
   ** Perform application specific initialization
   */
   if (Status == CFE_SUCCESS)
   {
       Status = InitApp();
   }

   /*
   ** At this point many flight apps use CFE_ES_WaitForStartupSync() to
   ** synchronize their startup timing with other apps. This is not
   ** needed for this simple app.
   */

   if (Status == CFE_SUCCESS) RunStatus = CFE_ES_APP_RUN;

   /*
   ** Main process loop
   */
   while (CFE_ES_RunLoop(&RunStatus))
   {

      /*
      ** This is just a an example loop. There are many ways to control the
      ** main loop execution flow.
      */
	  
	   CFE_ES_PerfLogExit(CHAL4_MAIN_PERF_ID);
      OS_TaskDelay(CHAL4_RUNLOOP_DELAY);
      CFE_ES_PerfLogEntry(CHAL4_MAIN_PERF_ID);

      EXOBJ_Execute();

      ProcessCommands();

   } /* End CFE_ES_RunLoop */


   /* Write to system log in case events not working */

   CFE_ES_WriteToSysLog("CHAL4 Terminating, RunLoop status = 0x%08X\n", RunStatus);

   CFE_EVS_SendEvent(CHAL4_EXIT_ERR_EID, CFE_EVS_CRITICAL, "CHAL4 Terminating,  RunLoop status = 0x%08X", RunStatus);

   CFE_ES_PerfLogExit(CHAL4_MAIN_PERF_ID);
   CFE_ES_ExitApp(RunStatus);  /* Let cFE kill the task (and any child tasks) */

} /* End of CHAL4_Main() */


/******************************************************************************
** Function: CHAL4_NoOpCmd
**
** Function signature must match CMDMGR_CmdFuncPtr typedef 
*/

boolean CHAL4_NoOpCmd(void* DataObjPtr, const CFE_SB_MsgPtr_t MsgPtr)
{

   CFE_EVS_SendEvent (CHAL4_CMD_NOOP_INFO_EID,
                      CFE_EVS_INFORMATION,
                      "No operation command received for CHAL4 version %d.%d",
                      CHAL4_MAJOR_VERSION,CHAL4_MINOR_VERSION);

   return TRUE;


} /* End CHAL4_NoOpCmd() */


/******************************************************************************
** Function: CHAL4_ResetAppCmd
**
** Function signature must match CMDMGR_CmdFuncPtr typedef 
*/

boolean CHAL4_ResetAppCmd(void* DataObjPtr, const CFE_SB_MsgPtr_t MsgPtr)
{

   CMDMGR_ResetStatus(CMDMGR_OBJ);
   EXOBJ_ResetStatus();

   return TRUE;

} /* End CHAL4_ResetAppCmd() */


/******************************************************************************
** Function: CHAL4_SendHousekeepingPkt
**
*/
void CHAL4_SendHousekeepingPkt(void)
{
   
   /*
   ** CMDMGR Data
   */

   Chal4HkPkt.ValidCmdCnt   = Chal4.CmdMgr.ValidCmdCnt;
   Chal4HkPkt.InvalidCmdCnt = Chal4.CmdMgr.InvalidCmdCnt;

   Chal4HkPkt.ExObjExecCnt = Chal4.ExObj.ExecCnt;

   CFE_SB_TimeStampMsg((CFE_SB_Msg_t *) &Chal4HkPkt);
   CFE_SB_SendMsg((CFE_SB_Msg_t *) &Chal4HkPkt);

} /* End CHAL4_SendHousekeepingPkt() */

/******************************************************************************
** Function: CHAL4_SendShellOutputPkt
**
*/
void CHAL4_SendShellOutputPkt(void) {

   CFE_PSP_MemCpy(Chal4ShellOutputPkt.ShellOutput,EX_OBJ->RxBuff,sizeof(Chal4ShellOutputPkt.ShellOutput));

   CFE_SB_TimeStampMsg((CFE_SB_Msg_t *) &Chal4ShellOutputPkt);
   CFE_SB_SendMsg((CFE_SB_Msg_t *) &Chal4ShellOutputPkt);

} /* End CHAL4_SendShellOutputPkt */

/******************************************************************************
** Function: InitApp
**
*/
static int32 InitApp(void)
{
    int32 Status = CFE_SUCCESS;

    
    /*
    ** Initialize 'entity' objects
    */

    EXOBJ_Constructor(EX_OBJ);

    /*
    ** Initialize cFE interfaces 
    */

    CFE_SB_CreatePipe(&Chal4.CmdPipe, CHAL4_CMD_PIPE_DEPTH, CHAL4_CMD_PIPE_NAME);
    CFE_SB_Subscribe(CHAL4_CMD_MID, Chal4.CmdPipe);
    CFE_SB_Subscribe(CHAL4_SEND_HK_MID, Chal4.CmdPipe);

    /*
    ** Initialize App Framework Components 
    */

    CMDMGR_Constructor(CMDMGR_OBJ);
    CMDMGR_RegisterFunc(CMDMGR_OBJ, CMDMGR_NOOP_CMD_FC,  NULL, CHAL4_NoOpCmd,     0);
    CMDMGR_RegisterFunc(CMDMGR_OBJ, CMDMGR_RESET_CMD_FC, NULL, CHAL4_ResetAppCmd, 0);
    
    CMDMGR_RegisterFunc(CMDMGR_OBJ, EXOBJ_SHELL_CMD_FC, EX_OBJ, EXOBJ_ShellCmd, EXOBJ_SHELL_CMD_DATA_LEN);
    CMDMGR_RegisterFunc(CMDMGR_OBJ, EXOBJ_ENA_CONSOLE_CMD_FC, EX_OBJ, EXOBJ_EnableConsole, EXOBJ_ENA_CONSOLE_CMD_DATA_LEN);

                         
    CFE_SB_InitMsg(&Chal4HkPkt, CHAL4_TLM_HK_MID, CHAL4_TLM_HK_LEN, TRUE);
    CFE_SB_InitMsg(&Chal4ShellOutputPkt, CHAL4_TLM_SHELL_OUTPUT_MID, CHAL4_TLM_SHELL_OUTPUT_LEN, TRUE);


                        
    /*
    ** Application startup event message
    */
    Status = CFE_EVS_SendEvent(CHAL4_INIT_INFO_EID,
                               CFE_EVS_INFORMATION,
                               "CHAL4 Initialized. Version %d.%d.%d.%d",
                               CHAL4_MAJOR_VERSION,
                               CHAL4_MINOR_VERSION,
                               CHAL4_REVISION,
                               CHAL4_MISSION_REV);

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

   Status = CFE_SB_RcvMsg(&CmdMsgPtr, Chal4.CmdPipe, CFE_SB_POLL);

   if (Status == CFE_SUCCESS)
   {

      MsgId = CFE_SB_GetMsgId(CmdMsgPtr);

      switch (MsgId)
      {
         case CHAL4_CMD_MID:
            CMDMGR_DispatchFunc(CMDMGR_OBJ, CmdMsgPtr);
            break;

         case CHAL4_SEND_HK_MID:
            CHAL4_SendHousekeepingPkt();
            CHAL4_SendShellOutputPkt();
            break;

         default:
            CFE_EVS_SendEvent(CHAL4_CMD_INVALID_MID_ERR_EID, CFE_EVS_ERROR,
                              "Received invalid command packet,MID = 0x%4X",MsgId);

            break;

      } /* End Msgid switch */

   } /* End if SB received a packet */

} /* End ProcessCommands() */


/* end of file */
