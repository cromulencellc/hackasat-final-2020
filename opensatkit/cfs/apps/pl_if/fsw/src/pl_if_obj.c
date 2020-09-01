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
#include "pl_if_obj.h"
#include <bsp.h>
#include <grlib/gpiolib.h>

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>


#include <rtems.h>
#include <rtems/termiostypes.h>

#include <rtems/libi2c.h>
#include <grlib/i2cmst.h>

/*
** Global File Data
*/

static PLIF_OBJ_Class*  PlIfObj = NULL;

/*
** Local Function Prototypes
*/


/******************************************************************************
** Function: PLIF_OBJ_Constructor
**
*/
void PLIF_OBJ_Constructor(PLIF_OBJ_Class*  PlIfObjPtr)
{
 
   PlIfObj = PlIfObjPtr;

   CFE_PSP_MemSet((void*)PlIfObj, 0, sizeof(PLIF_OBJ_Class));
   
   if ( gpiolib_initialize() ) {
      CFE_EVS_SendEvent (PLIF_OBJ_GPIO_EID, CFE_EVS_ERROR,
            "Attempted to disconnect from disconnected payload console");
   }

	/* Show all GPIO Ports available */
	// gpiolib_show(-1, NULL);

	PlIfObj->gpio_port = gpiolib_open(GPIO_PORT_NR);
	if ( PlIfObj->gpio_port == NULL ){
      CFE_EVS_SendEvent (PLIF_OBJ_GPIO_EID, CFE_EVS_ERROR,
            "Attempted to disconnect from disconnected payload console");
	}

   // Open a fd to the i2c device
   PLIF_OBJ_i2cOpen();

   CFE_SB_InitMsg(&(PlIfObj->PlStatusPkt), PL_IF_PL_STATUS_MID, PLIF_OBJ_PL_STATUS_DATA_LEN, TRUE);

} /* End PLIF_OBJ_Constructor() */

/******************************************************************************
** Function: PLIF_OBJ_i2cOpen
**
**  Opens i2c device
**
*/
void PLIF_OBJ_i2cOpen(void) {

   int status;
   dev_t raw_dev_t;
   char *raw_dev = "/dev/i2c-09";

   /* Create node for 'raw' access to EEPROM device */
   raw_dev_t = rtems_filesystem_make_dev_t(rtems_libi2c_major, 
				      RTEMS_LIBI2C_MAKE_MINOR(I2CMST_MINOR-1,PI_ADDR));
   status = mknod(raw_dev, S_IFCHR | 0666, raw_dev_t);
   if (status < 0) {

      CFE_EVS_SendEvent (PLIF_OBJ_I2C_EID, CFE_EVS_ERROR,
                        "mknod of %s failed: %s", raw_dev, strerror(errno));
      
   }

   PlIfObj->i2cfd = open(raw_dev, O_RDWR);
   if (PlIfObj->i2cfd < 0) {

      CFE_EVS_SendEvent (PLIF_OBJ_I2C_EID, CFE_EVS_ERROR,
                        "Could not open %s: %s", raw_dev, strerror(errno));
   } else {

      CFE_EVS_SendEvent (PLIF_OBJ_I2C_EID, CFE_EVS_INFORMATION,
                        "i2c open success fd: %d",PlIfObj->i2cfd);

   }

} /* End PLIF_OBJ_i2cOpen() */

/******************************************************************************
** Function: PLIF_OBJ_i2cWrite
**
**  Write to i2c device
**
*/
int PLIF_OBJ_i2cWrite(char *buf, int count) {

   int status;
  
   status = write(PlIfObj->i2cfd, buf, count);
   if (status < 0) {

      CFE_EVS_SendEvent (PLIF_OBJ_I2C_EID, CFE_EVS_ERROR,
                        "write() call returned with error: %s", strerror(errno));
   } else if (status < count) {

      CFE_EVS_SendEvent (PLIF_OBJ_I2C_EID, CFE_EVS_ERROR,
                        "write() did not output all bytes\n""requested bytes: %d, returned bytes: %d", count, status);
   } else if (status > count) {

      // count greater than status is returned when their is a valid i2c fd
      // but the payload has not yet initialized
      PlIfObj->AliveStatus = false;

   } else {

      PlIfObj->AliveStatus = true;
   }

  return status;

} /* End PLIF_OBJ_i2cWrite() */

