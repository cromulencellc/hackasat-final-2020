/*
** Purpose: Define platform configurations for the Chal4 application
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

#ifndef _chal4_platform_cfg_
#define _chal4_platform_cfg_

/*
** Includes
*/

#include "chal4_mission_cfg.h"
#include "chal4_msgids.h"
#include "chal4_perfids.h"

/******************************************************************************
** Application Macros
*/

#define  CHAL4_RUNLOOP_DELAY    500  /* Delay in milliseconds */

#define  CHAL4_CMD_PIPE_DEPTH    10
#define  CHAL4_CMD_PIPE_NAME     "CHAL4_CMD_PIPE"

/******************************************************************************
** Example Object Macros
*/

#define  CHAL4_EXTBL_DEF_LOAD_FILE  "/cf/chal4_extbl.json"
#define  CHAL4_EXTBL_DEF_DUMP_FILE  "/cf/chal4_extbl_d.json"

#endif /* _chal4_platform_cfg_ */
