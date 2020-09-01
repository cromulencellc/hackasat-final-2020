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

#ifndef _eyassatobj_
#define _eyassatobj_

/*
** Includes
*/
#include <termios.h>
#include "app_cfg.h"
#include "eyassat_tbl.h"
#include "trans_rs422.h"
#include <rtems/termiostypes.h>

/*
** Event Message IDs
*/

#define EYASSATOBJ_CMD_ENA_INFO_EID  (EYASSATOBJ_BASE_EID + 0)
#define EYASSATOBJ_CMD_INFO_EID      (EYASSATOBJ_BASE_EID + 1)
#define EYASSATOBJ_DEBUG_EID         (EYASSATOBJ_BASE_EID + 2)
#define EYASSATOBJ_CONNECT_EID       (EYASSATOBJ_BASE_EID + 3)
#define EYASSATOBJ_CHILD_TASK_EID    (EYASSATOBJ_BASE_EID + 4)

// Buffer size for incoming telemetry on the UART
#define EYASSATOBJ_TLM_BUFFER_SIZE     800
#define EYASSATOBJ_CMD_BUFFER_SIZE     100
#define EYASSATOBJ_CMD_BUFFER_DEPTH    32


#define EYASSAT_CONFIG_SERIAL_PORT      "/dev/console_c"
#define EYASSAT_CONFIG_BAUD_RATE        115200  //bps - gets converted to appropriate format in trans_rs422.c
#define EYASSAT_CONFIG_TIMEOUT          0       //milliseconds.  Gets converted to 100ms chunks in trans_rs422.c
#define EYASSAT_CONFIG_MINBYTES         0
/*
** Type Definitions
*/
typedef struct
{
   uint8    Header[CFE_SB_TLM_HDR_SIZE];
   char     TimeString[8];
   char     CallSign[3];
   uint8    PacketId;

} OS_ALIGN(4) ESHdr;
#define EYASSATOBJ_HDR_LEN sizeof (ESHdr)

typedef struct
{
   ESHdr    Hdr;
   uint8    TimeDelay;
   uint8    PwrTlm;
   uint8    AdcsTlm;
   uint8    ExpTlm;
   uint16   CmdTimeOut;
   uint16   Pad;

} OS_ALIGN(4) EYASSATOBJ_InternalPkt;
#define EYASSATOBJ_INTERNAL_LEN sizeof (EYASSATOBJ_InternalPkt)

typedef struct
{
   ESHdr    Hdr;
   float    DHTemp;
   float    ExpTemp;
   float    RefTemp;
   float    PanelATemp;
   float    PanelBTemp;
   float    BaseTemp;
   float    TopATemp;
   float    TopBTemp;


} OS_ALIGN(4) EYASSATOBJ_TempPkt;
#define EYASSATOBJ_TEMP_LEN sizeof (EYASSATOBJ_TempPkt)

typedef struct
{
   ESHdr    Hdr;
   uint8    SepStatus;
   uint8    Pad;
   uint16   SwitchStatus;
   float    VBatt;
   float    IBatt;
   float    VSA;
   float    ISA;
   float    IMB;
   float    V5V;
   float    I5V;
   float    V3V;
   float    I3V;
   float    BattTemp;
   float    SA1Temp;
   float    SA2Temp;
   // PWR_3V, PWR_ADCS, PWR_EXP, PWR_HTR1, PWR_HTR2 are derived telemetry points

} OS_ALIGN(4) EYASSATOBJ_PowerPkt;
#define EYASSATOBJ_POWER_LEN sizeof (EYASSATOBJ_PowerPkt)

typedef struct
{
   ESHdr    Hdr;
   uint8    XRod;
   uint8    YRod;
   uint8    CtrlMode;
   uint8    Pad[3];
   // uint8    MagEnabled;
   // uint8    AccEnabled;
   // uint16   Pad;
   uint16   SunTop;
   uint16   SunBottom;
   uint16   PWM;
   float    SunYawAng;
   float    Yaw;
   float    Pitch;
   float    Roll;
   float    MagX;
   float    MagY;
   float    MagZ;
   float    AccX;
   float    AccY;
   float    AccZ;
   float    RotX; //Angular rotation about axis in deg/s
   float    RotY;
   float    RotZ;
   float    ActWheelSpd; //rps
   float    CmdWheelSpd; //rps
   float    WheelAngMom; //Nms
   float    DeltaT;
   float    YawCmd;
   float    PointingError;
   float    Deadband;
   float    Extra;
   float    Kp;
   float    Ki;
   float    Kd;
   float    DeadBandScaleFactor;

} OS_ALIGN(4) EYASSATOBJ_ADCSPkt;

#define EYASSATOBJ_ADCS_LEN sizeof (EYASSATOBJ_ADCSPkt)