/******************************************************************************
** Function: PLIF_OBJ_i2cRead
**
**  Read from i2c device
**
*/
int PLIF_OBJ_i2cRead(uint8 *buff, int count) {

   int status;

   status = read(PlIfObj->i2cfd, buff, count);
   if (status < 0) {

      CFE_EVS_SendEvent (PLIF_OBJ_I2C_EID, CFE_EVS_ERROR,
                        "read() call returned with error: %s", strerror(errno));

   } else if (status < count) {

      CFE_EVS_SendEvent (PLIF_OBJ_I2C_EID, CFE_EVS_ERROR,
                        "read() did not reaturn with all bytes\n""requested bytes: %d, returned bytes: %d", count, status);
   } else if (status > count) {

      // count greater than status is returned when their is a valid i2c fd
      // but the payload has not yet initialized
      PlIfObj->AliveStatus = false;

   } else {

      PlIfObj->AliveStatus = true;
   }

   return status;

} /* End PLIF_OBJ_i2cRead() */

/******************************************************************************
** Function: PLIF_OBJ_i2cClose
**
**  Close i2c device
**
*/
void PLIF_OBJ_i2cClose(void) {

   close(PlIfObj->i2cfd);

} /* End PLIF_OBJ_i2cClose() */


/******************************************************************************
** Function: PLIF_OBJ_SendPlStatusTlm
**
**  Send the PL status telemetry packet
**
*/
void PLIF_OBJ_SendPlStatusTlm(void) {

   PlIfObj->PlStatusPkt.BusyStatus = PlIfObj->BusyStatus;
   PlIfObj->PlStatusPkt.ImgReadyStatus = PlIfObj->ImgReadyStatus;
   PlIfObj->PlStatusPkt.BadAddrStatus = PlIfObj->BadAddrStatus;
   PlIfObj->PlStatusPkt.AliveStatus = PlIfObj->AliveStatus;
   PlIfObj->PlStatusPkt.DownlinkStatus = PlIfObj->DownlinkStatus;
   PlIfObj->PlStatusPkt.ImgSize = (uint32)PlIfObj->ImgSize;
   PlIfObj->PlStatusPkt.CurrentDownlinkAddr = PlIfObj->CurrentDownlinkAddr;


   CFE_SB_TimeStampMsg((CFE_SB_Msg_t *) &(PlIfObj->PlStatusPkt));
   CFE_SB_SendMsg((CFE_SB_Msg_t *) &(PlIfObj->PlStatusPkt));

}

