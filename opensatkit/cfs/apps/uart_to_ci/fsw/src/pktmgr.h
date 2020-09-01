/* 
** Purpose: Manage the Packet Table that defines which packets will be sent from the
**          software bus to a socket.
**
** Notes:
**   None
**
** License:
**   Written by David McComas, licensed under the copyleft GNU
**   General Public License (GPL). 
**
** References:
**   1. OpenSatKit Object-based Application Developer's Guide.
**   2. cFS Application Developer's Guide.
**
*/

#ifndef _pktmgr_
#define _pktmgr_

/*
** Includes
*/
#include <termios.h>
#include "app_cfg.h"
#include "pkttbl.h"
#include "trans_rs422.h"
#include "queue.h"
#include <rtems/termiostypes.h>

#define PKTMGR_TX_SEM_NAME "PKTMGR_TX_SEM"


#define PKTMGR_CONSOLE_NAME_SIZE 32

/*
** Event Message IDs
*/
#define PKTMGR_CONSOLE_SEND_ERR_EID                 (PKTMGR_BASE_EID +  0)
#define PKTMGR_LOAD_TBL_SUBSCRIBE_ERR_EID           (PKTMGR_BASE_EID +  1)
#define PKTMGR_LOAD_TBL_INFO_EID                    (PKTMGR_BASE_EID +  2)
#define PKTMGR_LOAD_TBL_ERR_EID                     (PKTMGR_BASE_EID +  3)
#define PKTMGR_LOAD_TBL_ENTRY_SUBSCRIBE_ERR_EID     (PKTMGR_BASE_EID +  4)
#define PKTMGR_LOAD_TBL_ENTRY_INFO_EID              (PKTMGR_BASE_EID +  5)
#define PKTMGR_TLM_OUTPUT_ENA_INFO_EID              (PKTMGR_BASE_EID +  6)
#define PKTMGR_TLM_OUTPUT_ENA_SOCKET_ERR_EID        (PKTMGR_BASE_EID +  7)
#define PKTMGR_ADD_PKT_INFO_EID                     (PKTMGR_BASE_EID +  8)
#define PKTMGR_ADD_PKT_NO_FREE_ENTRY_ERR_EID        (PKTMGR_BASE_EID +  9)
#define PKTMGR_ADD_PKT_DUPLICATE_ENTRY_EID          (PKTMGR_BASE_EID + 10)
#define PKTMGR_ADD_PKT_SUBSCRIPTION_ERR_EID         (PKTMGR_BASE_EID + 11) 
#define PKTMGR_REMOVE_PKT_INFO_EID                  (PKTMGR_BASE_EID + 12)
#define PKTMGR_REMOVE_PKT_SB_ERR_EID                (PKTMGR_BASE_EID + 13)
#define PKTMGR_REMOVE_PKT_NOT_FOUND_ERR_EID         (PKTMGR_BASE_EID + 14)
#define PKTMGR_REMOVE_ALL_PKTS_UNSUBSCRIBE_ERR_EID  (PKTMGR_BASE_EID + 15)
#define PKTMGR_REMOVE_ALL_PKTS_INFO_EID             (PKTMGR_BASE_EID + 16)
#define PKTMGR_REMOVE_ALL_PKTS_ERR_EID              (PKTMGR_BASE_EID + 17)
#define PKTMGR_DESTRUCTOR_INFO_EID                  (PKTMGR_BASE_EID + 18)
#define PKTMGR_CMD_INGEST_ERR_EID                   (PKTMGR_BASE_EID + 19)
#define PKTMGR_CHILD_INIT_ERR_EID                   (PKTMGR_BASE_EID + 20)
#define PKTMGR_CHILD_TERM_ERR_EID                   (PKTMGR_BASE_EID + 21)

#define PKTMGR_TOTAL_EID  22

#define CI_CONFIG_SERIAL_PORT      "/dev/console_b"
#define CI_CONFIG_HIGH_BAUD_RATE   19200  //bps - gets converted to appropriate format in trans_rs422.c
#define CI_CONFIG_LOW_BAUD_RATE    9600   //bps - gets converted to appropriate format in trans_rs422.c
#define CI_CONFIG_TIMEOUT          0      //milliseconds.  Gets converted to 100ms chunks in trans_rs422.c
#define CI_CONFIG_MINBYTES         0

#define CI_CUSTOM_BUFFER_SIZE          4096
#define TO_CUSTOM_BUFFER_SIZE          4096

#define PKTMGR_CHILD_SEM_NAME          "PKTMGR_CHILD_SEM"
#define PKTMGR_CHILD_TASK_NAME         "PKTMGR_CHILD"
#define PKTMGR_CHILD_TASK_STACK_SIZE   20480
#define PKTMGR_CHILD_TASK_PRIORITY     30
#define PKTMGR_CHILD_SEM_INVALID       0xFFFFFFFF
#define PKTMGR_CHILD_TASK_DELAY        10
#define PKTMGR_CHILD_TIMEOUT_MAX       5

