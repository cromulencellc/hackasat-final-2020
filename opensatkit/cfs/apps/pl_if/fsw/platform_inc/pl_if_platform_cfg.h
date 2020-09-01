/*
** Purpose: Define platform configurations for the Pl_if application
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

#ifndef _pl_if_platform_cfg_
#define _pl_if_platform_cfg_

/*
** Includes
*/

#include "pl_if_mission_cfg.h"
#include "pl_if_msgids.h"
#include "pl_if_perfids.h"

/******************************************************************************
** Application Macros
*/

#define  PL_IF_RUNLOOP_DELAY    500  /* Delay in milliseconds */

#define  PL_IF_CMD_PIPE_DEPTH    10
#define  PL_IF_CMD_PIPE_NAME     "PL_IF_CMD_PIPE"

/******************************************************************************
** Example Object Macros
*/

#define  PL_IF_EXTBL_DEF_LOAD_FILE  "/cf/pl_if_extbl.json"
#define  PL_IF_EXTBL_DEF_DUMP_FILE  "/cf/pl_if_extbl_d.json"

#endif /* _pl_if_platform_cfg_ */