/******************************************************************************
** Function: PLIF_OBJ_CheckStatus
**
**  Check the current status of the payload
**
*/
void PLIF_OBJ_CheckStatus(void) {

   // We check status bytes in a round robin fashion
   switch (PlIfObj->CurrentStatusRqst)
   {
   case 0:
      // Read busy status
      memset(PlIfObj->CmdBuff,'\0',sizeof(PlIfObj->CmdBuff));
      snprintf((char *)&(PlIfObj->CmdBuff),sizeof(PlIfObj->CmdBuff),"%d",BUSY);
      PLIF_OBJ_i2cWrite((char *)&(PlIfObj->CmdBuff),sizeof(PlIfObj->CmdBuff));
      OS_TaskDelay(I2C_WRITE_READ_DELAY);
      PLIF_OBJ_i2cRead((uint8 *)&(PlIfObj->StatusBuff),1);
      PlIfObj->BusyStatus = PlIfObj->StatusBuff;
      break;
   
   case 1:
      // Read image ready status
      memset(PlIfObj->CmdBuff,'\0',sizeof(PlIfObj->CmdBuff));
      snprintf((char *)&(PlIfObj->CmdBuff),sizeof(PlIfObj->CmdBuff),"%d",IMG_READY);
      PLIF_OBJ_i2cWrite((char *)&(PlIfObj->CmdBuff),sizeof(PlIfObj->CmdBuff));
      OS_TaskDelay(I2C_WRITE_READ_DELAY);
      PLIF_OBJ_i2cRead((uint8 *)&(PlIfObj->StatusBuff),1);
      PlIfObj->ImgReadyStatus = PlIfObj->StatusBuff;
      break;

   case 2:
      // Read bad address status
      memset(PlIfObj->CmdBuff,'\0',sizeof(PlIfObj->CmdBuff));
      snprintf((char *)&(PlIfObj->CmdBuff),sizeof(PlIfObj->CmdBuff),"%d",BAD_ADDR);
      PLIF_OBJ_i2cWrite((char *)&(PlIfObj->CmdBuff),sizeof(PlIfObj->CmdBuff));
      OS_TaskDelay(I2C_WRITE_READ_DELAY);
      PLIF_OBJ_i2cRead((uint8 *)&(PlIfObj->StatusBuff),1);
      PlIfObj->BadAddrStatus = PlIfObj->StatusBuff;
      break;
   
   default:
      break;
   }
   PlIfObj->CurrentStatusRqst++;
   // Roll the counter back over
   if (PlIfObj->CurrentStatusRqst == 3) {
      PlIfObj->CurrentStatusRqst = 0;
   }

} /* End PLIF_OBJ_CheckStatus() */

/******************************************************************************
** Function: PLIF_OBJ_GetImageSize
**
**  Get the image size from the payload prior to downlinking
**
*/
void PLIF_OBJ_GetImageSize(void) {
   
   memset(PlIfObj->CmdBuff,'\0',sizeof(PlIfObj->CmdBuff));
   snprintf((char *)&(PlIfObj->CmdBuff),sizeof(PlIfObj->CmdBuff),"%d",IMG_SIZE);
   PLIF_OBJ_i2cWrite((char *)&(PlIfObj->CmdBuff),sizeof(PlIfObj->CmdBuff));
   OS_TaskDelay(I2C_IMG_SIZE_DELAY);
   PLIF_OBJ_i2cRead((uint8 *)&(PlIfObj->ImgSizeBuff),4);

   PlIfObj->ImgSize = (int)(PlIfObj->ImgSizeBuff[0] << 24 |
                      PlIfObj->ImgSizeBuff[1] << 16 |
                      PlIfObj->ImgSizeBuff[2] << 8 |
                      PlIfObj->ImgSizeBuff[3]);

   CFE_EVS_SendEvent (PLIF_OBJ_I2C_EID, CFE_EVS_INFORMATION,
                  "Payload Image size %d", PlIfObj->ImgSize);

}

