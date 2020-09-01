/*
** Purpose: Define platform configurations for the Eyassat_if application
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

#ifndef _eyassat_if_platform_cfg_
#define _eyassat_if_platform_cfg_

/*
** Includes
*/

#include "eyassat_if_mission_cfg.h"
#include "eyassat_if_msgids.h"
#include "eyassat_if_perfids.h"

/******************************************************************************
** Application Macros
*/

#define  EYASSAT_IF_RUNLOOP_DELAY     500 /* Delay in milliseconds */

#define  EYASSAT_IF_CMD_PIPE_DEPTH    20
#define  EYASSAT_IF_CMD_PIPE_NAME     "EYASSAT_IF_CMD_PIPE"

/******************************************************************************
** Example Object Macros
*/

#define  EYASSAT_IF_ADCS_LOAD_FILE  "/cf/es_adcs_tbl.json"
#define  EYASSAT_IF_ADCS_DUMP_FILE  "/cf/es_adcs_tbl_d.json"

#endif /* _eyassat_if_platform_cfg_ */
