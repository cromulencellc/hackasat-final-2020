/* 
** Purpose: Define a Ephem application.
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
#ifndef _ephem_app_
#define _ephem_app_

/*
** Includes
*/

#include "app_cfg.h"
#include "exobj.h"
#include "extbl.h"

/*
** Macro Definitions
*/

#define EPHEM_INIT_INFO_EID            (EPHEM_BASE_EID + 0)
#define EPHEM_EXIT_ERR_EID             (EPHEM_BASE_EID + 1)
#define EPHEM_CMD_NOOP_INFO_EID        (EPHEM_BASE_EID + 2)
#define EPHEM_CMD_INVALID_MID_ERR_EID  (EPHEM_BASE_EID + 3)

/*
** Type Definitions
*/

typedef struct
{

   CMDMGR_Class  CmdMgr;
   TBLMGR_Class  TblMgr;
   EXOBJ_Class   ExObj;
   EXTBL_Class   ExTbl;
   
   CFE_SB_PipeId_t CmdPipe;

} EPHEM_Class;

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

   uint16   ExObjExecCnt;

} OS_PACK EPHEM_HkPkt;

#define EPHEM_TLM_HK_LEN sizeof (EPHEM_HkPkt)

typedef struct
{

   uint8    Header[CFE_SB_TLM_HDR_SIZE];
   uint8    Pad[4];
   char     TimeString[64];
   double   AbsoluteTimeOffset;
   double   AboluteTimeEpoch;
   double   AbsoluteTime;
   double   PosN_X;
   double   PosN_Y;
   double   PosN_Z;
   double   VelN_X;
   double   VelN_Y;
   double   VelN_Z;

} OS_ALIGN(8) EPHEM_EphemPkt;

#define EPHEM_TLM_EPHEM_LEN sizeof (EPHEM_EphemPkt)

/*
** Exported Data
*/

extern EPHEM_Class  Ephem;

/*
** Exported Functions
*/

/******************************************************************************
** Function: EPHEM_AppMain
**
*/
void EPHEM_AppMain(void);

#endif /* _ephem_app_ */
