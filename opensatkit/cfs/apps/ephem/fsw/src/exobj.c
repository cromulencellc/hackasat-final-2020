/*
** Purpose: Implement an example object.
**
** Notes:
**   1. This serves as an example object that uses a table. It does not perform
**      any realistic funcions.
**
** License:
**   Template written by David McComas and licensed under the GNU
**   Lesser General Public License (LGPL).
**
** References:
**   1. OpenSatKit Object-based Application Developers Guide.
**   2. cFS Application Developer's Guide.
*/

/*
** Include Files:
*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

#include "app_cfg.h"
#include "exobj.h"
#include "TLE.h"
#include "timekit.h"

/*
** Global File Data
*/

static EXOBJ_Class*  ExObj = NULL;

/*
** Local Function Prototypes
*/


/******************************************************************************
** Function: EXOBJ_Constructor
**
*/
void EXOBJ_Constructor(EXOBJ_Class*  ExObjPtr)
{
 
   ExObj = ExObjPtr;

   CFE_PSP_MemSet((void*)ExObj, 0, sizeof(EXOBJ_Class));

   ExObj->DT = EPHEM_RUNLOOP_DELAY/1000; //seconds

   EXOBJ_Init();

    
} /* End EXOBJ_Constructor() */

/******************************************************************************
** Function: EXOBJ_Init
**
** Open TLE and get initial state
**
*/
void EXOBJ_Init(void) {

   // FILE *fp;
   int32 Fd = -1;
   char line1[80];
   char line2[80];
   char file[256];

   int readPtr  = 0;
   int readSize = 1;


   memset(line1,'\0',sizeof(line1));
   memset(line2,'\0',sizeof(line2));
   memset(file,'\0',sizeof(file));

   if (AppFw_VerifyFileForRead(TLE_FILE_PATH)) {
      Fd = OS_open(TLE_FILE_PATH, OS_READ_ONLY, 0);
   }

   if (Fd >= 0) {
      while(readSize > 0) {
         readSize = OS_read(Fd,&file[readPtr],1);
         readPtr += readSize;
      }
      if (file[0] == '1') {
         strncpy(line1,file,69);
      }
      if (file[71] == '2'){
         strncpy(line2,file+71,69);
      }

      if (tle_checksum(line1,line2)) {
         CFE_EVS_SendEvent(EXOBJ_INFO_EID, CFE_EVS_INFORMATION, "Valid TLE file.  Loading initial state");
         calculateEpochAbsoluteTime(line1); 
         parseLines(&(ExObj->tle),line1,line2);
         getRV(&(ExObj->tle),ExObj->DT,ExObj->Pos,ExObj->Vel);
      }
      else {
         CFE_EVS_SendEvent(EXOBJ_ERROR_EID, CFE_EVS_ERROR, "Invalid TLE file %s", TLE_FILE_PATH);         
      }

      OS_close(Fd);
   } else
   {
      CFE_EVS_SendEvent(EXOBJ_FILE_OPEN_EID, CFE_EVS_ERROR, "File open error for %s",
                        TLE_FILE_PATH);
   }

}

/******************************************************************************
** Function: EXOBJ_Execute
**
** Execute main object function.
**
*/
void EXOBJ_Execute(void)
{

   EXOBJ_PropogateOrbit();
   ExObj->ExecCnt++;

} /* EXOBJ_Execute() */


/******************************************************************************
** Function: EXOBJ_PropogateOrbit
**
** Propogate orbit based on delta from initial orbit epooch in mins
**
*/
void EXOBJ_PropogateOrbit(void) {
   
   long doy,Year,Month,Day,Hour,Minute;
   double Second;
   double minutesAfterEpoch = 0.0;

   memset(ExObj->TimeString,'\0',sizeof(ExObj->TimeString));

   //Using run loop delay as DT now.  Would be better if this was based on the cFS clock
   ExObj->AbsoluteTime += ExObj->DT; //seconds since J2000 Epoch from TLE
   minutesAfterEpoch = (ExObj->AbsoluteTime - ExObj->AbsoluteTimeEpoch)/60;

   AbsTimeToDate(ExObj->AbsoluteTime,&Year,&Month,&Day,&Hour,&Minute,&Second,ExObj->DT);
   doy = MD2DOY(Year,Month,Day);
   //YYYY-DDD-HH:MM:SS.SSS
   // sprintf(s,"%04li-%03li-%02li:%02li:%05.3f",Year,doy,Hour,Minute,Second);
   //YYYY-MM-DD-HH:MM:SS.SSS
   sprintf(ExObj->TimeString,"%04li-%02li-%02li-%02li:%02li:%05.3f",Year,Month,Day,Hour,Minute,Second);

   //OS_printf("Time since epoch: %f\n", ExObj->AbsoluteTime - ExObj->AbsoluteTimeEpoch);
   getRV(&(ExObj->tle),minutesAfterEpoch,ExObj->Pos,ExObj->Vel);

}

