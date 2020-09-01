/*
** Purpose: Implement an example object.
**
** Notes:
**   1. This serves as an example object that uses a table. It does not perform
**      any realistic funcions.
**
** License:
**   Template written by David McComas and licensed under the GNU
**   Lesser General Public License (LGPL).
**
** References:
**   1. OpenSatKit Object-based Application Developers Guide.
**   2. cFS Application Developer's Guide.
*/

/*
** Include Files:
*/

#include <string.h>

#include "app_cfg.h"
#include "exobj.h"
#include <bsp.h>
#include <grlib/gpiolib.h>

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>


#include <rtems.h>
#include <rtems/termiostypes.h>


/*
** Global File Data
*/

static EXOBJ_Class*  ExObj = NULL;

/*
** Local Function Prototypes
*/


/******************************************************************************
** Function: EXOBJ_Constructor
**
*/
void EXOBJ_Constructor(EXOBJ_Class*  ExObjPtr)
{
 
   ExObj = ExObjPtr;

   CFE_PSP_MemSet((void*)ExObj, 0, sizeof(EXOBJ_Class));

   /*Initialize a UART Port */
   strncpy((char *) &ExObj->config.device, CHAL4_CONFIG_SERIAL_PORT, PORT_NAME_SIZE);
   ExObj->config.baudRate = CHAL4_CONFIG_BAUD_RATE;
   ExObj->config.timeout  = CHAL4_CONFIG_TIMEOUT;
   ExObj->config.minBytes = CHAL4_CONFIG_MINBYTES;
   
   ExObj->RxWake.sw_pfn = Rx_Wake;
   ExObj->RxWake.sw_arg = NULL;

   ExObj->RxPtr = 0;

   EXOBJ_ConsoleConnect();

   /* Show all GPIO Ports available */
	gpiolib_show(-1, NULL);
   ExObj->gpio_port = gpiolib_open(GPIO_PORT_NR);
	if ( ExObj->gpio_port == NULL ){
      CFE_EVS_SendEvent (EXOBJ_GPIO_EID, CFE_EVS_ERROR,
            "Attempted to disconnect from disconnected payload console");
	}

    
} /* End EXOBJ_Constructor() */

/******************************************************************************
** Function: Rx_Wake
**
*/
void Rx_Wake(struct termios *tty,void*arg)
{
   UpdateRxBuffer();
}

/******************************************************************************
** Function: UpdateRxBuffer
**
*/
void UpdateRxBuffer() {
   IO_TransRS422Read(ExObj->fd,(uint8 *)&ExObj->RxBuff[ExObj->RxPtr], 1);
   OS_printf("%c",ExObj->RxBuff[ExObj->RxPtr]);
   ExObj->RxPtr++;
   if (ExObj->RxPtr == sizeof(ExObj->RxBuff)){
      ExObj->RxPtr = 0;
      memset(ExObj->RxBuff,0,sizeof(ExObj->RxBuff));
   }

}

/******************************************************************************
** Function: EX_OBJ_ConsoleDisconnect
**
** Closes the payload system console
**
*/
void EXOBJ_ConsoleDisconnect(void)
{
   if (ExObj->connected) {
      IO_TransRS422Close(ExObj->fd);
      CFE_EVS_SendEvent (EXOBJ_CMD_ENA_INFO_EID, CFE_EVS_INFORMATION,
            "Disconnected from payload console on %s",
            ExObj->config.device);
      ExObj->fd = 0;
      ExObj->connected = FALSE;

   } else {
      CFE_EVS_SendEvent (EXOBJ_CMD_ENA_INFO_EID, CFE_EVS_INFORMATION,
            "Attempted to disconnect from disconnected payload console");
   }
} /* EX_OBJ_ConsoleDisconnect() */