/******************************************************************************
** EyasSatObj_Class
*/

typedef struct {

   uint16                  ExecCnt;

   EYASSATTBL_Struct       Tbl;
   EYASSATOBJ_InternalPkt  InternalPkt;
   EYASSATOBJ_TempPkt      TempPkt;
   EYASSATOBJ_PowerPkt     PowerPkt;
   EYASSATOBJ_ADCSPkt      ADCSPkt;

   char  telemetryBuffer[EYASSATOBJ_TLM_BUFFER_SIZE];
   char  commandBuffer[EYASSATOBJ_CMD_BUFFER_DEPTH][EYASSATOBJ_CMD_BUFFER_SIZE];
   int   fd; // File descriptor for telemetry UART connection
   bool  connected;

   struct ttywakeup  RxWake;
   uint32            RxPtr;
   uint8             CmdBufferHead;
   uint8             CmdBufferTail;
   bool              CmdBufferFull;
   bool              newTelemetry;
   bool              updateConfig;
   bool              updateDB;
   bool              updatePID;
   bool              updateMagCal;
   bool              updateGyroCal;

   // Using specific flags for power and adcs
   // to drive updates/stalness since they
   // are not always on like internal and temp
   bool              PowerTlmUpdate;
   bool              AdcsTlmUpdate;

   bool              tableInit;

   IO_TransRS422Config_t config;

} EYASSATOBJ_Class;


/******************************************************************************
** Command Functions
*/

typedef struct
{

   uint8   CmdHeader[CFE_SB_CMD_HDR_SIZE];
   uint16  Parameter;

} EYASSATOBJ_DemoCmdMsg;
#define EYASSATOBJ_DEMO_CMD_DATA_LEN  (sizeof(EYASSATOBJ_DemoCmdMsg) - CFE_SB_CMD_HDR_SIZE)

typedef struct
{

   uint8   CmdHeader[CFE_SB_CMD_HDR_SIZE];
   char    CmdCode[2];

} OS_PACK EYASSATOBJ_DiscreteCmdMsg;
#define EYASSATOBJ_DISCRETE_CMD_DATA_LEN  (sizeof(EYASSATOBJ_DiscreteCmdMsg) - CFE_SB_CMD_HDR_SIZE)

typedef struct
{

   uint8   CmdHeader[CFE_SB_CMD_HDR_SIZE];
   char    CmdCode[2];
   uint8   CmdParameter;

} OS_PACK EYASSATOBJ_Uint8CmdMsg;
#define EYASSATOBJ_UINT8_CMD_DATA_LEN  (sizeof(EYASSATOBJ_Uint8CmdMsg) - CFE_SB_CMD_HDR_SIZE)

typedef struct
{

   uint8   CmdHeader[CFE_SB_CMD_HDR_SIZE];
   char    CmdCode[2];
   uint16  CmdParameter;

} OS_PACK EYASSATOBJ_Uint16CmdMsg;
#define EYASSATOBJ_UINT16_CMD_DATA_LEN  (sizeof(EYASSATOBJ_Uint16CmdMsg) - CFE_SB_CMD_HDR_SIZE)

typedef struct
{

   uint8   CmdHeader[CFE_SB_CMD_HDR_SIZE];
   char    CmdCode[2];
   float   CmdParameter;

} OS_PACK EYASSATOBJ_FloatCmdMsg;
#define EYASSATOBJ_FLOAT_CMD_DATA_LEN  (sizeof(EYASSATOBJ_FloatCmdMsg) - CFE_SB_CMD_HDR_SIZE)

typedef struct
{

   uint8   CmdHeader[CFE_SB_CMD_HDR_SIZE];

} OS_PACK EYASSATOBJ_ConnectCmdMsg;
#define EYASSATOBJ_CONNECT_CMD_DATA_LEN  (sizeof(EYASSATOBJ_ConnectCmdMsg) - CFE_SB_CMD_HDR_SIZE)

typedef struct
{

   uint8   CmdHeader[CFE_SB_CMD_HDR_SIZE];

} OS_PACK EYASSATOBJ_DisconnectCmdMsg;
#define EYASSATOBJ_DISCONNECT_CMD_DATA_LEN  (sizeof(EYASSATOBJ_DisconnectCmdMsg) - CFE_SB_CMD_HDR_SIZE)

typedef struct
{

   uint8    CmdHeader[CFE_SB_CMD_HDR_SIZE];
   float    MagCalX;
   float    MagCalY;
   float    MagCalZ;

} OS_PACK EYASSATOBJ_MagCalCmdMsg;
#define EYASSATOBJ_MAGCAL_CMD_DATA_LEN  (sizeof(EYASSATOBJ_MagCalCmdMsg) - CFE_SB_CMD_HDR_SIZE)

