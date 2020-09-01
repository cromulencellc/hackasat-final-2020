/* 
** Purpose: Implement a Eyassat_if application.
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
#include "eyassat_if_app.h"


/*
** Local Function Prototypes
*/

static int32 InitApp(void);
static void ProcessCommands(void);

/*
** Global Data
*/

EYASSAT_IF_Class  Eyassat_if;
EYASSAT_IF_HkPkt  Eyassat_ifHkPkt;
EYASSAT_IF_CalTblPkt Eyassat_ifCalTblPkt;


/*
** Convenience Macros
*/

#define  CMDMGR_OBJ      (&(Eyassat_if.CmdMgr))
#define  TBLMGR_OBJ      (&(Eyassat_if.TblMgr))
#define  EYASSAT_OBJ     (&(Eyassat_if.EyasSatObj))
#define  EYASSAT_TBL     (&(Eyassat_if.EyasSatTbl))

/******************************************************************************
** Function: EYASSAT_IF_AppMain
**
*/
void EYASSAT_IF_AppMain(void)
{

   int32  Status    = CFE_SEVERITY_ERROR;
   uint32 RunStatus = CFE_ES_APP_ERROR;


   CFE_ES_PerfLogEntry(EYASSAT_IF_MAIN_PERF_ID);
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
     
	   CFE_ES_PerfLogExit(EYASSAT_IF_MAIN_PERF_ID);
      OS_TaskDelay(EYASSAT_IF_RUNLOOP_DELAY);
      CFE_ES_PerfLogEntry(EYASSAT_IF_MAIN_PERF_ID);
 
      EYASSATOBJ_Execute();

      // Loading table after some time to allow power to be applied to the adcs before setting them and thus overriding them with Eyassat FW defaults
      if ((Eyassat_if.ExecCnt == 60) && !(Eyassat_if.EyasSatObj.tableInit)) {
         TBLMGR_RegisterTblWithDef(TBLMGR_OBJ, EYASSATTBL_LoadCmd, EYASSATTBL_DumpCmd, EYASSAT_IF_ADCS_LOAD_FILE);
         Eyassat_if.EyasSatObj.tableInit = true;
      }

      ProcessCommands();
      Eyassat_if.ExecCnt++;

   } /* End CFE_ES_RunLoop */


   /* Write to system log in case events not working */

   CFE_ES_WriteToSysLog("EYASSAT_IF Terminating, RunLoop status = 0x%08X\n", RunStatus);

   CFE_EVS_SendEvent(EYASSAT_IF_EXIT_ERR_EID, CFE_EVS_CRITICAL, "EYASSAT_IF Terminating,  RunLoop status = 0x%08X", RunStatus);

   CFE_ES_PerfLogExit(EYASSAT_IF_MAIN_PERF_ID);
   CFE_ES_ExitApp(RunStatus);  /* Let cFE kill the task (and any child tasks) */

} /* End of EYASSAT_IF_Main() */


/******************************************************************************
** Function: EYASSAT_IF_NoOpCmd
**
** Function signature must match CMDMGR_CmdFuncPtr typedef 
*/

boolean EYASSAT_IF_NoOpCmd(void* DataObjPtr, const CFE_SB_MsgPtr_t MsgPtr)
{

   CFE_EVS_SendEvent (EYASSAT_IF_CMD_NOOP_INFO_EID,
                      CFE_EVS_INFORMATION,
                      "No operation command received for EYASSAT_IF version %d.%d",
                      EYASSAT_IF_MAJOR_VERSION,EYASSAT_IF_MINOR_VERSION);

   return TRUE;


} /* End EYASSAT_IF_NoOpCmd() */


/******************************************************************************
** Function: EYASSAT_IF_ResetAppCmd
**
** Function signature must match CMDMGR_CmdFuncPtr typedef 
*/

boolean EYASSAT_IF_ResetAppCmd(void* DataObjPtr, const CFE_SB_MsgPtr_t MsgPtr)
{

   CMDMGR_ResetStatus(CMDMGR_OBJ);
   TBLMGR_ResetStatus(TBLMGR_OBJ);
   EYASSATTBL_ResetStatus();
   EYASSATOBJ_ResetStatus();

   return TRUE;

} /* End EYASSAT_IF_ResetAppCmd() */

/******************************************************************************
** Function: EYASSAT_IF_SendCalTblPkt
**
*/
void EYASSAT_IF_SendCalTblPkt(void)
{

   Eyassat_ifCalTblPkt.MagX = EYASSAT_TBL->Data.MagCal.MagX;
   Eyassat_ifCalTblPkt.MagY = EYASSAT_TBL->Data.MagCal.MagY;
   Eyassat_ifCalTblPkt.MagZ = EYASSAT_TBL->Data.MagCal.MagZ;

   Eyassat_ifCalTblPkt.GyroX = EYASSAT_TBL->Data.GyroCal.GyroX;
   Eyassat_ifCalTblPkt.GyroY = EYASSAT_TBL->Data.GyroCal.GyroY;
   Eyassat_ifCalTblPkt.GyroZ = EYASSAT_TBL->Data.GyroCal.GyroZ;

   CFE_SB_TimeStampMsg((CFE_SB_Msg_t *) &Eyassat_ifCalTblPkt);
   CFE_SB_SendMsg((CFE_SB_Msg_t *) &Eyassat_ifCalTblPkt);

}