/******************************************************************************
** Function: EX_OBJ_ConsoleConnect
**
**  Open the payload system console
**
*/
void EXOBJ_ConsoleConnect(void) 
{
   ExObj->fd = IO_TransRS422Init(&ExObj->config);

   int sc = ioctl( ExObj->fd, RTEMS_IO_RCVWAKEUP, &ExObj->RxWake );
   if (sc < 0) {
      CFE_EVS_SendEvent (EXOBJ_CMD_ENA_INFO_EID,
               CFE_EVS_ERROR,
               "Failed to register for wakeup callback on %s",
               ExObj->config.device);
   }

   if (ExObj->fd < 0)
   {
      CFE_EVS_SendEvent(EXOBJ_CMD_ENA_INFO_EID,
               CFE_EVS_ERROR,
               "Failed to connect to kubos console on %s",
               ExObj->config.device);
   } else { // Connected succesfully
      CFE_EVS_SendEvent(EXOBJ_CMD_ENA_INFO_EID,
               CFE_EVS_INFORMATION,
               "Connected succesfully to kubos console on %s",
               ExObj->config.device);
      ExObj->connected = TRUE;
   }
}

/******************************************************************************
** Function: EXOBJ_Execute
**
** Execute main object function.
**
*/
void EXOBJ_Execute(void)
{

   ExObj->ExecCnt++;

} /* EXOBJ_Execute() */

/******************************************************************************
** Function: EXOBJ_ShellCmd
**
** Send shell command to payload console.
**
** Note:
**  1. This function must comply with the CMDMGR_CmdFuncPtr definition
*/
boolean EXOBJ_ShellCmd(void* DataObjPtr, const CFE_SB_MsgPtr_t MsgPtr) {

   const EXOBJ_ShellCmdMsg *CmdMsg = (const EXOBJ_ShellCmdMsg *) MsgPtr;

   CFE_EVS_SendEvent (EXOBJ_SHELL_CMD_INFO_EID,
                      CFE_EVS_INFORMATION,
                      "Shell command received with parameter %s",
                      CmdMsg->Parameter);

   int writePtr = 0;
   int status = 0;
   while (CmdMsg->Parameter[writePtr] != 0) {
      status = IO_TransRS422Write(ExObj->fd,(uint8 *)&CmdMsg->Parameter[writePtr],1);
      // rtems_task_wake_after(1);
      writePtr += status;
      if (status < 0) {
         CFE_EVS_SendEvent (EXOBJ_SHELL_CMD_INFO_EID, CFE_EVS_ERROR, "Error payload console command on %s. Status=%d\n", ExObj->config.device, status);
      }
   }
   if (writePtr > 0) {
      status = IO_TransRS422Write(ExObj->fd,(uint8 *)"\n",1);
   }

   return TRUE;
}

/******************************************************************************
** Function: EXOBJ_EnableConsole
**
** Enable/Disable payload pwoer.
**
*/
boolean EXOBJ_EnableConsole(void* DataObjPtr, const CFE_SB_MsgPtr_t MsgPtr)
{

   const EXOBJ_EnableConsoleCmdMsg *CmdMsg = (const EXOBJ_EnableConsoleCmdMsg *) MsgPtr;

   CFE_EVS_SendEvent (EXOBJ_GPIO_EID,
                      CFE_EVS_INFORMATION,
                      "GPIO console enable command received with parameter %d",
                      CmdMsg->Parameter);

   if(CmdMsg->Parameter == 1) {
      if(ExObj->gpio_port) {
         gpiolib_set(ExObj->gpio_port,1,1);
      }
   } else if (CmdMsg->Parameter == 0) {
      if(ExObj->gpio_port) {
	      gpiolib_set(ExObj->gpio_port,1,0);
      }
   } else {
      CFE_EVS_SendEvent (EXOBJ_GPIO_EID, CFE_EVS_INFORMATION,
                        "Unsupported GPIO setting.  Send 1(On) or 0(Off)");
   }

   return TRUE;

} /* End PLIF_OBJ_PowerCmd() */

/******************************************************************************
** Function:  EXOBJ_ResetStatus
**
*/
void EXOBJ_ResetStatus(void)
{

   ExObj->ExecCnt = 0;
   
} /* End EXOBJ_ResetStatus() */


/* end of file */
