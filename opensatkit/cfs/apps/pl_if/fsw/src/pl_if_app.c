/* 
** Purpose: Implement a Pl_if application.
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
#include "pl_if_app.h"


/*
** Local Function Prototypes
*/

static int32 InitApp(void);
static void ProcessCommands(void);

/*
** Global Data
*/

PL_IF_Class  Pl_if;
PL_IF_HkPkt  Pl_ifHkPkt;

/*
** Convenience Macros
*/

#define  CMDMGR_OBJ (&(Pl_if.CmdMgr))
#define  TBLMGR_OBJ (&(Pl_if.TblMgr))
#define  PLIF_OBJ   (&(Pl_if.PlIfObj))
#define  EX_TBL     (&(Pl_if.ExTbl))

/******************************************************************************
** Function: PL_IF_AppMain
**
*/
void PL_IF_AppMain(void)
{

   int32  Status    = CFE_SEVERITY_ERROR;
   uint32 RunStatus = CFE_ES_APP_ERROR;


   CFE_ES_PerfLogEntry(PL_IF_MAIN_PERF_ID);
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
	  
	   CFE_ES_PerfLogExit(PL_IF_MAIN_PERF_ID);
      OS_TaskDelay(PL_IF_RUNLOOP_DELAY);
      CFE_ES_PerfLogEntry(PL_IF_MAIN_PERF_ID);

      PLIF_OBJ_Execute();

      ProcessCommands();

   } /* End CFE_ES_RunLoop */


   /* Write to system log in case events not working */

   CFE_ES_WriteToSysLog("PL_IF Terminating, RunLoop status = 0x%08X\n", RunStatus);

   CFE_EVS_SendEvent(PL_IF_EXIT_ERR_EID, CFE_EVS_CRITICAL, "PL_IF Terminating,  RunLoop status = 0x%08X", RunStatus);

   CFE_ES_PerfLogExit(PL_IF_MAIN_PERF_ID);
   CFE_ES_ExitApp(RunStatus);  /* Let cFE kill the task (and any child tasks) */

} /* End of PL_IF_Main() */


/******************************************************************************
** Function: PL_IF_NoOpCmd
**
** Function signature must match CMDMGR_CmdFuncPtr typedef 
*/

boolean PL_IF_NoOpCmd(void* DataObjPtr, const CFE_SB_MsgPtr_t MsgPtr)
{

   CFE_EVS_SendEvent (PL_IF_CMD_NOOP_INFO_EID,
                      CFE_EVS_INFORMATION,
                      "No operation command received for PL_IF version %d.%d",
                      PL_IF_MAJOR_VERSION,PL_IF_MINOR_VERSION);

   return TRUE;


} /* End PL_IF_NoOpCmd() */


/******************************************************************************
** Function: PL_IF_ResetAppCmd
**
** Function signature must match CMDMGR_CmdFuncPtr typedef 
*/

boolean PL_IF_ResetAppCmd(void* DataObjPtr, const CFE_SB_MsgPtr_t MsgPtr)
{

   CMDMGR_ResetStatus(CMDMGR_OBJ);
   TBLMGR_ResetStatus(TBLMGR_OBJ);
   EXTBL_ResetStatus();
   PLIF_OBJ_ResetStatus();

   return TRUE;

} /* End PL_IF_ResetAppCmd() */