/******************************************************************************
** Function: EYASSAT_IF_SendHousekeepingPkt
**
*/
void EYASSAT_IF_SendHousekeepingPkt(void)
{

   /* Good design practice in case app expands to more than one table */
   const TBLMGR_Tbl* LastTbl = TBLMGR_GetLastTblStatus(TBLMGR_OBJ);

   
   /*
   ** CMDMGR Data
   */

   Eyassat_ifHkPkt.ValidCmdCnt   = Eyassat_if.CmdMgr.ValidCmdCnt;
   Eyassat_ifHkPkt.InvalidCmdCnt = Eyassat_if.CmdMgr.InvalidCmdCnt;

   
   /*
   ** EXTBL/EXOBJ Data
   ** - At a minimum all OBJECT variables effected by a reset must be included
   */

   Eyassat_ifHkPkt.LastAction       = LastTbl->LastAction;
   Eyassat_ifHkPkt.LastActionStatus = LastTbl->LastActionStatus;

   Eyassat_ifHkPkt.EyasSatObjExecCnt = Eyassat_if.EyasSatObj.ExecCnt;

   Eyassat_ifHkPkt.CmdBufferHead = Eyassat_if.EyasSatObj.CmdBufferHead;
   Eyassat_ifHkPkt.CmdBufferTail = Eyassat_if.EyasSatObj.CmdBufferTail;

   CFE_SB_TimeStampMsg((CFE_SB_Msg_t *) &Eyassat_ifHkPkt);
   CFE_SB_SendMsg((CFE_SB_Msg_t *) &Eyassat_ifHkPkt);

   // Send Internal and Temps Packets with HK
   CFE_SB_SendMsg((CFE_SB_Msg_t *) &(Eyassat_if.EyasSatObj.InternalPkt));
   CFE_SB_SendMsg((CFE_SB_Msg_t *) &(Eyassat_if.EyasSatObj.TempPkt));


} /* End EYASSAT_IF_SendHousekeepingPkt() */

/******************************************************************************
** Function: EYASSAT_IF_SendPowerPkt
**
*/
void EYASSAT_IF_SendPowerPkt(void)
{
   if(Eyassat_if.EyasSatObj.PowerTlmUpdate) {
      CFE_SB_SendMsg((CFE_SB_Msg_t *) &(Eyassat_if.EyasSatObj.PowerPkt));
      Eyassat_if.EyasSatObj.PowerTlmUpdate = false;
   }
}

