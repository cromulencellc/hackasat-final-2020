/*
** Purpose: Implement Eyassat ADCS Configuration Table.
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
** Include Files:
*/

#include <string.h>
#include "eyassat_tbl.h"

/* Convenience macro */
#define  JSON_OBJ  &(EyasSatTbl->Json)

/*
** Type Definitions
*/


/*
** Global File Data
*/

static EYASSATTBL_Class* EyasSatTbl = NULL;

/*
** Local File Function Prototypes
*/

/******************************************************************************
** Function: EntryCallBack
**
** Notes:
**   1. This must have the same function signature as JSON_ContainerFuncPtr.
*/
boolean ConfigCallback (int TokenIdx);
boolean DBCallback (int TokenIdx);
boolean PIDCallback (int TokenIdx);
boolean MagCalCallback (int TokenIdx);
boolean GyroCalCallback (int TokenIdx);

/******************************************************************************
** Function: EYASSATTBL_Constructor
**
** Notes:
**    1. This must be called prior to any other functions
**
*/
void EYASSATTBL_Constructor(EYASSATTBL_Class* ObjPtr,
                       EYASSATTBL_GetTblPtr    GetTblPtrFunc,
                       EYASSATTBL_LoadTbl      LoadTblFunc, 
                       EYASSATTBL_LoadTblEntry LoadTblEntryFunc)
{

   EyasSatTbl = ObjPtr;

   CFE_PSP_MemSet(EyasSatTbl, 0, sizeof(EYASSATTBL_Class));

   EyasSatTbl->GetTblPtrFunc    = GetTblPtrFunc;
   EyasSatTbl->LoadTblFunc      = LoadTblFunc;
   EyasSatTbl->LoadTblEntryFunc = LoadTblEntryFunc; 

   JSON_ObjConstructor(&(EyasSatTbl->JsonObj[EYASSATTBL_OBJ_CONFIG]),
                        "Config",
                        ConfigCallback,
                        (void *)&(EyasSatTbl->Data.Config));

   JSON_ObjConstructor(&(EyasSatTbl->JsonObj[EYASSATTBL_OBJ_DB]),
                        "DB_Coef",
                        DBCallback,
                        (void *)&(EyasSatTbl->Data.DB));
   
    JSON_ObjConstructor(&(EyasSatTbl->JsonObj[EYASSATTBL_OBJ_PID]),
                        "PID_Coef",
                        PIDCallback,
                        (void *)&(EyasSatTbl->Data.PID));

   JSON_ObjConstructor(&(EyasSatTbl->JsonObj[EYASSATTBL_OBJ_MAGCAL]),
                        "MagCal",
                        MagCalCallback,
                        (void *)&(EyasSatTbl->Data.MagCal));

   JSON_ObjConstructor(&(EyasSatTbl->JsonObj[EYASSATTBL_OBJ_GYROCAL]),
                        "GyroCal",
                        GyroCalCallback,
                        (void *)&(EyasSatTbl->Data.GyroCal));

} /* End EYASSATTBL_Constructor() */


/******************************************************************************
** Function: EYASSATTBL_ResetStatus
**
*/
void EYASSATTBL_ResetStatus(void)
{

   int i;

   EyasSatTbl->LastLoadStatus    = TBLMGR_STATUS_UNDEF;
   EyasSatTbl->AttrErrCnt        = 0;
   EyasSatTbl->ObjLoadCnt        = 0;

   for (i=0; i < EYASSATTBL_OBJ_CNT; i++) EyasSatTbl->JsonObj[i].Modified = FALSE;
      
} /* End EYASSATTBL_ResetStatus() */