typedef struct
{

   uint8    CmdHeader[CFE_SB_CMD_HDR_SIZE];
   float    GyroCalX;
   float    GyroCalY;
   float    GyroCalZ;

} OS_PACK EYASSATOBJ_GyroCalCmdMsg;
#define EYASSATOBJ_GYROCAL_CMD_DATA_LEN  (sizeof(EYASSATOBJ_GyroCalCmdMsg) - CFE_SB_CMD_HDR_SIZE)

/*
** Exported Functions
*/

/******************************************************************************
** Function: EYASSATOBJ_Constructor
**
** Initialize the example object to a known state
**
** Notes:
**   1. This must be called prior to any other function.
**   2. The table values are not populated. This is done when the table is 
**      registered with the table manager.
**
*/
void EYASSATOBJ_Constructor(EYASSATOBJ_Class *EyasSatObjPtr);

/******************************************************************************
** Function: Rx_Wake
**
*/
void Rx_Wake(struct termios *tty,void*arg);

/******************************************************************************
** Function: CmdBufferFull
**
** Check to see if the command buffer is full
**
*/
boolean CmdBufferFull(void);

/******************************************************************************
** Function: CmdBufferEmpty
**
** Checks if the commmand buffer is empty
**
*/
boolean CmdBufferEmpty(void);

/******************************************************************************
** Function: CmdBufferFreeSpace
**
** Checks for free space in the command buffer
**
*/
uint8 CmdBufferFreeSpace(void);

/******************************************************************************
** Function: CmdBufferAdvancePointer
**
** Advance the command buffer when adding a command to the buffer
**
*/
void CmdBufferAdvancePointer(void);

/******************************************************************************
** Function: CmdBufferRetreatPointer
**
** Retreat the command buffer tail pointer after sending a command
**
*/
void CmdBufferRetreatPointer(void);

/******************************************************************************
** Function: CmdBufferPutCmd
**
** Add a command to the command buffer
**
*/
void CmdBufferPutCmd(char* Cmd);

/******************************************************************************
** Function: EYASSATOBJ_Execute
**
** Execute main object function.
**
*/
void EYASSATOBJ_Execute(void);

/******************************************************************************
** Function: EYASSATOBJ_ConsoleConnect
**
** Connect to EyasSat console.
**
*/
void EYASSATOBJ_ConsoleConnect(void);

/******************************************************************************
** Function: EYASSATOBJ_ConsoleDisconnect
**
** Closes EyasSat console file descriptor
**
*/
void EYASSATOBJ_ConsoleDisconnect(void);

/******************************************************************************
** Function: EYASSATOBJ_ProcessTelemetry
**
** Process telemetry messages from EyasSat C&DH board.
**
*/
void EYASSATOBJ_ProcessTelemetry(void);

/******************************************************************************
** Function: ProcessPacketGroup
** 
** Receives an entire group of EyasSat packets per processing cycle
** Based on the packet id, it calls the appropriate processing function
**
*/
void EYASSATOBJ_ProcessPacketGroup(int totalBytesRead);

/******************************************************************************
** Function: EYASSATOBJ_ProcessInternalPacket
** 
** Processes the EyasSat Internal telemetry packet and populates the appropriate structure
**
*/
void EYASSATOBJ_ProcessInternalPacket(int pktIndex);

/******************************************************************************
** Function: EYASSATOBJ_ProcessTempPacket
** 
** Processes the EyasSat Temperature telemetry packet and populates the appropriate structure
**
*/
void EYASSATOBJ_ProcessTempPacket(int pktIndex);

/******************************************************************************
** Function: EYASSATOBJ_ProcessPowerPacket
** 
** Processes the EyasSat Power telemetry packet and populates the appropriate structure
**
*/
void EYASSATOBJ_ProcessPowerPacket(int pktIndex);

/******************************************************************************
** Function: EYASSATOBJ_ProcessADCSPacket
** 
** Processes the EyasSat ADCS telemetry packet and populates the appropriate structure
**
*/
void EYASSATOBJ_ProcessADCSPacket(int pktIndex);

/******************************************************************************
** Function: EYASSATOBJ_InitSat
**
** Command to turn on pwr_tlm, adcs_tlm, and enable IMU on startup
**
*/
void EYASSATOBJ_InitSat();

/******************************************************************************
** Function: EYASSATOBJ_InitSat
**
** Command to turn on pwr_tlm, adcs_tlm, and enable IMU on startup
**
*/
void EYASSATOBJ_InitSat();

/******************************************************************************
** Function: EYASSATOBJ_MagCalCmd
**
** Command to set magnetometer calibration offsets
**
*/
boolean EYASSATOBJ_MagCalCmd(void* DataObjPtr, const CFE_SB_MsgPtr_t MsgPtr);