/******************************************************************************
** Function: PLIF_OBJ_DownlinkImage
**
**  Downink image from payload to cfs filesystem
**
*/
void PLIF_OBJ_DownlinkImage(void) {

   uint32 imgAddr = 0;
   int readStatus;
   int writeStatus;
   int numSegments = 0;
   int lastWriteSize = 0;

   // Verify i2c fd is open and pl is alive
   if ((PlIfObj->i2cfd > 0) && PlIfObj->AliveStatus) {

      // Check to see if the image is ready and if not, don't downlink
      if (PlIfObj->ImgReadyStatus) {
         PLIF_OBJ_GetImageSize();
         if (PlIfObj->ImgSize > 200000) {
            CFE_EVS_SendEvent (PLIF_OBJ_I2C_EID, CFE_EVS_ERROR,
                              "Image size is invalid %d. Downlink Cancelled", PlIfObj->ImgSize);
            PlIfObj->DownlinkStatus = 2;
            PlIfObj->PerformDownlink = false;
            return;
         }
         OS_TaskDelay(I2C_WRITE_READ_DELAY);
      } else {

         CFE_EVS_SendEvent (PLIF_OBJ_I2C_EID, CFE_EVS_INFORMATION,
                           "Image not not ready. Wait for image ready prior to downlinking");
         PlIfObj->DownlinkStatus = 2;
         PlIfObj->PerformDownlink = false;
         return;
      }

      CFE_EVS_SendEvent (PLIF_OBJ_I2C_EID, CFE_EVS_INFORMATION,
                        "Image downlink starting");

      // Open the image file
      PlIfObj->ImgFd = OS_open(IMG_FILE_PATH, OS_READ_WRITE, O_APPEND);
      if (PlIfObj->ImgFd < 0) {
         CFE_EVS_SendEvent (PLIF_OBJ_I2C_EID, CFE_EVS_ERROR,
                           "Error opening image file. Downlink Cancelled");
         PlIfObj->DownlinkStatus = 2;
         PlIfObj->PerformDownlink = false;
         return;
      }

      // Calculate number of 16 byte segments
      // If there is a remainder add one
      if (PlIfObj->ImgSize % IMG_SEG_SIZE == 0) {
         numSegments = PlIfObj->ImgSize/IMG_SEG_SIZE;
      } else {
         numSegments = PlIfObj->ImgSize/IMG_SEG_SIZE + 1;
      }

      //Request the image
      for(int readPtr = 0; readPtr < numSegments; readPtr++) {
      
         // Send image image register address to pi
         imgAddr = IMG_REG + readPtr;
         memset(PlIfObj->AddrCmdBuf,'\0',sizeof(PlIfObj->AddrCmdBuf));
         int ret = snprintf((char *)&(PlIfObj->AddrCmdBuf),sizeof(PlIfObj->AddrCmdBuf),"%d",imgAddr);
         if (ret < 0) {
            CFE_EVS_SendEvent (PLIF_OBJ_I2C_EID, CFE_EVS_ERROR,
                              "image address command string conversion error %d", ret);
            PlIfObj->DownlinkStatus = 2;
            PlIfObj->PerformDownlink = false;
            OS_close(PlIfObj->ImgFd);
            return;
         }

         // check for write status less than addr cmd buf if so abort downlink
         if ((writeStatus = PLIF_OBJ_i2cWrite((char *)&(PlIfObj->AddrCmdBuf),sizeof(PlIfObj->AddrCmdBuf))) < sizeof(PlIfObj->AddrCmdBuf)) {
            CFE_EVS_SendEvent (PLIF_OBJ_I2C_EID, CFE_EVS_ERROR,
                              "Image address write status returned less than size of address command buffer %d. Downlink Cancelled", writeStatus);
            OS_close(PlIfObj->ImgFd);
            PlIfObj->DownlinkStatus = 2;
            PlIfObj->PerformDownlink = false;
            return;
         }


         OS_TaskDelay(I2C_WRITE_READ_DELAY);

         // Update our image buffer with the data from the pi
         memset(PlIfObj->ImgBuff,'\0',sizeof(PlIfObj->ImgBuff));
         if((readStatus = read(PlIfObj->i2cfd,PlIfObj->ImgBuff,IMG_SEG_SIZE)) != IMG_SEG_SIZE) {

            CFE_EVS_SendEvent (PLIF_OBJ_I2C_EID, CFE_EVS_ERROR,
                              "Image buffer read returned less than 16 bytes. Downlink Cancelled.");
            OS_close(PlIfObj->ImgFd);
            PlIfObj->DownlinkStatus = 2;
            PlIfObj->PerformDownlink = false;
            return;
         } else { //write the data to our file

            // If this is the last segment strip the null bytes
            if (readPtr == (numSegments-1)) {
               for (int j = (IMG_SEG_SIZE-1);j>=0;j--) {
                  if (PlIfObj->ImgBuff[j] != '\0') {
                     lastWriteSize = j+1;
                     break;
                  }
               }
               // Write the last segment to the file without the null bytes
               if (OS_write(PlIfObj->ImgFd,&PlIfObj->ImgBuff,lastWriteSize) < 0) {
                     CFE_EVS_SendEvent (PLIF_OBJ_I2C_EID, CFE_EVS_ERROR,
                                    "Error writing image file data to file. Downlink Cancelled.");
                     OS_close(PlIfObj->ImgFd);
                     PlIfObj->DownlinkStatus = 2;
                     PlIfObj->PerformDownlink = false;
                     return;
               }

            } else {
               // Write all but the last 16 byte segment to the file
               if (OS_write(PlIfObj->ImgFd,&PlIfObj->ImgBuff,IMG_SEG_SIZE) < 0) {
                  CFE_EVS_SendEvent (PLIF_OBJ_I2C_EID, CFE_EVS_ERROR,
                                 "Error writing image file data to file. Downlink Cancelled.");
                  OS_close(PlIfObj->ImgFd);
                  PlIfObj->DownlinkStatus = 2;
                  PlIfObj->PerformDownlink = false;
                  return;
               }
            }
         }

         OS_TaskDelay(I2C_INTER_MSG_DELAY);

         if (((imgAddr-16) % 1000) == 0) {
            // printf("Rx %d bytes: %d bytes: ", imgAddr, IMG_SEG_SIZE);
            //    for (int z = 0; z < IMG_SEG_SIZE; z++) {
            //    printf("%02X",PlIfObj->ImgBuff[z]);
            // }
            // printf("\n");
            PlIfObj->CurrentDownlinkAddr = imgAddr;
            PLIF_OBJ_SendPlStatusTlm();
         }

      }

      OS_close(PlIfObj->ImgFd);
      PlIfObj->DownlinkStatus = 2;
      PlIfObj->PerformDownlink = false;

   } else {
      CFE_EVS_SendEvent (PLIF_OBJ_I2C_EID, CFE_EVS_ERROR,
                        "Attempt to downlink image when i2c fd %d is invalid or payload is not yet alive %d", PlIfObj->i2cfd, PlIfObj->AliveStatus);
      PlIfObj->DownlinkStatus = 2;
      PlIfObj->PerformDownlink = false;
      return;

   }

}