/******************************************************************************
** Function: EYASSATTBL_LoadCmd
**
** Notes:
**  1. Function signature must match TBLMGR_LoadTblFuncPtr.
**  2. Can assume valid table file name because this is a callback from 
**     the app framework table manager that has verified the file.
*/
boolean EYASSATTBL_LoadCmd(TBLMGR_Tbl *Tbl, uint8 LoadType, const char* Filename)
{

   int i;
   
   OS_printf("EYASSATTBL_LoadCmd() Entry\n");

   /*
   ** Set all data and flags to zero. If a table replace is commanded and
   ** all of the data is not defined the zeroes will be copied into the table. 
   ** Real flight code would validate all data is loaded for a replace.
   */
   
   CFE_PSP_MemSet(&(EyasSatTbl->Data), 0, sizeof(EYASSATTBL_Struct));  /* Wouldn't do in flight but helps debug prototype */
   
   EYASSATTBL_ResetStatus();  /* Reset status helps isolate errors if they occur */

   JSON_Constructor(&(EyasSatTbl->Json), EyasSatTbl->JsonFileBuf, EyasSatTbl->JsonFileTokens);
   
   if (JSON_OpenFile(&(EyasSatTbl->Json), Filename)) {
  
      JSON_RegContainerCallback(JSON_OBJ,
	                            EyasSatTbl->JsonObj[EYASSATTBL_OBJ_CONFIG].Name,
	                            EyasSatTbl->JsonObj[EYASSATTBL_OBJ_CONFIG].Callback);
      JSON_RegContainerCallback(JSON_OBJ,
	                            EyasSatTbl->JsonObj[EYASSATTBL_OBJ_DB].Name,
	                            EyasSatTbl->JsonObj[EYASSATTBL_OBJ_DB].Callback);
      JSON_RegContainerCallback(JSON_OBJ,
	                            EyasSatTbl->JsonObj[EYASSATTBL_OBJ_PID].Name,
	                            EyasSatTbl->JsonObj[EYASSATTBL_OBJ_PID].Callback);
      JSON_RegContainerCallback(JSON_OBJ,
	                            EyasSatTbl->JsonObj[EYASSATTBL_OBJ_MAGCAL].Name,
	                            EyasSatTbl->JsonObj[EYASSATTBL_OBJ_MAGCAL].Callback);
      JSON_RegContainerCallback(JSON_OBJ,
	                            EyasSatTbl->JsonObj[EYASSATTBL_OBJ_GYROCAL].Name,
	                            EyasSatTbl->JsonObj[EYASSATTBL_OBJ_GYROCAL].Callback);

      JSON_ProcessTokens(JSON_OBJ);

      if (LoadType == TBLMGR_LOAD_TBL_REPLACE) {
         
         if (EyasSatTbl->ObjLoadCnt == EYASSATTBL_OBJ_CNT) {
			 
            EyasSatTbl->LastLoadStatus = ((EyasSatTbl->LoadTblFunc)(&(EyasSatTbl->Data)) == TRUE) ? TBLMGR_STATUS_VALID : TBLMGR_STATUS_INVALID;

         } /* End if valid object count */
         else {
            CFE_EVS_SendEvent(EYASSATTBL_CMD_LOAD_REPLACE_ERR_EID,CFE_EVS_ERROR,
			                     "EYASSATTBL: Replace table command rejected. File contained %d objects when expecting %d",
							         EyasSatTbl->ObjLoadCnt, EYASSATTBL_OBJ_CNT);
         } /* End if not all objects loaded for a replace table command */
		 
	   } /* End if replace entire table */
      else if (LoadType == TBLMGR_LOAD_TBL_UPDATE) {
         
         if (EyasSatTbl->ObjLoadCnt > 0 && EyasSatTbl->ObjLoadCnt <= EYASSATTBL_OBJ_CNT) {
			 
		      EyasSatTbl->LastLoadStatus = TBLMGR_STATUS_VALID;
   
            for (i=0; i < EYASSATTBL_OBJ_CNT; i++) {

               if (EyasSatTbl->JsonObj[i].Modified) {
                  // if (!(EyasSatTbl->LoadTblEntryFunc)(i, &(EyasSatTbl->JsonObj[i].Data)))
                  //    EyasSatTbl->LastLoadStatus = TBLMGR_STATUS_INVALID;
                  boolean status = false;
                  switch(i) {
                     case EYASSATTBL_OBJ_CONFIG:
                        status = EyasSatTbl->LoadTblEntryFunc(i, &(EyasSatTbl->Data.Config));
                        break;
                     case EYASSATTBL_OBJ_DB:
                        status = EyasSatTbl->LoadTblEntryFunc(i, &(EyasSatTbl->Data.DB));
                        break;
                     case EYASSATTBL_OBJ_PID:
                        status = EyasSatTbl->LoadTblEntryFunc(i, &(EyasSatTbl->Data.PID));
                        break;
                     case EYASSATTBL_OBJ_MAGCAL:
                        status = EyasSatTbl->LoadTblEntryFunc(i, &(EyasSatTbl->Data.MagCal));
                        break;                                            
                     case EYASSATTBL_OBJ_GYROCAL:
                        status = EyasSatTbl->LoadTblEntryFunc(i, &(EyasSatTbl->Data.GyroCal));
                        break;
                     default:
                        status = false;   
                  }
                  if (!status) {
                     EyasSatTbl->LastLoadStatus = TBLMGR_STATUS_INVALID;
                  }
               }

            } /* End entry loop */
			
         } /* End if valid object count */
         else {
            CFE_EVS_SendEvent(EYASSATTBL_CMD_LOAD_UPDATE_ERR_EID,CFE_EVS_ERROR,
			                  "EYASSATTBL: Update table command rejected. File contained %d objects when expecting 1 to %d",
							  EyasSatTbl->ObjLoadCnt, EYASSATTBL_OBJ_CNT);
         } /* End if not all objects loaded for a replace table command */

      } /* End if update individual records */
      else {
         CFE_EVS_SendEvent(EYASSATTBL_CMD_LOAD_TYPE_ERR_EID,CFE_EVS_ERROR,"EYASSATTBL: Invalid table command load type %d",LoadType);
      }
      
   } /* End if valid file */
   else {
      //printf("**ERROR** Processing CTRL file %s. Status = %d JSMN Status = %d\n",TEST_FILE, Json.FileStatus, Json.JsmnStatus);
      CFE_EVS_SendEvent(EYASSATTBL_CMD_LOAD_TYPE_ERR_EID,CFE_EVS_ERROR,"EYASSATTBL: Table open failure for file %s. File Status = %s JSMN Status = %s",
	                    Filename, JSON_GetFileStatusStr(EyasSatTbl->Json.FileStatus), JSON_GetJsmnErrStr(EyasSatTbl->Json.JsmnStatus));
   }
    
   return (EyasSatTbl->LastLoadStatus == TBLMGR_STATUS_VALID);

} /* End of EYASSATTBL_LoadCmd() */


