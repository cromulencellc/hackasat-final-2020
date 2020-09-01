/* 
** Purpose: Define a Eyassat_if application.
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
#ifndef _eyassat_if_app_
#define _eyassat_if_app_

/*
** Includes
*/

#include "app_cfg.h"
#include "eyassat_obj.h"
#include "eyassat_tbl.h"

/*
** Macro Definitions
*/

#define EYASSAT_IF_INIT_INFO_EID            (EYASSAT_IF_BASE_EID + 0)
#define EYASSAT_IF_EXIT_ERR_EID             (EYASSAT_IF_BASE_EID + 1)
#define EYASSAT_IF_CMD_NOOP_INFO_EID        (EYASSAT_IF_BASE_EID + 2)
#define EYASSAT_IF_CMD_INVALID_MID_ERR_EID  (EYASSAT_IF_BASE_EID + 3)

/*
** Type Definitions
*/

typedef struct
{

   CMDMGR_Class       CmdMgr;
   TBLMGR_Class       TblMgr;
   EYASSATOBJ_Class   EyasSatObj;
   EYASSATTBL_Class   EyasSatTbl;
   
   CFE_SB_PipeId_t    CmdPipe;
   uint16             ExecCnt;



} EYASSAT_IF_Class;

typedef struct
{

   uint8    Header[CFE_SB_TLM_HDR_SIZE];

   /*
   ** CMDMGR Data
   */
   uint16   ValidCmdCnt;
   uint16   InvalidCmdCnt;

   /*
   ** Example Table Data 
   ** - Loaded with status from the last table action 
   */

   uint8    LastAction;
   uint8    LastActionStatus;

   
   /*
   ** EXOBJ Data
   */

   uint16   EyasSatObjExecCnt;

   /*
   ** Command Buffer Counters
   */
   uint8     CmdBufferHead;
   uint8     CmdBufferTail;

   uint16    Pad;


} OS_ALIGN(4) EYASSAT_IF_HkPkt;
#define EYASSAT_IF_TLM_HK_LEN sizeof (EYASSAT_IF_HkPkt)

typedef struct
{

   uint8    Header[CFE_SB_TLM_HDR_SIZE];
   uint32   Pad;
   double   MagX;
   double   MagY;
   double   MagZ;

   double   GyroX;
   double   GyroY;
   double   GyroZ;

} OS_ALIGN(8) EYASSAT_IF_CalTblPkt;
#define EYASSAT_IF_TLM_CAL_TBL_LEN sizeof (EYASSAT_IF_CalTblPkt)

/*
** Exported Data
*/

extern EYASSAT_IF_Class  Eyassat_if;

/*
** Exported Functions
*/

/******************************************************************************
** Function: EYASSAT_IF_AppMain
**
*/
void EYASSAT_IF_AppMain(void);

#endif /* _eyassat_if_app_ */
