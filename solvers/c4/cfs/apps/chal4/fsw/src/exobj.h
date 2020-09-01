/*
** Purpose: Define an example object.
**
** Notes:
**   1. This object is to show how an object is integrated into 
**      the opensat application framework.
**
** License:
**   Template written by David McComas and licensed under the GNU
**   Lesser General Public License (LGPL).
**
** References:
**   1. OpenSatKit Object-based Application Developer's Guide.
**   2. cFS Application Developer's Guide.
*/

#ifndef _exobj_
#define _exobj_

/*
** Includes
*/

#include "app_cfg.h"
#include "trans_rs422.h"
#include <rtems/termiostypes.h>

/*
** Event Message IDs
*/

#define EXOBJ_CMD_ENA_INFO_EID    (EXOBJ_BASE_EID + 0)
#define EXOBJ_SHELL_CMD_INFO_EID  (EXOBJ_BASE_EID + 1)
#define EXOBJ_GPIO_EID            (EXOBJ_BASE_EID + 2)

#define GPIO_PORT_NR (1)

/*
** Type Definitions
*/
#define CHAL4_CONFIG_SERIAL_PORT   "/dev/console_d"
#define CHAL4_CONFIG_BAUD_RATE     115200  //bps - gets converted to appropriate format in trans_rs422.c
#define CHAL4_CONFIG_TIMEOUT       0       //milliseconds.  Gets converted to 100ms chunks in trans_rs422.c
#define CHAL4_CONFIG_MINBYTES      0


/******************************************************************************
** ExObj_Class
*/

typedef struct {

   uint16       ExecCnt;
   void *gpio_port;
   
   bool connected;
   int fd;

   IO_TransRS422Config_t config;
   struct ttywakeup RxWake;

   uint8    RxBuff[128];
   uint8    RxPtr;

} EXOBJ_Class;


/******************************************************************************
** Command Functions
*/

typedef struct
{

   uint8  CmdHeader[CFE_SB_CMD_HDR_SIZE];
   uint8  Parameter[256];

} EXOBJ_ShellCmdMsg;
#define EXOBJ_SHELL_CMD_DATA_LEN  (sizeof(EXOBJ_ShellCmdMsg) - CFE_SB_CMD_HDR_SIZE)

typedef struct
{

   uint8  CmdHeader[CFE_SB_CMD_HDR_SIZE];
   uint8  Parameter;

} EXOBJ_EnableConsoleCmdMsg;
#define EXOBJ_ENA_CONSOLE_CMD_DATA_LEN  (sizeof(EXOBJ_EnableConsoleCmdMsg) - CFE_SB_CMD_HDR_SIZE)
/*
** Exported Functions
*/

/******************************************************************************
** Function: EXOBJ_Constructor
**
** Initialize the example object to a known state
**
** Notes:
**   1. This must be called prior to any other function.
**   2. The table values are not populated. This is done when the table is 
**      registered with the table manager.
**
*/
void EXOBJ_Constructor(EXOBJ_Class *ExObjPtr);


/******************************************************************************
** Function: EXOBJ_Execute
**
** Execute main object function.
**
*/
void EXOBJ_Execute(void);

/******************************************************************************
** Function: Rx_Wake
**
*/
void Rx_Wake(struct termios *tty,void*arg);

/******************************************************************************
** Function: UpdateRxBuffer
**
*/
void UpdateRxBuffer(void);

/******************************************************************************
** Function: EXOBJ_ConsoleDisconnect
**
**  Close the kubos payload terminal console
**
*/
void EXOBJ_ConsoleDisconnect(void);

/******************************************************************************
** Function: EX_OBJ_ConsoleConnect
**
**  Connect to the kubos payload terminal console
**
*/
void EXOBJ_ConsoleConnect(void);

/******************************************************************************
** Function: EXOBJ_ResetStatus
**
** Reset counters and status flags to a known reset state.
**
** Notes:
**   1. Any counter or variable that is reported in HK telemetry that doesn't
**      change the functional behavior should be reset.
**
*/
void EXOBJ_ResetStatus(void);

/******************************************************************************
** Function: EXOBJ_ShellCmd
**
** Send shell command to payload console.
**
** Note:
**  1. This function must comply with the CMDMGR_CmdFuncPtr definition
*/
boolean EXOBJ_ShellCmd(void* DataObjPtr, const CFE_SB_MsgPtr_t MsgPtr);

/******************************************************************************
** Function: EXOBJ_EnableConsole
**
** Enable/Disable the payload console port
**
** Note:
**  1. This function must comply with the CMDMGR_CmdFuncPtr definition
*/
boolean EXOBJ_EnableConsole(void* DataObjPtr, const CFE_SB_MsgPtr_t MsgPtr);

#endif /* _exobj_ */