/******************************************************************************
** Function: EYASSATTBL_DumpCmd
**
** Notes:
**  1. Function signature must match TBLMGR_DumpTblFuncPtr.
**  2. Can assume valid table file name because this is a callback from 
**     the app framework table manager that has verified the file.
**  3. DumpType is unused.
**  4. File is formatted so it can be used as a load file. It does not follow
**     the cFE table file format. 
**  5. Creates a new dump file, overwriting anything that may have existed
**     previously
*/
boolean EYASSATTBL_DumpCmd(TBLMGR_Tbl *Tbl, uint8 DumpType, const char* Filename)
{

   boolean  RetStatus = FALSE;
   int32    FileHandle;
   char     DumpRecord[512];
   const EYASSATTBL_Struct *EyasSatTblPtr;

   FileHandle = OS_creat(Filename, OS_WRITE_ONLY);

   if (FileHandle >= OS_FS_SUCCESS)
   {

      EyasSatTblPtr = (EyasSatTbl->GetTblPtrFunc)();

      sprintf(DumpRecord,"\n{\n\"name\": \"Eyassat ADCS Control Table\",\n");
      OS_write(FileHandle,DumpRecord,strlen(DumpRecord));

      sprintf(DumpRecord,"\"description\": \"Attitude Control Parameters\",\n");
      OS_write(FileHandle,DumpRecord,strlen(DumpRecord));

      /* Config */
      sprintf(DumpRecord,"\"%s\": {\n",EyasSatTbl->JsonObj[EYASSATTBL_OBJ_CONFIG].Name);
      OS_write(FileHandle,DumpRecord,strlen(DumpRecord));
      sprintf(DumpRecord,"   \"Mode\": %d,\n   \"YawCmd\": %f,\n   \"PWM_Baseline\": %d\n},\n",
              EyasSatTblPtr->Config.CtrlMode, EyasSatTblPtr->Config.YawCmd, EyasSatTblPtr->Config.PWM_Baseline);
      OS_write(FileHandle,DumpRecord,strlen(DumpRecord));

      /* DB_Coef */
      sprintf(DumpRecord,"\"%s\": {\n",EyasSatTbl->JsonObj[EYASSATTBL_OBJ_DB].Name);
      OS_write(FileHandle,DumpRecord,strlen(DumpRecord));
      sprintf(DumpRecord,"   \"Deadband\": %f,\n   \"Deadband_ScaleFactor\": %f,\n   \"Extra\": %f\n},\n",
              EyasSatTblPtr->DB.Deadband, EyasSatTblPtr->DB.DeadBandScaleFactor, EyasSatTblPtr->DB.Extra);
      OS_write(FileHandle,DumpRecord,strlen(DumpRecord));

      /* PID_Coef */
      sprintf(DumpRecord,"\"%s\": {\n",EyasSatTbl->JsonObj[EYASSATTBL_OBJ_PID].Name);
      OS_write(FileHandle,DumpRecord,strlen(DumpRecord));
      sprintf(DumpRecord,"   \"Kp\": %f,\n   \"Ki\": %f,\n   \"Kd\": %f\n},\n",
              EyasSatTblPtr->PID.Kp, EyasSatTblPtr->PID.Ki, EyasSatTblPtr->PID.Kd);
      OS_write(FileHandle,DumpRecord,strlen(DumpRecord));

      /* MagCal */
      sprintf(DumpRecord,"\"%s\": {\n",EyasSatTbl->JsonObj[EYASSATTBL_OBJ_MAGCAL].Name);
      OS_write(FileHandle,DumpRecord,strlen(DumpRecord));
      sprintf(DumpRecord,"   \"X\": %f,\n   \"Y\": %f,\n   \"Z\": %f\n},\n",
              EyasSatTblPtr->MagCal.MagX, EyasSatTblPtr->MagCal.MagY, EyasSatTblPtr->MagCal.MagZ);
      OS_write(FileHandle,DumpRecord,strlen(DumpRecord));

      /* GyroCal */
      sprintf(DumpRecord,"\"%s\": {\n",EyasSatTbl->JsonObj[EYASSATTBL_OBJ_GYROCAL].Name);
      OS_write(FileHandle,DumpRecord,strlen(DumpRecord));
      sprintf(DumpRecord,"   \"X\": %f,\n   \"Y\": %f,\n   \"Z\": %f\n},\n",
              EyasSatTblPtr->GyroCal.GyroX, EyasSatTblPtr->GyroCal.GyroY, EyasSatTblPtr->GyroCal.GyroZ);
      OS_write(FileHandle,DumpRecord,strlen(DumpRecord));

      sprintf(DumpRecord,"}\n");
      OS_write(FileHandle,DumpRecord,strlen(DumpRecord));

      RetStatus = TRUE;

      OS_close(FileHandle);

   } /* End if file create */
   else
   {
   
      CFE_EVS_SendEvent(EYASSATTBL_CREATE_FILE_ERR_EID, CFE_EVS_ERROR,
                        "Error creating dump file '%s', Status=0x%08X", Filename, FileHandle);
   
   } /* End if file create error */

   return RetStatus;
   
} /* End of EYASSATTBL_DumpCmd() */