/******************************************************************************
** Function: EYASSATOBJ_GyroCalCmd
**
** Command to set gyro calibration offsets
**
*/
boolean EYASSATOBJ_GyroCalCmd(void* DataObjPtr, const CFE_SB_MsgPtr_t MsgPtr);

/******************************************************************************
** Function: EYASSATOBJ_SendCommand
**
** Send the current command in the buffer to the C&DH board
**
*/
void EYASSATOBJ_SendCommand();

/******************************************************************************
** Function: EYASSATOBJ_ResetStatus
**
** Reset counters and status flags to a known reset state.
**
** Notes:
**   1. Any counter or variable that is reported in HK telemetry that doesn't
**      change the functional behavior should be reset.
**
*/
void EYASSATOBJ_ResetStatus(void);

/******************************************************************************
** Function: EYASSATOBJ_GetTblPtr
**
** Get pointer to EyasSatObj's table data
**
** Note:
**  1. This function must match the EXTBL_GetTblPtr definition.
**  2. Supplied as a callback to ExTbl.
*/
const EYASSATTBL_Struct* EYASSATOBJ_GetTblPtr(void);

/******************************************************************************
** Function: EYASSATOBJ_UpdateConfigParams
**
*/
boolean EYASSATOBJ_UpdateConfigParams();

/******************************************************************************
** Function: EYASSATOBJ_UpdateDBParams
**
*/
boolean EYASSATOBJ_UpdateDBParams();

/******************************************************************************
** Function: EYASSATOBJ_UpdatePIDParams
**
*/
boolean EYASSATOBJ_UpdatePIDParams();

/******************************************************************************
** Function: EYASSATOBJ_UpdateMagCalParams
**
*/
boolean EYASSATOBJ_UpdateMagCalParams();

/******************************************************************************
** Function: EYASSATOBJ_UpdateGyroCalParams
**
*/
boolean EYASSATOBJ_UpdateGyroCalParams();

/******************************************************************************
** Function: EYASSATOBJ_LoadTbl
**
** Load data into EyasSatObj's example table.
**
** Note:
**  1. This function must match the EXTBL_LoadTblFunc definition.
**  2. Supplied as a callback to ExTbl.
*/
boolean EYASSATOBJ_LoadTbl (EYASSATTBL_Struct* NewTbl);


/******************************************************************************
** Function: EYASSATOBJ_LoadEntry
**
** Load data into EyasSatObj's example table.
**
** Note:
**  1. This function must match the EXTBL_LoadEntryFunc definition.
**  2. Supplied as a callback to ExTbl.
*/
boolean EYASSATOBJ_LoadTblEntry(uint16 ObjId, void* ObjData);


/******************************************************************************
** Function: EYASSATOBJ_DemoCmd
**
** Demonstrate an 'entity' object having a command.
**
** Note:
**  1. This function must comply with the CMDMGR_CmdFuncPtr definition
*/
boolean EYASSATOBJ_DemoCmd(void* DataObjPtr, const CFE_SB_MsgPtr_t MsgPtr);

/******************************************************************************
** Function: EYASSATOBJ_DiscreteCmd
**
** Process EyasSat command messages with no command parameters
**
*/
boolean EYASSATOBJ_DiscreteCmd(void* DataObjPtr, const CFE_SB_MsgPtr_t MsgPtr);

/******************************************************************************
** Function: EYASSATOBJ_Uint8Cmd
**
** Process EyasSat command messages with uint8 command parameters
**
*/
boolean EYASSATOBJ_Uint8Cmd(void* DataObjPtr, const CFE_SB_MsgPtr_t MsgPtr);

/******************************************************************************
** Function: EYASSATOBJ_Uint16Cmd
**
** Process EyasSat command messages with uint16 command parameters
**
*/
boolean EYASSATOBJ_Uint16Cmd(void* DataObjPtr, const CFE_SB_MsgPtr_t MsgPtr);

/******************************************************************************
** Function: EYASSATOBJ_FloatCmd
**
** Process EyasSat command messages with float command parameters
**
*/
boolean EYASSATOBJ_FloatCmd(void* DataObjPtr, const CFE_SB_MsgPtr_t MsgPtr);

/******************************************************************************
** Function: EYASSATOBJ_ConnectCmd
**
** Command to connect to the EyasSat UART port. Connection occurs on startup
** This commmand is only required if a disconnect has been performed manually
**
*/
boolean EYASSATOBJ_ConnectCmd(void* DataObjPtr, const CFE_SB_MsgPtr_t MsgPtr);

/******************************************************************************
** Function: EYASSATOBJ_DisconnectCmd
**
** Command to disconnect from the EyasSat UART port.
**
*/
boolean EYASSATOBJ_DisconnectCmd(void* DataObjPtr, const CFE_SB_MsgPtr_t MsgPtr);

#endif /* _eyassatobj_ */
