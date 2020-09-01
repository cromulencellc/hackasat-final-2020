/*
** Purpose: Define platform configurations for the Ephem application
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

#ifndef _ephem_platform_cfg_
#define _ephem_platform_cfg_

/*
** Includes
*/

#include "ephem_mission_cfg.h"
#include "ephem_msgids.h"
#include "ephem_perfids.h"

/******************************************************************************
** Application Macros
*/

#define  EPHEM_RUNLOOP_DELAY    1000  /* Delay in milliseconds */

#define  EPHEM_CMD_PIPE_DEPTH    10
#define  EPHEM_CMD_PIPE_NAME     "EPHEM_CMD_PIPE"

/******************************************************************************
** Example Object Macros
*/

#define  EPHEM_EXTBL_DEF_LOAD_FILE  "/cf/ephem_extbl.json"
#define  EPHEM_EXTBL_DEF_DUMP_FILE  "/cf/ephem_extbl_d.json"

#endif /* _ephem_platform_cfg_ */