/******************************************************************************
** Function: ConfigCallback
**
** Process an EyasSat ADCS Config table entry
**
** Notes:
**   1. This must have the same function signature as JSON_ContainerFuncPtr.
*/
boolean ConfigCallback (int TokenIdx)
{

   int     ObjCnt=0;
   int     Mode, PWM_Baseline;
   double  YawCmd; //Temporary variables
   boolean RetStatus = FALSE;   
   
   CFE_EVS_SendEvent(EYASSATTBL_LOAD_INIT_EID, CFE_EVS_DEBUG, 
                     "\nEYASSATTBL.ConfigCallback: ObjLoadCnt %d, AttrErrCnt %d, TokenIdx %d\n",
                     EyasSatTbl->ObjLoadCnt, EyasSatTbl->AttrErrCnt, TokenIdx);

   if (JSON_GetValShortInt(JSON_OBJ, TokenIdx, "Mode", &Mode)) ObjCnt++;
   if (JSON_GetValDouble(JSON_OBJ, TokenIdx, "YawCmd", &YawCmd)) ObjCnt++;
   if (JSON_GetValShortInt(JSON_OBJ, TokenIdx, "PWM_Baseline", &PWM_Baseline)) ObjCnt++;
   
   if (ObjCnt == 3) {
   
      EyasSatTbl->Data.Config.CtrlMode = Mode;
      EyasSatTbl->Data.Config.YawCmd = YawCmd;
      EyasSatTbl->Data.Config.PWM_Baseline = PWM_Baseline;

      EyasSatTbl->ObjLoadCnt++;
      RetStatus = TRUE;
      EyasSatTbl->JsonObj[EYASSATTBL_OBJ_CONFIG].Modified = TRUE;
	  
      CFE_EVS_SendEvent(EYASSATTBL_LOAD_INIT_EID, CFE_EVS_INFORMATION,  "EYASSATTBL.ConfigCallback: %d, %f, %d",
                        EyasSatTbl->Data.Config.CtrlMode, EyasSatTbl->Data.Config.YawCmd, EyasSatTbl->Data.Config.PWM_Baseline);
   
   } /* End if ObjCnt == 3 */
   else {
	   
      EyasSatTbl->AttrErrCnt++;     
      CFE_EVS_SendEvent(EYASSATTBL_LOAD_ERR_EID, CFE_EVS_ERROR, "Invalid number of Config entries %d. Should be 6.",
                        ObjCnt);
   
   } /* End if ObjCnt != 3 */
      
   return RetStatus;

} /* ConfigCallback() */

