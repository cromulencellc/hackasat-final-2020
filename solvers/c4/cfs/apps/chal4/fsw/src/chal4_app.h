/* 
** Purpose: Define a Chal4 application.
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
#ifndef _chal4_app_
#define _chal4_app_

/*
** Includes
*/

#include "app_cfg.h"
#include "exobj.h"

/*
** Macro Definitions
*/

#define CHAL4_INIT_INFO_EID            (CHAL4_BASE_EID + 0)
#define CHAL4_EXIT_ERR_EID             (CHAL4_BASE_EID + 1)
#define CHAL4_CMD_NOOP_INFO_EID        (CHAL4_BASE_EID + 2)
#define CHAL4_CMD_INVALID_MID_ERR_EID  (CHAL4_BASE_EID + 3)

/*
** Type Definitions
*/

typedef struct
{

   CMDMGR_Class  CmdMgr;
   EXOBJ_Class   ExObj;
   CFE_SB_PipeId_t CmdPipe;

} CHAL4_Class;

typedef struct
{

   uint8    Header[CFE_SB_TLM_HDR_SIZE];

   /*
   ** CMDMGR Data
   */
   uint16   ValidCmdCnt;
   uint16   InvalidCmdCnt;

   
   /*
   ** EXOBJ Data
   */

   uint16   ExObjExecCnt;
   uint16   Pad;

} OS_ALIGN(4) CHAL4_HkPkt;

#define CHAL4_TLM_HK_LEN sizeof (CHAL4_HkPkt)

typedef struct
{

   uint8    Header[CFE_SB_TLM_HDR_SIZE];
   char     ShellOutput[128];


} OS_ALIGN(4) CHAL4_ShellOutputPkt;

#define CHAL4_TLM_SHELL_OUTPUT_LEN sizeof (CHAL4_ShellOutputPkt)


/*
** Exported Data
*/

extern CHAL4_Class  Chal4;

/*
** Exported Functions
*/

/******************************************************************************
** Function: CHAL4_AppMain
**
*/
void CHAL4_AppMain(void);

#endif /* _chal4_app_ */
