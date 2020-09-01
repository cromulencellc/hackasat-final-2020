/* 
** Purpose: Define a Pl_if application.
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
#ifndef _pl_if_app_
#define _pl_if_app_

/*
** Includes
*/

#include "app_cfg.h"
#include "pl_if_obj.h"
#include "extbl.h"

/*
** Macro Definitions
*/

#define PL_IF_INIT_INFO_EID            (PL_IF_BASE_EID + 0)
#define PL_IF_EXIT_ERR_EID             (PL_IF_BASE_EID + 1)
#define PL_IF_CMD_NOOP_INFO_EID        (PL_IF_BASE_EID + 2)
#define PL_IF_CMD_INVALID_MID_ERR_EID  (PL_IF_BASE_EID + 3)

/*
** Type Definitions
*/

typedef struct
{

   CMDMGR_Class   CmdMgr;
   TBLMGR_Class   TblMgr;
   PLIF_OBJ_Class PlIfObj;
   EXTBL_Class    ExTbl;
   
   CFE_SB_PipeId_t CmdPipe;

} PL_IF_Class;

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
   ** PLIF_OBJ Data
   */

   uint16   PlIfObjExecCnt;

} OS_ALIGN(4) PL_IF_HkPkt;

#define PL_IF_TLM_HK_LEN sizeof (PL_IF_HkPkt)

/*
** Exported Data
*/

extern PL_IF_Class  Pl_if;

/*
** Exported Functions
*/

/******************************************************************************
** Function: PL_IF_AppMain
**
*/
void PL_IF_AppMain(void);

#endif /* _pl_if_app_ */