/*
** Type Definitions
*/

/******************************************************************************
** Packet Manager Class
*/

typedef struct {

   CFE_SB_PipeId_t   TlmPipe;
   boolean           DownlinkOn;
   boolean           SuppressSend;
   PKTTBL_Tbl        Tbl;

   bool              connected;
   int               fd;
   int               totalBytesSent;
   int               startSeconds;
   int               elapsedSeconds;
   uint8             syncPattern[4];

   IO_TransRS422Config_t config;

   // Deal with the Uart Rx Buffer
   struct  ttywakeup RxWake;
   QUEUE_t           RecvQueue; 
   uint8             RecvBuff[CI_CUSTOM_BUFFER_SIZE]; 
 
   // Ingesting Commmands
   uint8             CmdBuff[CI_CUSTOM_BUFFER_SIZE];
   uint32            CmdLen;     // Current len of CmdBuff
   CFE_SB_MsgId_t    CmdMsgId;   // Current Cmd MsgId
   uint32            CmdMsgSize; // Expected Cmd length, Including SyncPattern 

   // Failure Handling
   uint8             CmdRecvTimeoutCnt;
   uint32            LastRecvCmdLen;
   uint32            RecvCmdErrCnt;
   uint32            RecvCmdCnt;
   uint32            SentTlmCnt;


   // Deal with the UART Tx Buffer
   // struct  ttywakeup TxWake;
   QUEUE_t           SendQueue;
   uint8             SendBuff[TO_CUSTOM_BUFFER_SIZE];

   uint32            ChildTaskID;
   uint32            ChildSemaphore;
} PKTMGR_Class;


/******************************************************************************
** Command Packets
*/

typedef struct
{

   uint8   CmdHeader[CFE_SB_CMD_HDR_SIZE];

} PKTMGR_EnableOutputCmdParam;
#define PKKTMGR_ENABLE_OUTPUT_CMD_DATA_LEN  (sizeof(PKTMGR_EnableOutputCmdParam) - CFE_SB_CMD_HDR_SIZE)


typedef struct
{

   uint8              CmdHeader[CFE_SB_CMD_HDR_SIZE];
   CFE_SB_MsgId_t     StreamId;
   CFE_SB_Qos_t       Qos;
   uint8              BufLim;

}  OS_PACK PKTMGR_AddPktCmdParam;
#define PKKTMGR_ADD_PKT_CMD_DATA_LEN  (sizeof(PKTMGR_AddPktCmdParam) - CFE_SB_CMD_HDR_SIZE)


typedef struct
{

   uint8              CmdHeader[CFE_SB_CMD_HDR_SIZE];
   CFE_SB_MsgId_t     StreamId;

}  PKTMGR_RemovePktCmdParam;
#define PKKTMGR_REMOVE_PKT_CMD_DATA_LEN  (sizeof(PKTMGR_RemovePktCmdParam) - CFE_SB_CMD_HDR_SIZE)

typedef struct
{

   uint8   CmdHeader[CFE_SB_CMD_HDR_SIZE];

} PKTMGR_DisableOutputCmdParam;
#define PKKTMGR_DISABLE_OUTPUT_CMD_DATA_LEN  (sizeof(PKTMGR_DisableOutputCmdParam) - CFE_SB_CMD_HDR_SIZE)

typedef struct
{

   uint8   CmdHeader[CFE_SB_CMD_HDR_SIZE];
   uint8   Rate;

} PKTMGR_SetRateCmdParam;
#define PKKTMGR_SET_RATE_CMD_DATA_LEN  (sizeof(PKTMGR_SetRateCmdParam) - CFE_SB_CMD_HDR_SIZE)
/*
** Exported Functions
*/

/******************************************************************************
** Function: PKTMGR_Constructor
**
** Construct a PKTMGR object. All table entries are cleared and the LoadTbl()
** function should be used to load an initial table.
**
** Notes:
**   1. This must be called prior to any other function.
**   2. Decoupling the initial table load gives an app flexibility in file
**      management during startup.
**
*/
void PKTMGR_Constructor(PKTMGR_Class *PktMgrPtr, char* PipeName, uint16 PipeDepth);

/******************************************************************************
** Function: Rx_Wake
**
*/
void Rx_Wake(struct termios *tty,void*arg);

/******************************************************************************
** Function: Tx_Wake
**
*/
// void Tx_Wake(struct termios *tty,void*arg);

/******************************************************************************
** Function: PKTMGR_GetTblPtr
**
** Return a pointer to the packet table.
**
** Notes:
**   1. Function signature must match PKTTBL_GetTblPtr
**
*/
const PKTTBL_Tbl* PKTMGR_GetTblPtr(void);


