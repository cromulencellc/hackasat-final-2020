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

#ifndef _plifobj_
#define _plifobj_

/*
** Includes
*/

#include "app_cfg.h"
#include "extbl.h"

/*
** Event Message IDs
*/

#define PLIF_OBJ_CMD_ENA_INFO_EID  (PLIF_OBJ_BASE_EID + 0)
#define PLIF_OBJ_DEMO_CMD_INFO_EID (PLIF_OBJ_BASE_EID + 1)
#define PLIF_OBJ_DEMO_DEBUG_EID    (PLIF_OBJ_BASE_EID + 2)
#define PLIF_OBJ_GPIO_EID          (PLIF_OBJ_BASE_EID + 3)
#define PLIF_OBJ_I2C_EID           (PLIF_OBJ_BASE_EID + 4)

#define GPIO_PORT_NR (0)
#define I2CMST_MINOR 1
#define PI_ADDR 0x09
#define IMG_SEG_SIZE 16

/* I2C Control Registers */
#define TAKE_IMG  0x0
#define REBOOT    0x1
#define STOP      0x2 

/* I2C Status Registers */
#define BUSY      0x3
#define IMG_READY 0x4
// 4 bytes 0x05 - 0x08
#define IMG_SIZE  0x5
#define BAD_ADDR  0x9

/* I2C Image Register */
#define IMG_REG   0x10
// Downlink image is only used by Cosmos
// for initiating the downlink from the pl
#define DLINK_IMG 0x30
#define I2C_WRITE_READ_DELAY 30
#define I2C_INTER_MSG_DELAY 10
#define I2C_IMG_SIZE_DELAY 30

#define IMG_FILE_PATH              "/cf/img.png"

/*
** Type Definitions
*/

typedef struct
{
   uint8    Header[CFE_SB_TLM_HDR_SIZE];
   uint8    BusyStatus;
   uint8    ImgReadyStatus;
   uint8    BadAddrStatus;
   uint8    AliveStatus;
   uint8    DownlinkStatus;
   uint8    Pad[3];
   uint32   ImgSize;
   uint32   CurrentDownlinkAddr;

} OS_ALIGN(4) PLIF_OBJ_PlStatusPkt;
#define PLIF_OBJ_PL_STATUS_DATA_LEN sizeof (PLIF_OBJ_PlStatusPkt)

/******************************************************************************
** PLIFObj_Class
*/

typedef struct {

   uint16       ExecCnt;

   EXTBL_Struct Tbl;
   PLIF_OBJ_PlStatusPkt PlStatusPkt;

   void *gpio_port;

   int i2cfd;

   // i2c command buffer
   char CmdBuff[2];
   // i2c address command buffer
   char AddrCmdBuf[6];
   // i2c status buffer
   uint8 StatusBuff;

   // For reading in ImgSize from PL
   uint8 ImgSizeBuff[4];
   // i2c image buffer.  Each read is 16 bytes
   uint8 ImgBuff[16];
   // Converted image size
   int   ImgSize;

   uint8 PowerStatus;
   uint8 BusyStatus;
   uint8 ImgReadyStatus;
   uint8 BadAddrStatus;
   uint8 DownlinkStatus;
   uint8 CurrentStatusRqst;
   uint8 CurrentImgRqst;
   bool  PerformDownlink;
   // Until the payload is initialized, the i2c
   // device returns an error status upon a write
   // attempt. Attempt a write and populate alive
   // status upon power on.
   uint8 AliveStatus;
   uint32 CurrentDownlinkAddr;
   int   ImgFd;

} PLIF_OBJ_Class;


/******************************************************************************
** Command Functions
*/

typedef struct
{

   uint8  CmdHeader[CFE_SB_CMD_HDR_SIZE];
   uint8  Parameter;

} PLIF_OBJ_PowerCmdMsg;
#define PLIF_OBJ_POWER_CMD_DATA_LEN  (sizeof(PLIF_OBJ_PowerCmdMsg) - CFE_SB_CMD_HDR_SIZE)