/******************************************************************************
** Function: DBCallback
**
** Process an EyasSat Deadband Config table entry
**
** Notes:
**   1. This must have the same function signature as JSON_ContainerFuncPtr.
*/
boolean DBCallback (int TokenIdx)
{

   int     ObjCnt=0;
   double  Extra, Deadband, Deadband_Scale_Factor; //Temporary variables
   boolean RetStatus = FALSE;   
   
   CFE_EVS_SendEvent(EYASSATTBL_LOAD_INIT_EID, CFE_EVS_DEBUG, 
                     "\nEYASSATTBL.DBCallback: ObjLoadCnt %d, AttrErrCnt %d, TokenIdx %d\n",
                     EyasSatTbl->ObjLoadCnt, EyasSatTbl->AttrErrCnt, TokenIdx);

   if (JSON_GetValDouble(JSON_OBJ, TokenIdx, "Deadband", &Deadband)) ObjCnt++;
   if (JSON_GetValDouble(JSON_OBJ, TokenIdx, "Deadband_ScaleFactor", &Deadband_Scale_Factor)) ObjCnt++;
   if (JSON_GetValDouble(JSON_OBJ, TokenIdx, "Extra", &Extra)) ObjCnt++;

   if (ObjCnt == 3) {
   
      EyasSatTbl->Data.DB.Deadband = Deadband;
      EyasSatTbl->Data.DB.DeadBandScaleFactor = Deadband_Scale_Factor;
      EyasSatTbl->Data.DB.Extra = Extra;

      EyasSatTbl->ObjLoadCnt++;
      RetStatus = TRUE;
      EyasSatTbl->JsonObj[EYASSATTBL_OBJ_DB].Modified = TRUE;
	  
      CFE_EVS_SendEvent(EYASSATTBL_LOAD_INIT_EID, CFE_EVS_INFORMATION,  "EYASSATTBL.DBCallback: %f, %f, %f",
                        EyasSatTbl->Data.DB.Deadband, EyasSatTbl->Data.DB.DeadBandScaleFactor,EyasSatTbl->Data.DB.Extra);
   
   } /* End if ObjCnt == 3 */
   else {
	   
      EyasSatTbl->AttrErrCnt++;     
      CFE_EVS_SendEvent(EYASSATTBL_LOAD_ERR_EID, CFE_EVS_ERROR, "Invalid number of Config entries %d. Should be 3.",
                        ObjCnt);
   
   } /* End if ObjCnt != 3 */
      
   return RetStatus;

} /* DBCallback() */