/******************************************************************************
** Function: PKTMGR_ResetStatus
**
** Reset counters and status flags to a known reset state.
**
** Notes:
**   1. Any counter or variable that is reported in HK telemetry that doesn't
**      change the functional behavior should be reset.
**
*/
void PKTMGR_ResetStatus(void);


/******************************************************************************
** Function: PKTMGR_ConsoleConnect
**
**  Open the command and telemetry console
**
*/
void PKTMGR_ConsoleConnect(void);

/******************************************************************************
** Function: PKTMGR_ConsoleDisconnect
**
** Closes telemetry console file descriptor
**
*/
void PKTMGR_ConsoleDisconnect(void);

/******************************************************************************
** Function: PKTMGR_CheckCmdTimeout
**
** This function checks for a partial command buffer that hasn't been cleared
** after 4 cycles of the packet manager function.  Primarily concerned with a
** lost command link that leaves a partial command in the buffer, or a corrupt
** incorrect command header length that leaves the command ingest function
** looking for more data. 
**
*/
void PKTMGR_CheckCmdTimeout(void);

/******************************************************************************
** Function: PKTMGR_OutputTelemetry
**
** If downlink is enabled and output hasn't been suppressed it sends all of the
** SB packets on the telemetry input pipe out the socket.
**
*/
void PKTMGR_OutputTelemetry(void);

/******************************************************************************
** Function: PKTMGR_IngestCommands
**
** Read a command message into the Rx buffer based on the rx wake signal
**
*/
void PKTMGR_IngestCommands();

/******************************************************************************
** Function: PKTMGR_SendTelemetry
** Write telemetry packets to telemetry console (UART3)
**
*/
void PKTMGR_SendTelemetry(CFE_SB_Msg_t* PktPtr, uint16 PktLen);

/******************************************************************************
** Function: PKTMGR_LoadTbl
**
** Unsubscribe from all current SB telemetry, flushes the telemetry input pipe,
** loads the entire new table and subscribes to each message.
**
** Notes:
**   1. No validity checks are performed on the table data.
**   2. Function signature must match PKTTBL_LoadTbl
**
*/
boolean PKTMGR_LoadTbl(PKTTBL_Tbl* NewTbl);


/******************************************************************************
** Function: PKTMGR_LoadTblEntry
**
** Load a single message table entry
**
** Notes:
**   1. Range checking is not performed on the parameters.
**   2. Function signature must match PKTTBL_LoadTblEntry
**
*/
boolean PKTMGR_LoadTblEntry(uint16 PktIdx, PKTTBL_Pkt* PktArray);


/******************************************************************************
** Function: PKTMGR_EnableTlmOutputCmd
**
** The command and telemetry console connection occurs on startup to support
** command reception. If already connected, this function simply enables the
** tlm output.  If not connected, we attempt to connect.
**
*/
boolean PKTMGR_EnableTlmOutputCmd(void* ObjDataPtr, const CFE_SB_MsgPtr_t MsgPtr);

/******************************************************************************
** Function: PKTMGR_DisableOutputCmd
**
** Disable telemetry output 
**
*/
boolean PKTMGR_DisableTlmOutputCmd(void* ObjDataPtr, const CFE_SB_MsgPtr_t MsgPtr);

/******************************************************************************
** Function: PKTMGR_SetRateCmd
**
** Set the command and telemetry rate to either high (19200) or low (9600)
**
*/
boolean PKTMGR_SetRateCmd(void* ObjDataPtr, const CFE_SB_MsgPtr_t MsgPtr);

/******************************************************************************
** Function: PKTMGR_AddPktCmd
**
** Add a packet to the table and subscribe for it on the SB.
*/
boolean PKTMGR_AddPktCmd(void* ObjDataPtr, const CFE_SB_MsgPtr_t MsgPtr);


/******************************************************************************
** Function: PKTMGR_RemovePktCmd
**
** Remove a packet from the table and unsubscribe from receiving it on the SB.
**
** Notes:
**   1. Don't consider trying to remove an non-existent entry an error
*/
boolean PKTMGR_RemovePktCmd(void* ObjDataPtr, const CFE_SB_MsgPtr_t MsgPtr);


/******************************************************************************
** Function: PKTMGR_RemoveAllPktsCmd
**
** Remove all packets from the table and unsubscribe from receiving them on the
** SB.
*/
boolean PKTMGR_RemoveAllPktsCmd(void* ObjDataPtr, const CFE_SB_MsgPtr_t MsgPtr);

#endif /* _pktmgr_ */

/******************************************************************************
** Function: PKTMGR_ChildInit 
*/
void PKTMGR_ChildInit(void);

/******************************************************************************
** Function: PKTMGR_ChildTask
*/
void PKTMGR_ChildTask(void);

/******************************************************************************
** Function: PKTMGR_ChildLoop
*/
void PKTMGR_ChildLoop(void);