/******************************************************************************
** Function: PLIF_OBJ_Execute
**
** Execute main object function.
**
*/
void PLIF_OBJ_Execute(void)
{

   if (PlIfObj->PowerStatus) {
         PLIF_OBJ_CheckStatus();

         PLIF_OBJ_SendPlStatusTlm();

      if(PlIfObj->PerformDownlink) {
         PLIF_OBJ_DownlinkImage();
      }

   }
   PlIfObj->ExecCnt++;

} /* PLIF_OBJ_Execute() */

/******************************************************************************
** Function:  PLIF_OBJ_ResetStatus
**
*/
void PLIF_OBJ_ResetStatus(void)
{

   PlIfObj->ExecCnt = 0;
   
} /* End PLIF_OBJ_ResetStatus() */


/******************************************************************************
** Function: PLIF_OBJ_GetTblPtr
**
*/
const EXTBL_Struct* PLIF_OBJ_GetTblPtr(void)
{

   return &(PlIfObj->Tbl);

} /* End PLIF_OBJ_GetTblPtr() */


/******************************************************************************
** Function: PLIF_OBJ_LoadTbl
**
*/
boolean PLIF_OBJ_LoadTbl(EXTBL_Struct* NewTbl)
{

   boolean  RetStatus = TRUE;

   CFE_EVS_SendEvent (PLIF_OBJ_DEMO_DEBUG_EID, CFE_EVS_DEBUG,"PLIF_OBJ_LoadTbl() Entered");

   /*
   ** This is a simple table copy. More complex table loads may have pass/fail 
   ** criteria.
   */

   CFE_PSP_MemCpy(&(PlIfObj->Tbl), NewTbl, sizeof(EXTBL_Struct));

   return RetStatus;

} /* End PLIF_OBJ_LoadTbl() */