/******************************************************************************
** Function: PIDCallback
**
** Process an EyasSat PID Config table entry
**
** Notes:
**   1. This must have the same function signature as JSON_ContainerFuncPtr.
*/
boolean PIDCallback (int TokenIdx)
{

   int     ObjCnt=0;
   double  Kp, Ki, Kd; //Temporary variables
   boolean RetStatus = FALSE;   
   
   CFE_EVS_SendEvent(EYASSATTBL_LOAD_INIT_EID, CFE_EVS_DEBUG, 
                     "\nEYASSATTBL.PIDCallback: ObjLoadCnt %d, AttrErrCnt %d, TokenIdx %d\n",
                     EyasSatTbl->ObjLoadCnt, EyasSatTbl->AttrErrCnt, TokenIdx);

   if (JSON_GetValDouble(JSON_OBJ, TokenIdx, "Kp", &Kp)) ObjCnt++;
   if (JSON_GetValDouble(JSON_OBJ, TokenIdx, "Ki", &Ki)) ObjCnt++;
   if (JSON_GetValDouble(JSON_OBJ, TokenIdx, "Kd", &Kd)) ObjCnt++;
   
   if (ObjCnt == 3) {
   
      EyasSatTbl->Data.PID.Kp = Kp;
      EyasSatTbl->Data.PID.Ki = Ki;
      EyasSatTbl->Data.PID.Kd = Kd;

      EyasSatTbl->ObjLoadCnt++;
      RetStatus = TRUE;
      EyasSatTbl->JsonObj[EYASSATTBL_OBJ_PID].Modified = TRUE;

	  
      CFE_EVS_SendEvent(EYASSATTBL_LOAD_INIT_EID, CFE_EVS_INFORMATION,  "EYASSATTBL.ConfigCallback: %f, %f, %f",
                        EyasSatTbl->Data.PID.Kp, EyasSatTbl->Data.PID.Ki, EyasSatTbl->Data.PID.Kd);
   
   } /* End if ObjCnt == 3 */
   else {
	   
      EyasSatTbl->AttrErrCnt++;     
      CFE_EVS_SendEvent(EYASSATTBL_LOAD_ERR_EID, CFE_EVS_ERROR, "Invalid number of Config entries %d. Should be 3.",
                        ObjCnt);
   
   } /* End if ObjCnt != 3 */
      
   return RetStatus;

} /* PIDCallback() */

