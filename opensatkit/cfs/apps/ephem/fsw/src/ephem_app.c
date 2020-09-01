/* 
** Purpose: Implement a Ephem application.
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
#include "ephem_app.h"


/*
** Local Function Prototypes
*/

static int32 InitApp(void);
static void ProcessCommands(void);
void EPHEM_SendEphemPkt(void);
/*
** Global Data
*/

EPHEM_Class  Ephem;
EPHEM_HkPkt  EphemHkPkt;
EPHEM_EphemPkt EphemPkt;
/*
** Convenience Macros
*/

#define  CMDMGR_OBJ (&(Ephem.CmdMgr))
#define  TBLMGR_OBJ (&(Ephem.TblMgr))
#define  EX_OBJ     (&(Ephem.ExObj))
#define  EX_TBL     (&(Ephem.ExTbl))

/******************************************************************************
** Function: EPHEM_AppMain
**
*/
void EPHEM_AppMain(void)
{

   int32  Status    = CFE_SEVERITY_ERROR;
   uint32 RunStatus = CFE_ES_APP_ERROR;


   CFE_ES_PerfLogEntry(EPHEM_MAIN_PERF_ID);
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
	  
	  CFE_ES_PerfLogExit(EPHEM_MAIN_PERF_ID);
      OS_TaskDelay(EPHEM_RUNLOOP_DELAY);
      CFE_ES_PerfLogEntry(EPHEM_MAIN_PERF_ID);

      EXOBJ_Execute();

      EPHEM_SendEphemPkt();

      ProcessCommands();

   } /* End CFE_ES_RunLoop */


   /* Write to system log in case events not working */

   CFE_ES_WriteToSysLog("EPHEM Terminating, RunLoop status = 0x%08X\n", RunStatus);

   CFE_EVS_SendEvent(EPHEM_EXIT_ERR_EID, CFE_EVS_CRITICAL, "EPHEM Terminating,  RunLoop status = 0x%08X", RunStatus);

   CFE_ES_PerfLogExit(EPHEM_MAIN_PERF_ID);
   CFE_ES_ExitApp(RunStatus);  /* Let cFE kill the task (and any child tasks) */

} /* End of EPHEM_Main() */


/******************************************************************************
** Function: EPHEM_NoOpCmd
**
** Function signature must match CMDMGR_CmdFuncPtr typedef 
*/

boolean EPHEM_NoOpCmd(void* DataObjPtr, const CFE_SB_MsgPtr_t MsgPtr)
{

   CFE_EVS_SendEvent (EPHEM_CMD_NOOP_INFO_EID,
                      CFE_EVS_INFORMATION,
                      "No operation command received for EPHEM version %d.%d",
                      EPHEM_MAJOR_VERSION,EPHEM_MINOR_VERSION);

   return TRUE;


} /* End EPHEM_NoOpCmd() */


/******************************************************************************
** Function: EPHEM_ResetAppCmd
**
** Function signature must match CMDMGR_CmdFuncPtr typedef 
*/

boolean EPHEM_ResetAppCmd(void* DataObjPtr, const CFE_SB_MsgPtr_t MsgPtr)
{

   CMDMGR_ResetStatus(CMDMGR_OBJ);
   TBLMGR_ResetStatus(TBLMGR_OBJ);
   EXTBL_ResetStatus();
   EXOBJ_ResetStatus();

   return TRUE;

} /* End EPHEM_ResetAppCmd() */


/******************************************************************************
** Function: EPHEM_SendHousekeepingPkt
**
*/
void EPHEM_SendHousekeepingPkt(void)
{

   /* Good design practice in case app expands to more than one table */
   const TBLMGR_Tbl* LastTbl = TBLMGR_GetLastTblStatus(TBLMGR_OBJ);

   
   /*
   ** CMDMGR Data
   */

   EphemHkPkt.ValidCmdCnt   = Ephem.CmdMgr.ValidCmdCnt;
   EphemHkPkt.InvalidCmdCnt = Ephem.CmdMgr.InvalidCmdCnt;

   
   /*
   ** EXTBL/EXOBJ Data
   ** - At a minimum all OBJECT variables effected by a reset must be included
   */

   EphemHkPkt.LastAction       = LastTbl->LastAction;
   EphemHkPkt.LastActionStatus = LastTbl->LastActionStatus;

   EphemHkPkt.ExObjExecCnt = Ephem.ExObj.ExecCnt;

   CFE_SB_TimeStampMsg((CFE_SB_Msg_t *) &EphemHkPkt);
   CFE_SB_SendMsg((CFE_SB_Msg_t *) &EphemHkPkt);

} /* End EPHEM_SendHousekeepingPkt() */