typedef struct
{

   uint8  CmdHeader[CFE_SB_CMD_HDR_SIZE];
   uint8  Parameter;

} PLIF_OBJ_i2cCmdMsg;
#define PLIF_OBJ_I2C_CMD_DATA_LEN  (sizeof(PLIF_OBJ_i2cCmdMsg) - CFE_SB_CMD_HDR_SIZE)

/*
** Exported Functions
*/

/******************************************************************************
** Function: PLIF_OBJ_Constructor
**
** Initialize the plif object to a known state
**
** Notes:
**   1. This must be called prior to any other function.
**   2. The table values are not populated. This is done when the table is 
**      registered with the table manager.
**
*/
void PLIF_OBJ_Constructor(PLIF_OBJ_Class *PlIfObjPtr);

/******************************************************************************
** Function: PLIF_OBJ_i2cOpen
**
**  Opens i2c device
**
*/
void PLIF_OBJ_i2cOpen(void);

/******************************************************************************
** Function: PLIF_OBJ_i2cWrite
**
**  Write to i2c device
**
*/
int PLIF_OBJ_i2cWrite(char* buff, int count);

/******************************************************************************
** Function: PLIF_OBJ_i2cRead
**
**  Read from i2c device
**
*/
int PLIF_OBJ_i2cRead(uint8* buff, int count);

/******************************************************************************
** Function: PLIF_OBJ_i2cClose
**
**  Close i2c device
**
*/
void PLIF_OBJ_i2cClose(void);

/******************************************************************************
** Function: PLIF_OBJ_SendPlStatusTlm
**
**  Send the PL status telemetry packet
**
*/
void PLIF_OBJ_SendPlStatusTlm(void);

/******************************************************************************
** Function: PLIF_OBJ_CheckStatus
**
**  Check the status of the payload
**
*/
void PLIF_OBJ_CheckStatus(void);

/******************************************************************************
** Function: PLIF_OBJ_DownlinkImage
**
**  Downink image from payload to cfs filesystem
**
*/
void PLIF_OBJ_DownlinkImage(void);

/******************************************************************************
** Function: PLIF_OBJ_Execute
**
** Execute main object function.
**
*/
void PLIF_OBJ_Execute(void);

/******************************************************************************
** Function: PLIF_OBJ_ResetStatus
**
** Reset counters and status flags to a known reset state.
**
** Notes:
**   1. Any counter or variable that is reported in HK telemetry that doesn't
**      change the functional behavior should be reset.
**
*/
void PLIF_OBJ_ResetStatus(void);

/******************************************************************************
** Function: PLIF_OBJ_GetTblPtr
**
** Get pointer to PlIfObj's table data
**
** Note:
**  1. This function must match the EXTBL_GetTblPtr definition.
**  2. Supplied as a callback to ExTbl.
*/
const EXTBL_Struct* PLIF_OBJ_GetTblPtr(void);


/******************************************************************************
** Function: PLIF_OBJ_LoadTbl
**
** Load data into PlIfObj's example table.
**
** Note:
**  1. This function must match the EXTBL_LoadTblFunc definition.
**  2. Supplied as a callback to ExTbl.
*/
boolean PLIF_OBJ_LoadTbl (EXTBL_Struct* NewTbl);


/******************************************************************************
** Function: PLIF_OBJ_LoadEntry
**
** Load data into PlIfObj's example table.
**
** Note:
**  1. This function must match the EXTBL_LoadEntryFunc definition.
**  2. Supplied as a callback to ExTbl.
*/
boolean PLIF_OBJ_LoadTblEntry (uint16 EntryId, EXTBL_Entry* TblEntry);

/******************************************************************************
** Function: PLIF_OBJ_i2cCmd
**
** Command the pi zero payload over i2c
**
*/
boolean PLIF_OBJ_i2cCmd(void* DataObjPtr, const CFE_SB_MsgPtr_t MsgPtr);

/******************************************************************************
** Function: PLIF_OBJ_PowerCmd
**
** Enable/Disable payload power.
**
** Note:
**  1. This function must comply with the CMDMGR_CmdFuncPtr definition
*/
boolean PLIF_OBJ_PowerCmd(void* DataObjPtr, const CFE_SB_MsgPtr_t MsgPtr);

#endif /* _plifobj_ */