/******************************************************************************
** Function: tle_checksum
**
** validates tle checksum
**
*/
bool tle_checksum(const char* line1, const char* line2)
{
  char char_a, char_b;
  int i, check_a, check_b;

  if(strlen(line1) != 69)
  {
    return false;
  }

  if(strlen(line2) != 69)
  {
    return false;
  }

  check_a = 0;
  check_b = 0;

  for(i=0; i<68; i++)
  {
    char_a = line1[i];
    if(isdigit(char_a))
    {
      check_a += char_a - '0';
    }
    else if(char_a == '-')
    {
      check_a += 1;
    }
    
    char_b = line2[i];
    if(isdigit(char_b))
    {
      check_b += char_b - '0';
    }
    else if(char_b == '-')
    {
      check_b += 1;
    }
  }

  if((check_a % 10) != (line1[68] - '0'))
  {
    return false;
  }
  
  if((check_b % 10) != (line2[68] - '0'))
  {
    return false;
  }

  return true;
}

/******************************************************************************
** Function: calculateEpochAbsoluteTime
**
** Converts epoch time from TLE to seconds since J2000 Epoch
**
*/
void calculateEpochAbsoluteTime(const char* line1) {
   
   char YearString[3];
   char DOYstring[13];
   long year,DOY,Month,Day;
   double FloatDOY,FracDay,JDepoch;
   double Epoch;

   strncpy(YearString,&line1[18],2);
   year = (long) atoi(YearString);
   if (year < 57) year += 2000;
   else year += 1900;
   strncpy(DOYstring,&line1[20],12);
   FloatDOY = (double) atof(DOYstring);
   DOY = (long) FloatDOY;
   FracDay = FloatDOY - ((double) DOY);
   DOY2MD(year,DOY,&Month,&Day);
   JDepoch = YMDHMS2JD(year,Month,Day,0,0,0.0);
   JDepoch += FracDay;
   Epoch = JDToAbsTime(JDepoch);
   ExObj->AbsoluteTimeEpoch = Epoch;
   ExObj->AbsoluteTime = Epoch;
      
}

/******************************************************************************
** Function:  EXOBJ_ResetStatus
**
*/
void EXOBJ_ResetStatus(void)
{

   ExObj->ExecCnt = 0;
   
} /* End EXOBJ_ResetStatus() */

/******************************************************************************
** Function: EXOBJ_GetTblPtr
**
*/
const EXTBL_Struct* EXOBJ_GetTblPtr(void)
{

   return &(ExObj->Tbl);

} /* End EXOBJ_GetTblPtr() */


/******************************************************************************
** Function: EXOBJ_LoadTbl
**
*/
boolean EXOBJ_LoadTbl(EXTBL_Struct* NewTbl)
{

   boolean  RetStatus = TRUE;

   CFE_EVS_SendEvent (EXOBJ_DEMO_DEBUG_EID, CFE_EVS_DEBUG,"EXOBJ_LoadTbl() Entered");

   /*
   ** This is a simple table copy. More complex table loads may have pass/fail 
   ** criteria.
   */

   CFE_PSP_MemCpy(&(ExObj->Tbl), NewTbl, sizeof(EXTBL_Struct));

   return RetStatus;

} /* End EXOBJ_LoadTbl() */


/******************************************************************************
** Function: EXOBJ_LoadTblEntry
**
*/
boolean EXOBJ_LoadTblEntry(uint16 EntryId, EXTBL_Entry* NewEntry)
{

   boolean  RetStatus = TRUE;

   /* 
   ** This is a simple table entry copy. More complex table load may have 
   ** pass/fail criteria.
   */

   CFE_PSP_MemCpy(&(ExObj->Tbl.Entry[EntryId]),NewEntry,sizeof(EXTBL_Entry));

   return RetStatus;

} /* End EXOBJ_LoadTblEntry() */


/******************************************************************************
** Function: EXOBJ_DemoCmd
**
** Send an event message showing that the example object's command is executed.
**
*/
boolean EXOBJ_DemoCmd(void* DataObjPtr, const CFE_SB_MsgPtr_t MsgPtr)
{

   const EXOBJ_DemoCmdMsg *CmdMsg = (const EXOBJ_DemoCmdMsg *) MsgPtr;

   CFE_EVS_SendEvent (EXOBJ_DEMO_CMD_INFO_EID,
                      CFE_EVS_INFORMATION,
                      "Example demo command received with parameter %d",
                      CmdMsg->Parameter);

   return TRUE;

} /* End EXOBJ_EnableDataLoadCmd() */


/* end of file */