/******************************************************************************
** Function: PL_IF_SendHousekeepingPkt
**
*/
void PL_IF_SendHousekeepingPkt(void)
{

   /* Good design practice in case app expands to more than one table */
   const TBLMGR_Tbl* LastTbl = TBLMGR_GetLastTblStatus(TBLMGR_OBJ);

   
   /*
   ** CMDMGR Data
   */

   Pl_ifHkPkt.ValidCmdCnt   = Pl_if.CmdMgr.ValidCmdCnt;
   Pl_ifHkPkt.InvalidCmdCnt = Pl_if.CmdMgr.InvalidCmdCnt;

   
   /*
   ** EXTBL/PLIF_OBJ Data
   ** - At a minimum all OBJECT variables effected by a reset must be included
   */

   Pl_ifHkPkt.LastAction       = LastTbl->LastAction;
   Pl_ifHkPkt.LastActionStatus = LastTbl->LastActionStatus;

   Pl_ifHkPkt.PlIfObjExecCnt = Pl_if.PlIfObj.ExecCnt;

   CFE_SB_TimeStampMsg((CFE_SB_Msg_t *) &Pl_ifHkPkt);
   CFE_SB_SendMsg((CFE_SB_Msg_t *) &Pl_ifHkPkt);

} /* End PL_IF_SendHousekeepingPkt() */

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

    EXTBL_Constructor(EX_TBL, PLIF_OBJ_GetTblPtr, PLIF_OBJ_LoadTbl, PLIF_OBJ_LoadTblEntry);
    PLIF_OBJ_Constructor(PLIF_OBJ);

    /*
    ** Initialize cFE interfaces 
    */

    CFE_SB_CreatePipe(&Pl_if.CmdPipe, PL_IF_CMD_PIPE_DEPTH, PL_IF_CMD_PIPE_NAME);
    CFE_SB_Subscribe(PL_IF_CMD_MID, Pl_if.CmdPipe);
    CFE_SB_Subscribe(PL_IF_SEND_HK_MID, Pl_if.CmdPipe);

    /*
    ** Initialize App Framework Components 
    */

    CMDMGR_Constructor(CMDMGR_OBJ);
    CMDMGR_RegisterFunc(CMDMGR_OBJ, CMDMGR_NOOP_CMD_FC,  NULL, PL_IF_NoOpCmd,     0);
    CMDMGR_RegisterFunc(CMDMGR_OBJ, CMDMGR_RESET_CMD_FC, NULL, PL_IF_ResetAppCmd, 0);
    
    CMDMGR_RegisterFunc(CMDMGR_OBJ, EXTBL_LOAD_CMD_FC,  TBLMGR_OBJ,  TBLMGR_LoadTblCmd,   TBLMGR_LOAD_TBL_CMD_DATA_LEN);
    CMDMGR_RegisterFunc(CMDMGR_OBJ, EXTBL_DUMP_CMD_FC,  TBLMGR_OBJ,  TBLMGR_DumpTblCmd,   TBLMGR_DUMP_TBL_CMD_DATA_LEN);
    CMDMGR_RegisterFunc(CMDMGR_OBJ, PLIF_OBJ_POWER_CMD_FC, PLIF_OBJ, PLIF_OBJ_PowerCmd,   PLIF_OBJ_POWER_CMD_DATA_LEN);
    CMDMGR_RegisterFunc(CMDMGR_OBJ, PLIF_OBJ_I2C_CMD_FC,   PLIF_OBJ, PLIF_OBJ_i2cCmd,     PLIF_OBJ_I2C_CMD_DATA_LEN);

    TBLMGR_Constructor(TBLMGR_OBJ);
    TBLMGR_RegisterTblWithDef(TBLMGR_OBJ, EXTBL_LoadCmd, EXTBL_DumpCmd, PL_IF_EXTBL_DEF_LOAD_FILE);
                         
    CFE_SB_InitMsg(&Pl_ifHkPkt, PL_IF_TLM_HK_MID, PL_IF_TLM_HK_LEN, TRUE);
                        
    /*
    ** Application startup event message
    */
    Status = CFE_EVS_SendEvent(PL_IF_INIT_INFO_EID,
                               CFE_EVS_INFORMATION,
                               "PL_IF Initialized. Version %d.%d.%d.%d",
                               PL_IF_MAJOR_VERSION,
                               PL_IF_MINOR_VERSION,
                               PL_IF_REVISION,
                               PL_IF_MISSION_REV);

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

   Status = CFE_SB_RcvMsg(&CmdMsgPtr, Pl_if.CmdPipe, CFE_SB_POLL);

   if (Status == CFE_SUCCESS)
   {

      MsgId = CFE_SB_GetMsgId(CmdMsgPtr);

      switch (MsgId)
      {
         case PL_IF_CMD_MID:
            CMDMGR_DispatchFunc(CMDMGR_OBJ, CmdMsgPtr);
            break;

         case PL_IF_SEND_HK_MID:
            PL_IF_SendHousekeepingPkt();
            break;

         default:
            CFE_EVS_SendEvent(PL_IF_CMD_INVALID_MID_ERR_EID, CFE_EVS_ERROR,
                              "Received invalid command packet,MID = 0x%4X",MsgId);

            break;

      } /* End Msgid switch */

   } /* End if SB received a packet */

} /* End ProcessCommands() */


/* end of file */