/******************************************************************************
** Function: PLIF_OBJ_LoadTblEntry
**
*/
boolean PLIF_OBJ_LoadTblEntry(uint16 EntryId, EXTBL_Entry* NewEntry)
{

   boolean  RetStatus = TRUE;

   /* 
   ** This is a simple table entry copy. More complex table load may have 
   ** pass/fail criteria.
   */

   CFE_PSP_MemCpy(&(PlIfObj->Tbl.Entry[EntryId]),NewEntry,sizeof(EXTBL_Entry));

   return RetStatus;

} /* End PLIF_OBJ_LoadTblEntry() */

/******************************************************************************
** Function: PLIF_OBJ_i2cCmd
**
** Command the pi zero payload over i2c
**
*/
boolean PLIF_OBJ_i2cCmd(void* DataObjPtr, const CFE_SB_MsgPtr_t MsgPtr) {

   const PLIF_OBJ_i2cCmdMsg *CmdMsg = (const PLIF_OBJ_i2cCmdMsg *) MsgPtr;

   CFE_EVS_SendEvent (PLIF_OBJ_I2C_EID,
                      CFE_EVS_INFORMATION,
                      "i2c command received with parameter %d",
                      CmdMsg->Parameter);

   switch (CmdMsg->Parameter) {
      case TAKE_IMG:
      case REBOOT:
      case STOP:
         memset(PlIfObj->CmdBuff,'\0',sizeof(PlIfObj->CmdBuff));
         snprintf((char *)&(PlIfObj->CmdBuff),sizeof(PlIfObj->CmdBuff),"%d",CmdMsg->Parameter);
         PLIF_OBJ_i2cWrite((char *)&(PlIfObj->CmdBuff),sizeof(PlIfObj->CmdBuff));
         break;
      case DLINK_IMG:
         PlIfObj->PerformDownlink = true;
         PlIfObj->DownlinkStatus = 1;
         break;
      case IMG_SIZE:
         PLIF_OBJ_GetImageSize();
         break;
      default:
         CFE_EVS_SendEvent (PLIF_OBJ_I2C_EID, CFE_EVS_ERROR,
                           "Received unknown i2c command parameter %d",
                           CmdMsg->Parameter);
   }

   return TRUE;

}

/******************************************************************************
** Function: PLIF_OBJ_PowerCmd
**
** Enable/Disable payload pwoer.
**
*/
boolean PLIF_OBJ_PowerCmd(void* DataObjPtr, const CFE_SB_MsgPtr_t MsgPtr)
{

   const PLIF_OBJ_PowerCmdMsg *CmdMsg = (const PLIF_OBJ_PowerCmdMsg *) MsgPtr;

   CFE_EVS_SendEvent (PLIF_OBJ_DEMO_CMD_INFO_EID,
                      CFE_EVS_INFORMATION,
                      "Power command received with parameter %d",
                      CmdMsg->Parameter);

   if(CmdMsg->Parameter == 1) {
      if(PlIfObj->gpio_port) {
         gpiolib_set(PlIfObj->gpio_port,1,1);
         PlIfObj->PowerStatus = 1;
      }
   } else if (CmdMsg->Parameter == 0) {
      if(PlIfObj->gpio_port) {
	      gpiolib_set(PlIfObj->gpio_port,1,0);
         PlIfObj->PowerStatus = 0;
         PlIfObj->AliveStatus = false;

      }
   } else {
      CFE_EVS_SendEvent (PLIF_OBJ_GPIO_EID, CFE_EVS_INFORMATION,
                        "Unsupported GPIO setting.  Send 1(On) or 0(Off)");
   }

   return TRUE;

} /* End PLIF_OBJ_PowerCmd() */

/* end of file */