/******************************************************************************
** Function: EYASSAT_IF_SendAdcsPkt
**
*/
void EYASSAT_IF_SendAdcsPkt(void)
{
   if(Eyassat_if.EyasSatObj.AdcsTlmUpdate) {
      CFE_SB_SendMsg((CFE_SB_Msg_t *) &(Eyassat_if.EyasSatObj.ADCSPkt));
      Eyassat_if.EyasSatObj.AdcsTlmUpdate = false;
   }
}

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

    EYASSATTBL_Constructor(EYASSAT_TBL, EYASSATOBJ_GetTblPtr, EYASSATOBJ_LoadTbl, EYASSATOBJ_LoadTblEntry);
    EYASSATOBJ_Constructor(EYASSAT_OBJ);

    /*
    ** Initialize cFE interfaces 
    */

    CFE_SB_CreatePipe(&Eyassat_if.CmdPipe, EYASSAT_IF_CMD_PIPE_DEPTH, EYASSAT_IF_CMD_PIPE_NAME);
    CFE_SB_Subscribe(EYASSAT_IF_CMD_MID, Eyassat_if.CmdPipe);
    CFE_SB_Subscribe(EYASSAT_IF_SEND_HK_MID, Eyassat_if.CmdPipe);
    CFE_SB_Subscribe(EYASSAT_IF_SEND_POWER_MID, Eyassat_if.CmdPipe);
    CFE_SB_Subscribe(EYASSAT_IF_SEND_ADCS_MID, Eyassat_if.CmdPipe);


    /*
    ** Initialize App Framework Components 
    */

    CMDMGR_Constructor(CMDMGR_OBJ);
    CMDMGR_RegisterFunc(CMDMGR_OBJ, CMDMGR_NOOP_CMD_FC,  NULL, EYASSAT_IF_NoOpCmd,     0);
    CMDMGR_RegisterFunc(CMDMGR_OBJ, CMDMGR_RESET_CMD_FC, NULL, EYASSAT_IF_ResetAppCmd, 0);
    
    CMDMGR_RegisterFunc(CMDMGR_OBJ, EYASSATTBL_LOAD_CMD_FC,  TBLMGR_OBJ,      TBLMGR_LoadTblCmd,      TBLMGR_LOAD_TBL_CMD_DATA_LEN);
    CMDMGR_RegisterFunc(CMDMGR_OBJ, EYASSATTBL_DUMP_CMD_FC,  TBLMGR_OBJ,      TBLMGR_DumpTblCmd,      TBLMGR_DUMP_TBL_CMD_DATA_LEN);
    CMDMGR_RegisterFunc(CMDMGR_OBJ, EYASSATOBJ_DISCRETE_CMD_FC,  EYASSAT_OBJ, EYASSATOBJ_DiscreteCmd,     EYASSATOBJ_DISCRETE_CMD_DATA_LEN);
    CMDMGR_RegisterFunc(CMDMGR_OBJ, EYASSATOBJ_UINT8_CMD_FC,   EYASSAT_OBJ,   EYASSATOBJ_Uint8Cmd,     EYASSATOBJ_UINT8_CMD_DATA_LEN);
    CMDMGR_RegisterFunc(CMDMGR_OBJ, EYASSATOBJ_UINT16_CMD_FC,  EYASSAT_OBJ,   EYASSATOBJ_Uint16Cmd,    EYASSATOBJ_UINT16_CMD_DATA_LEN);
    CMDMGR_RegisterFunc(CMDMGR_OBJ, EYASSATOBJ_FLOAT_CMD_FC,  EYASSAT_OBJ,   EYASSATOBJ_FloatCmd,    EYASSATOBJ_FLOAT_CMD_DATA_LEN);
    CMDMGR_RegisterFunc(CMDMGR_OBJ, EYASSATOBJ_CONNECT_CMD_FC,  EYASSAT_OBJ,   EYASSATOBJ_ConnectCmd,    EYASSATOBJ_CONNECT_CMD_DATA_LEN);
    CMDMGR_RegisterFunc(CMDMGR_OBJ, EYASSATOBJ_DISCONNECT_CMD_FC,  EYASSAT_OBJ,   EYASSATOBJ_DisconnectCmd,    EYASSATOBJ_DISCONNECT_CMD_DATA_LEN);
    CMDMGR_RegisterFunc(CMDMGR_OBJ, EYASSATOBJ_MAG_CAL_CMD_FC,  EYASSAT_OBJ,   EYASSATOBJ_MagCalCmd,  EYASSATOBJ_MAGCAL_CMD_DATA_LEN);
    CMDMGR_RegisterFunc(CMDMGR_OBJ, EYASSATOBJ_GYRO_CAL_CMD_FC, EYASSAT_OBJ,   EYASSATOBJ_GyroCalCmd, EYASSATOBJ_GYROCAL_CMD_DATA_LEN);
    // Treating separately for the game.  We'll be locking out most adcs commands but we want them to be able to dump momentum
    CMDMGR_RegisterFunc(CMDMGR_OBJ, EYASSATOBJ_TORQUE_ROD_CMD_FC, EYASSAT_OBJ,   EYASSATOBJ_Uint16Cmd, EYASSATOBJ_UINT16_CMD_DATA_LEN);


    TBLMGR_Constructor(TBLMGR_OBJ);
                         
    CFE_SB_InitMsg(&Eyassat_ifHkPkt, EYASSAT_IF_TLM_HK_MID, EYASSAT_IF_TLM_HK_LEN, TRUE);
    CFE_SB_InitMsg(&Eyassat_ifCalTblPkt, EYASSAT_IF_TLM_CAL_TBL_MID, EYASSAT_IF_TLM_CAL_TBL_LEN, TRUE);


    /*
    ** Application startup event message
    */
    Status = CFE_EVS_SendEvent(EYASSAT_IF_INIT_INFO_EID,
                               CFE_EVS_INFORMATION,
                               "EYASSAT_IF Initialized. Version %d.%d.%d.%d",
                               EYASSAT_IF_MAJOR_VERSION,
                               EYASSAT_IF_MINOR_VERSION,
                               EYASSAT_IF_REVISION,
                               EYASSAT_IF_MISSION_REV);

    Eyassat_if.ExecCnt = 0;

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

   Status = CFE_SB_RcvMsg(&CmdMsgPtr, Eyassat_if.CmdPipe, CFE_SB_POLL);

   if (Status == CFE_SUCCESS)
   {

      MsgId = CFE_SB_GetMsgId(CmdMsgPtr);

      switch (MsgId)
      {
         case EYASSAT_IF_CMD_MID:
            CMDMGR_DispatchFunc(CMDMGR_OBJ, CmdMsgPtr);
            break;

         case EYASSAT_IF_SEND_HK_MID:
            EYASSAT_IF_SendHousekeepingPkt();
            EYASSAT_IF_SendCalTblPkt();
            break;

         case EYASSAT_IF_SEND_POWER_MID:
            EYASSAT_IF_SendPowerPkt();
            break;

         case EYASSAT_IF_SEND_ADCS_MID:
            EYASSAT_IF_SendAdcsPkt();
            break;

         default:
            CFE_EVS_SendEvent(EYASSAT_IF_CMD_INVALID_MID_ERR_EID, CFE_EVS_ERROR,
                              "Received invalid command packet,MID = 0x%4X",MsgId);

            break;

      } /* End Msgid switch */

   } /* End if SB received a packet */

} /* End ProcessCommands() */


/* end of file */