/******************************************************************************
** Function: EPHEM_SendEphemPkt
**
*/
void EPHEM_SendEphemPkt(void)
{ 
   strncpy(EphemPkt.TimeString,Ephem.ExObj.TimeString,sizeof(EphemPkt.TimeString));
   EphemPkt.AbsoluteTimeOffset = Ephem.ExObj.AbsoluteTimeOffset;
   EphemPkt.AboluteTimeEpoch = Ephem.ExObj.AbsoluteTimeEpoch;
   EphemPkt.AbsoluteTime = Ephem.ExObj.AbsoluteTime;
   EphemPkt.PosN_X = Ephem.ExObj.Pos[0];
   EphemPkt.PosN_Y = Ephem.ExObj.Pos[1];
   EphemPkt.PosN_Z = Ephem.ExObj.Pos[2];
   EphemPkt.VelN_X = Ephem.ExObj.Vel[0];
   EphemPkt.VelN_Y = Ephem.ExObj.Vel[1];
   EphemPkt.VelN_Z = Ephem.ExObj.Vel[2];

   CFE_SB_TimeStampMsg((CFE_SB_Msg_t *) &EphemPkt);
   CFE_SB_SendMsg((CFE_SB_Msg_t *) &EphemPkt);

} /* End EPHEM_SendEphemPkt() */

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

    EXTBL_Constructor(EX_TBL, EXOBJ_GetTblPtr, EXOBJ_LoadTbl, EXOBJ_LoadTblEntry);
    EXOBJ_Constructor(EX_OBJ);

    /*
    ** Initialize cFE interfaces 
    */

    CFE_SB_CreatePipe(&Ephem.CmdPipe, EPHEM_CMD_PIPE_DEPTH, EPHEM_CMD_PIPE_NAME);
    CFE_SB_Subscribe(EPHEM_CMD_MID, Ephem.CmdPipe);
    CFE_SB_Subscribe(EPHEM_SEND_HK_MID, Ephem.CmdPipe);

    /*
    ** Initialize App Framework Components 
    */

    CMDMGR_Constructor(CMDMGR_OBJ);
    CMDMGR_RegisterFunc(CMDMGR_OBJ, CMDMGR_NOOP_CMD_FC,  NULL, EPHEM_NoOpCmd,     0);
    CMDMGR_RegisterFunc(CMDMGR_OBJ, CMDMGR_RESET_CMD_FC, NULL, EPHEM_ResetAppCmd, 0);
    
    CMDMGR_RegisterFunc(CMDMGR_OBJ, EXTBL_LOAD_CMD_FC,  TBLMGR_OBJ, TBLMGR_LoadTblCmd, TBLMGR_LOAD_TBL_CMD_DATA_LEN);
    CMDMGR_RegisterFunc(CMDMGR_OBJ, EXTBL_DUMP_CMD_FC,  TBLMGR_OBJ, TBLMGR_DumpTblCmd, TBLMGR_DUMP_TBL_CMD_DATA_LEN);
    CMDMGR_RegisterFunc(CMDMGR_OBJ, EXOBJ_DEMO_CMD_FC,  EX_OBJ,     EXOBJ_DemoCmd,     EXOBJ_DEMO_CMD_DATA_LEN);

    TBLMGR_Constructor(TBLMGR_OBJ);
    TBLMGR_RegisterTblWithDef(TBLMGR_OBJ, EXTBL_LoadCmd, EXTBL_DumpCmd, EPHEM_EXTBL_DEF_LOAD_FILE);
                         
    CFE_SB_InitMsg(&EphemHkPkt, EPHEM_TLM_HK_MID, EPHEM_TLM_HK_LEN, TRUE);
    CFE_SB_InitMsg(&EphemPkt, EPHEM_TLM_EPHEM_MID, EPHEM_TLM_EPHEM_LEN, TRUE);

                        
    /*
    ** Application startup event message
    */
    Status = CFE_EVS_SendEvent(EPHEM_INIT_INFO_EID,
                               CFE_EVS_INFORMATION,
                               "EPHEM Initialized. Version %d.%d.%d.%d",
                               EPHEM_MAJOR_VERSION,
                               EPHEM_MINOR_VERSION,
                               EPHEM_REVISION,
                               EPHEM_MISSION_REV);

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

   Status = CFE_SB_RcvMsg(&CmdMsgPtr, Ephem.CmdPipe, CFE_SB_POLL);

   if (Status == CFE_SUCCESS)
   {

      MsgId = CFE_SB_GetMsgId(CmdMsgPtr);

      switch (MsgId)
      {
         case EPHEM_CMD_MID:
            CMDMGR_DispatchFunc(CMDMGR_OBJ, CmdMsgPtr);
            break;

         case EPHEM_SEND_HK_MID:
            EPHEM_SendHousekeepingPkt();
            break;

         default:
            CFE_EVS_SendEvent(EPHEM_CMD_INVALID_MID_ERR_EID, CFE_EVS_ERROR,
                              "Received invalid command packet,MID = 0x%4X",MsgId);

            break;

      } /* End Msgid switch */

   } /* End if SB received a packet */

} /* End ProcessCommands() */


/* end of file */
