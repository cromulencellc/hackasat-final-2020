/*
** Purpose: Define Eyassat ADCS Configuration Table
**
** Notes:
**   1. Use the Singleton design pattern. A pointer to the table object
**      is passed to the constructor and saved for all other operations.
**      This is a table-specific file so it doesn't need to be re-entrant.
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
#ifndef _eyassattbl_
#define _eyassattbl_

/*
** Includes
*/

#include "app_cfg.h"


/*
** Macro Definitions
*/

// #define EYASSATTBL_MAX_ENTRY_ID 32

/*
** Event Message IDs
*/

#define EYASSATTBL_CREATE_FILE_ERR_EID          (EYASSATTBL_BASE_EID + 0)
#define EYASSATTBL_LOAD_INDEX_ERR_EID           (EYASSATTBL_BASE_EID + 1)
#define EYASSATTBL_LOAD_INIT_EID                (EYASSATTBL_BASE_EID + 2)
#define EYASSATTBL_CMD_LOAD_TYPE_ERR_EID        (EYASSATTBL_BASE_EID + 3)
#define EYASSATTBL_CMD_LOAD_PARSE_ERR_EID       (EYASSATTBL_BASE_EID + 4)
#define EYASSATTBL_CMD_LOAD_OPEN_ERR_EID        (EYASSATTBL_BASE_EID + 5)
#define EYASSATTBL_CMD_LOAD_REPLACE_ERR_EID     (EYASSATTBL_BASE_EID + 6)
#define EYASSATTBL_CMD_LOAD_UPDATE_ERR_EID      (EYASSATTBL_BASE_EID + 7)
#define EYASSATTBL_LOAD_ERR_EID                 (EYASSATTBL_BASE_EID + 8)

/*
** Table Structure Objects 
*/

#define  EYASSATTBL_OBJ_CONFIG       0
#define  EYASSATTBL_OBJ_DB           1
#define  EYASSATTBL_OBJ_PID          2
#define  EYASSATTBL_OBJ_MAGCAL       3
#define  EYASSATTBL_OBJ_GYROCAL      4
#define  EYASSATTBL_OBJ_CNT          5

/*
** Type Definitions
*/

/******************************************************************************
** Table -  Local table copy used for table loads
** 
*/

typedef struct {

   int      CtrlMode;
   double   YawCmd;
   int      PWM_Baseline;

} Config_Struct;

typedef struct {

   double   Deadband;
   double   DeadBandScaleFactor;
   double   Extra;

} DB_Struct;

typedef struct {

   double   Kp;
   double   Ki;
   double   Kd;

} PID_Struct;

typedef struct {

   double   MagX;
   double   MagY;
   double   MagZ;

} MagCal_Struct;

typedef struct {

   double   GyroX;
   double   GyroY;
   double   GyroZ;

} GyroCal_Struct;

typedef struct
{

   Config_Struct Config;
   DB_Struct DB;
   PID_Struct PID;
   MagCal_Struct MagCal;
   GyroCal_Struct GyroCal;

} EYASSATTBL_Struct;

/*
** Table Owner Callback Functions
*/

/* Return pointer to owner's table data */
typedef const EYASSATTBL_Struct* (*EYASSATTBL_GetTblPtr)(void);
            
/* Table Owner's function to load all table data */
typedef boolean (*EYASSATTBL_LoadTbl)(EYASSATTBL_Struct* NewTable); 

/* Table Owner's function to load a single table entry */
typedef boolean (*EYASSATTBL_LoadTblEntry)(uint16 ObjId, void* ObjData);   


typedef struct {

   uint8    LastLoadStatus;
   uint16   AttrErrCnt;
   uint16   ObjLoadCnt;

   EYASSATTBL_Struct Data;

   EYASSATTBL_GetTblPtr    GetTblPtrFunc;
   EYASSATTBL_LoadTbl      LoadTblFunc;
   EYASSATTBL_LoadTblEntry LoadTblEntryFunc; 

   JSON_Class Json;
   JSON_Obj   JsonObj[EYASSATTBL_OBJ_CNT];
   char       JsonFileBuf[JSON_MAX_FILE_CHAR];   
   jsmntok_t  JsonFileTokens[JSON_MAX_FILE_TOKENS];

} EYASSATTBL_Class;


/*
** Exported Functions
*/


/******************************************************************************
** Function: EYASSATTBL_Constructor
**
** Initialize the example table object.
**
** Notes:
**   1. The table values are not populated. This is done when the table is 
**      registered with the table manager.
**
*/
void EYASSATTBL_Constructor(EYASSATTBL_Class* TblObj, 
                       EYASSATTBL_GetTblPtr    GetTblPtrFunc,
                       EYASSATTBL_LoadTbl      LoadTblFunc, 
                       EYASSATTBL_LoadTblEntry LoadTblEntryFunc);


/******************************************************************************
** Function: EYASSATTBL_ResetStatus
**
** Reset counters and status flags to a known reset state.  The behavour of
** the table manager should not be impacted. The intent is to clear counters
** and flags to a known default state for telemetry.
**
*/
void EYASSATTBL_ResetStatus(void);


/******************************************************************************
** Function: EYASSATTBL_LoadCmd
**
** Command to load the table.
**
** Notes:
**  1. Function signature must match TBLMGR_LoadTblFuncPtr.
**  2. Can assume valid table file name because this is a callback from 
**     the app framework table manager.
**
*/
boolean EYASSATTBL_LoadCmd(TBLMGR_Tbl *Tbl, uint8 LoadType, const char* Filename);


/******************************************************************************
** Function: EYASSATTBL_DumpCmd
**
** Command to dump the table.
**
** Notes:
**  1. Function signature must match TBLMGR_DumpTblFuncPtr.
**  2. Can assume valid table file name because this is a callback from 
**     the app framework table manager.
**
*/
boolean EYASSATTBL_DumpCmd(TBLMGR_Tbl *Tbl, uint8 DumpType, const char* Filename);

#endif /* _eyassattbl_ */