/******************************************************************************
** Function: MagCalCallback
**
** Process an EyasSat ADCS Magnetometer Calibration table entry
**
** Notes:
**   1. This must have the same function signature as JSON_ContainerFuncPtr.
*/
boolean MagCalCallback (int TokenIdx)
{

   int     ObjCnt=0;
   double  x, y, z; //Temporary variables
   boolean RetStatus = FALSE;   
   
   CFE_EVS_SendEvent(EYASSATTBL_LOAD_INIT_EID, CFE_EVS_DEBUG, 
                     "\nEYASSATTBL.MagCalCallback: ObjLoadCnt %d, AttrErrCnt %d, TokenIdx %d\n",
                     EyasSatTbl->ObjLoadCnt, EyasSatTbl->AttrErrCnt, TokenIdx);

   if (JSON_GetValDouble(JSON_OBJ, TokenIdx, "X", &x)) ObjCnt++;
   if (JSON_GetValDouble(JSON_OBJ, TokenIdx, "Y", &y)) ObjCnt++;
   if (JSON_GetValDouble(JSON_OBJ, TokenIdx, "Z", &z)) ObjCnt++;
   
   if (ObjCnt == 3) {
   
      EyasSatTbl->Data.MagCal.MagX = x;
      EyasSatTbl->Data.MagCal.MagY = y;
      EyasSatTbl->Data.MagCal.MagZ = z;

      EyasSatTbl->ObjLoadCnt++;
      RetStatus = TRUE;
      EyasSatTbl->JsonObj[EYASSATTBL_OBJ_MAGCAL].Modified = TRUE;
	  
      CFE_EVS_SendEvent(EYASSATTBL_LOAD_INIT_EID, CFE_EVS_INFORMATION,  "EYASSATTBL.MagCalCallback: %f, %f, %f",
                        EyasSatTbl->Data.MagCal.MagX,EyasSatTbl->Data.MagCal.MagY,EyasSatTbl->Data.MagCal.MagZ);
   
   } /* End if ObjCnt == 3 */
   else {
	   
      EyasSatTbl->AttrErrCnt++;     
      CFE_EVS_SendEvent(EYASSATTBL_LOAD_ERR_EID, CFE_EVS_ERROR, "Invalid number of Config entries %d. Should be 3.",
                        ObjCnt);
   
   } /* End if ObjCnt != 3 */
      
   return RetStatus;

} /* MagCalCallback() */

/******************************************************************************
** Function: GyroCalCallback
**
** Process an EyasSat ADCS Gyro Calibration table entry
**
** Notes:
**   1. This must have the same function signature as JSON_ContainerFuncPtr.
*/
boolean GyroCalCallback (int TokenIdx)
{

   int     ObjCnt=0;
   double  x, y, z; //Temporary variables
   boolean RetStatus = FALSE;   
   
   CFE_EVS_SendEvent(EYASSATTBL_LOAD_INIT_EID, CFE_EVS_DEBUG, 
                     "\nEYASSATTBL.GyroCallback: ObjLoadCnt %d, AttrErrCnt %d, TokenIdx %d\n",
                     EyasSatTbl->ObjLoadCnt, EyasSatTbl->AttrErrCnt, TokenIdx);

   if (JSON_GetValDouble(JSON_OBJ, TokenIdx, "X", &x)) ObjCnt++;
   if (JSON_GetValDouble(JSON_OBJ, TokenIdx, "Y", &y)) ObjCnt++;
   if (JSON_GetValDouble(JSON_OBJ, TokenIdx, "Z", &z)) ObjCnt++;
   
   if (ObjCnt == 3) {
   
      EyasSatTbl->Data.GyroCal.GyroX = x;
      EyasSatTbl->Data.GyroCal.GyroY = y;
      EyasSatTbl->Data.GyroCal.GyroZ = z;

      EyasSatTbl->ObjLoadCnt++;
      RetStatus = TRUE;
      EyasSatTbl->JsonObj[EYASSATTBL_OBJ_GYROCAL].Modified = TRUE;
	  
      CFE_EVS_SendEvent(EYASSATTBL_LOAD_INIT_EID, CFE_EVS_INFORMATION,  "EYASSATTBL.GyroCalCallback: %f, %f, %f",
                        EyasSatTbl->Data.GyroCal.GyroX,EyasSatTbl->Data.GyroCal.GyroY,EyasSatTbl->Data.GyroCal.GyroZ);
   
   } /* End if ObjCnt == 3 */
   else {
	   
      EyasSatTbl->AttrErrCnt++;     
      CFE_EVS_SendEvent(EYASSATTBL_LOAD_ERR_EID, CFE_EVS_ERROR, "Invalid number of Config entries %d. Should be 3.",
                        ObjCnt);
   
   } /* End if ObjCnt != 3 */
      
   return RetStatus;

} /* GyroCalCallback() */

/* end of file */
