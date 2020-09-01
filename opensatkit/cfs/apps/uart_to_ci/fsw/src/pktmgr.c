/* 
** Purpose: Manage Packet Table that defines which packets will be sent from the
**          software bus to a socket.
**
** Notes:
**   1. This is much simpler than a typical flight TO app. If a packet is defined
**      in the table then it will be subscribed to and every packet received will 
**      be output over the socket. No filter algorithms are available.
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

/*
** Include Files:
*/

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "app_cfg.h"
#include "pktmgr.h"

#include <rtems.h>
#include <grlib/apbuart.h>
#include <rtems/termiostypes.h>

/*
** Global File Data
*/

static PKTMGR_Class*  PktMgr = NULL;
static PKTTBL_Pkt     UnusedTblPkt = {PKTTBL_UNUSED_MSG_ID, {0, 0}, 0};

/*
** Local Function Prototypes
*/

static void DestructorCallback(void);
static void FlushTlmPipe(void);
static int  StreamIdPktIdx(uint16 StreamId);
static int32 SubscribeNewPkt(PKTTBL_Pkt* NewPkt); 
static int  UnusedTblPktIdx(void);

/******************************************************************************
** Function: PKTMGR_Constructor
**
*/
void PKTMGR_Constructor(PKTMGR_Class*  PktMgrPtr, char* PipeName, uint16 PipeDepth)
{

   int i;
   int32 Result = 0;
   PktMgr = PktMgrPtr;

   PktMgr->DownlinkOn   = FALSE;
   PktMgr->SuppressSend = TRUE;

   PktMgr->CmdRecvTimeoutCnt = 0;

   CFE_PSP_MemSet((void*)&(PktMgr->Tbl), 0, sizeof(PKTTBL_Tbl));
   for (i=0; i < PKTTBL_MAX_PKT_CNT; i++) {

      PktMgr->Tbl.Pkt[i].StreamId = PKTTBL_UNUSED_MSG_ID;

   } /* end pkt loop */

   PktMgr->syncPattern[0] = 0xde;
   PktMgr->syncPattern[1] = 0xad;
   PktMgr->syncPattern[2] = 0xbe;
   PktMgr->syncPattern[3] = 0xef;

   CFE_SB_CreatePipe(&(PktMgr->TlmPipe), PipeDepth, PipeName);

   OS_TaskInstallDeleteHandler((void *)(&DestructorCallback)); /* Call when application terminates */

   /*Initialize a RS422 Port */
   strncpy((char *) &PktMgr->config.device, CI_CONFIG_SERIAL_PORT, PORT_NAME_SIZE);
   PktMgr->config.baudRate = CI_CONFIG_HIGH_BAUD_RATE;
   PktMgr->config.timeout  = CI_CONFIG_TIMEOUT;
   PktMgr->config.minBytes = CI_CONFIG_MINBYTES;
   
   PktMgr->RxWake.sw_pfn = Rx_Wake;
   PktMgr->RxWake.sw_arg = NULL;

   // PktMgr->TxWake.sw_pfn = Tx_Wake;
   // PktMgr->TxWake.sw_arg = NULL;

   QUEUE_Setup(&PktMgr->RecvQueue, PktMgr->RecvBuff, sizeof(PktMgr->RecvBuff));

   PktMgr->CmdLen = 0;
   PktMgr->CmdMsgSize = 0;
   PktMgr->CmdMsgId = 0;
   PktMgr->CmdRecvTimeoutCnt = 0;
   PktMgr->LastRecvCmdLen = 0;

   QUEUE_Setup(&PktMgr->SendQueue, PktMgr->SendBuff, sizeof(PktMgr->SendBuff));

   PktMgr->ChildSemaphore = PKTMGR_CHILD_SEM_INVALID;
   Result = OS_CountSemCreate(&PktMgr->ChildSemaphore, PKTMGR_CHILD_SEM_NAME, 0, 0);
   if (Result != OS_SUCCESS)
   {
      CFE_EVS_SendEvent (PKTMGR_TLM_OUTPUT_ENA_INFO_EID,
         CFE_EVS_ERROR,
         "Failed to create Child Semaphore");
   }

   PKTMGR_ConsoleConnect();


} /* End PKTMGR_Constructor() */

/******************************************************************************
** Function: PKTMGR_ChildInit
*/
void PKTMGR_ChildInit(void)
{
   int32 Result;

   Result = CFE_ES_CreateChildTask(&PktMgr->ChildTaskID,
                                    PKTMGR_CHILD_TASK_NAME,
                                    PKTMGR_ChildTask,
                                    0,
                                    PKTMGR_CHILD_TASK_STACK_SIZE,
                                    PKTMGR_CHILD_TASK_PRIORITY, 
                                    0);   
   if (Result != CFE_SUCCESS) 
   {
      CFE_EVS_SendEvent(PKTMGR_CHILD_INIT_ERR_EID, CFE_EVS_ERROR, 
               "Child Task Initialization error : could not create task: %d", (int)Result);
   }
}

/******************************************************************************
** Function: PKTMGR_ChildTask
*/
void PKTMGR_ChildTask(void)
{
   int32 Result;
   Result = CFE_ES_RegisterChildTask();
   if (Result != CFE_SUCCESS)
   {
      CFE_EVS_SendEvent(PKTMGR_CHILD_INIT_ERR_EID, CFE_EVS_ERROR, 
               "Child Task Initialization error : could not register task: %d", (int)Result);
   }
   else 
   {
      CFE_EVS_SendEvent(PKTMGR_CHILD_INIT_ERR_EID, CFE_EVS_INFORMATION, 
               "Child Task Initialization : initialized. Starting");
      
      // Does not return until time to stop
      PKTMGR_ChildLoop();
   }
   // Exit
   CFE_ES_ExitChildTask();
}

/******************************************************************************
** Function: PKTMGR_ChildLoop
*/
void PKTMGR_ChildLoop(void)
{
   int32 Result = CFE_SUCCESS;
   uint32 count = 0, size = 0;
   uint8 b = 0; 
   while (Result == CFE_SUCCESS)
   {
      Result = OS_CountSemTake(PktMgr->ChildSemaphore);

      CFE_ES_PerfLogEntry(PKTMGR_CHILD_TASK_PERF_ID);
      
      if (Result == CFE_SUCCESS)
      {
         // Process Data
         while (true) {
            size = QUEUE_Pop(&PktMgr->SendQueue, &b, 1);
            if (size == 0) { 
               break;
            }

            // Send just one byte
            for (count = 0; count < PKTMGR_CHILD_TIMEOUT_MAX; count++) {
               size = IO_TransRS422Write(PktMgr->fd, &b, 1);
               if (size == 1) {
                  break;
               }
               
               // Take a break, check back in a bit
               CFE_ES_PerfLogExit(PKTMGR_CHILD_TASK_PERF_ID);
               OS_TaskDelay(PKTMGR_CHILD_TASK_DELAY);
               CFE_ES_PerfLogEntry(PKTMGR_CHILD_TASK_PERF_ID);
            }

            if (count == PKTMGR_CHILD_TIMEOUT_MAX) {
               CFE_EVS_SendEvent(PKTMGR_CHILD_INIT_ERR_EID, CFE_EVS_ERROR, 
               "Child Task: Too many Telemetry timeouts : write error : %d", (int)size);
               
               PktMgr->SuppressSend = TRUE;
               CFE_ES_PerfLogExit(PKTMGR_CHILD_TASK_PERF_ID);
               return; 
            }
         }
      }


      CFE_ES_PerfLogExit(PKTMGR_CHILD_TASK_PERF_ID);
   }
}

/******************************************************************************
** Function: Rx_Wake
** Blindly reads one byte to RxTailPtr. 
** The command reader must read fast enough to keep up or this will overwrite it.
*/
void Rx_Wake(struct termios *tty,void*arg)
{
   int size;
   uint8 b = 0;
   // Preflight, we shouldn't be here...
   if (PktMgr->fd < 0)
   {
      CFE_EVS_SendEvent(PKTMGR_CMD_INGEST_ERR_EID, CFE_EVS_ERROR, 
                        "UART_TO_CI: Serial Port not set. Check init. "
                        "Quitting PKTMGR_IngestCommands.");
      return;
   }
   
   // Try to read a byte, really shouldn't fail... 
   size = IO_TransRS422Read(PktMgr->fd, &b, 1);
   if (size <= 0)
   {
      return;
   }

   // OK Send it
   QUEUE_Push(&PktMgr->RecvQueue, &b, size);
}

/******************************************************************************
** Function: Tx_Wake
** Send Bytes one at a time
*/
/*
void Tx_Wake(struct termios *tty,void*arg)
{
   int size = 0;
   uint8 b = 0;
   // Preflight, we shouldn't be here...
   if (PktMgr->fd < 0)
   {
      CFE_EVS_SendEvent(PKTMGR_CMD_INGEST_ERR_EID, CFE_EVS_ERROR, 
                        "UART_TO_CI: Serial Port not set. Check init. "
                        "Quitting PKTMGR_IngestCommands.");
      OS_CountSemGive(PktMgr->TxSemaphore);
      PktMgr->SuppressSend = TRUE;
      return;
   }

   size = QUEUE_Pop(&PktMgr->SendQueue, &b, 1);
   if (size == 0) 
   {
      // Nothing to send, this marks end of send loop, let the next one go 
      OS_CountSemGive(PktMgr->TxSemaphore);
      return;
   }

   size = IO_TransRS422Write(PktMgr->fd, &b, 1);
   if (size <= 0) 
   {
      CFE_EVS_SendEvent(PKTMGR_CMD_INGEST_ERR_EID, CFE_EVS_ERROR, "UART_TO_CI: Serial Byte write failed");
      OS_CountSemGive(PktMgr->TxSemaphore);
      PktMgr->SuppressSend = TRUE;
   }
}
*/ 

/******************************************************************************
** Function: PKTMGR_GetTblPtr
**
*/
const PKTTBL_Tbl* PKTMGR_GetTblPtr()
{

   return &(PktMgr->Tbl);

} /* End PKTMGR_GetTblPtr() */


/******************************************************************************
** Function:  PKTMGR_ResetStatus
**
*/
void PKTMGR_ResetStatus(void)
{

   /* Nothing to do */

} /* End PKTMGR_ResetStatus() */

/******************************************************************************
** Function: PKTMGR_ConsoleDisconnect
**
** Closes the command and telemetry console
**
*/
void PKTMGR_ConsoleDisconnect(void)
{
   if (PktMgr->connected) {
      IO_TransRS422Close(PktMgr->fd);
      CFE_EVS_SendEvent (PKTMGR_TLM_OUTPUT_ENA_INFO_EID, CFE_EVS_INFORMATION,
            "Disconnected from command and telemetry console on %s",
            PktMgr->config.device);
      PktMgr->fd = -1;
      PktMgr->connected = FALSE;
      PktMgr->DownlinkOn = FALSE;
      
      // Let it shut down... 
      OS_CountSemGive(PktMgr->ChildSemaphore);
      PktMgr->CmdLen = 0;
      PktMgr->CmdMsgSize = 0;
      PktMgr->CmdMsgId = 0;
      PktMgr->ChildSemaphore = 0xFFFFFFFF;

   } else {
      CFE_EVS_SendEvent (PKTMGR_TLM_OUTPUT_ENA_INFO_EID, CFE_EVS_INFORMATION,
            "Attempted to disconnect from disconnected command and telemetry port");
   }
} /* PKTMGR_ConsoleDisconnect() */

/******************************************************************************
** Function: PKTMGR_ConsoleConnect
**
**  Open the command and telemetry console
**
*/
void PKTMGR_ConsoleConnect(void) 
{
   PktMgr->fd = IO_TransRS422Init(&PktMgr->config);

   if (PktMgr->fd < 0)
   {
      CFE_EVS_SendEvent (PKTMGR_TLM_OUTPUT_ENA_INFO_EID,
               CFE_EVS_ERROR,
               "Failed to connect to command and telemetry radio on %s",
               PktMgr->config.device);
      return; 
   } 
  
   // Connected successfully
   PktMgr->connected = TRUE;

   CFE_EVS_SendEvent (PKTMGR_TLM_OUTPUT_ENA_INFO_EID,
            CFE_EVS_INFORMATION,
            "Connected succesfully to command and telemetry radio on %s",
            PktMgr->config.device);

   // Connect the RX Callback
   if ( ioctl( PktMgr->fd, RTEMS_IO_RCVWAKEUP, &PktMgr->RxWake ) < 0 )
   {
      CFE_EVS_SendEvent (PKTMGR_TLM_OUTPUT_ENA_INFO_EID,
               CFE_EVS_ERROR,
               "Failed to register for RX wakeup callback on %s",
               PktMgr->config.device);
      // Failed, need to disconnect...
      PKTMGR_ConsoleDisconnect();
      return; 
   }

   PKTMGR_ChildInit();

   /* Tx Callback seems unreliable
   // Connect the TX Callback
   if ( ioctl( PktMgr->fd, RTEMS_IO_SNDWAKEUP, &PktMgr->TxWake ) < 0 )
   {
      CFE_EVS_SendEvent (PKTMGR_TLM_OUTPUT_ENA_INFO_EID,
               CFE_EVS_ERROR,
               "Failed to register for TX wakeup callback on %s",
               PktMgr->config.device);
      // Failed, need to disconnect...
      PKTMGR_ConsoleDisconnect();
      return; 
   }
   */
}

/******************************************************************************
** Function: alignSyncWord
**    Find the sync word, and move it to align with the start of the buffer
**    Returns the new length (zero indicates it could not be found)
**    WARNING: Don't call this until a full header could be available
*/
uint32 alignSyncWord(uint8 *buff, uint32 start, uint32 end)
{
   uint32 ii = 0, jj = 0;

   for (ii = start; ii < end - 4; ii++ )
   {
      // Find 'ii' that points to the first byte of the sync word 
      if (  buff[ii+0] == PktMgr->syncPattern[0]
         && buff[ii+1] == PktMgr->syncPattern[1]
         && buff[ii+2] == PktMgr->syncPattern[2]
         && buff[ii+3] == PktMgr->syncPattern[3])
      {
         // Correcting Misalignment
         if (ii != 0)
         {
            CFE_EVS_SendEvent (PKTMGR_CMD_INGEST_ERR_EID, CFE_EVS_ERROR, 
                     "Command Stream mis-aligned, dropped %d bytes.", ii);
            
            for (jj = 0; jj < end - ii; jj++)
            {
               buff[jj] = buff[jj+ii]; 
            }
         }
         // Return the new length
         return end - ii;
      }
   }
   return 0;
}

/******************************************************************************
** Function: PKTMGR_CheckCmdTimeout
**
*/
void PKTMGR_CheckCmdTimeout(void)
{
   uint32 size = 0;
   // No Partial Command data 
   if (PktMgr->CmdLen == 0) {
      return ;
   }

   // Avoid memory alignment trap exception.
   if ((PktMgr->CmdLen % 2) != 0) {

      memset(PktMgr->RecvBuff,0,sizeof(PktMgr->RecvBuff));
      PktMgr->CmdLen     = 0;
      PktMgr->CmdMsgId   = 0;
      PktMgr->CmdMsgSize = 0;

      PktMgr->LastRecvCmdLen    = 0;
      PktMgr->CmdRecvTimeoutCnt = 0; 
      PktMgr->RecvCmdErrCnt++;
      
      CFE_EVS_SendEvent (PKTMGR_CMD_INGEST_ERR_EID, CFE_EVS_ERROR, 
            "Odd # of bytes while attempting to read command header. Clearing buffer.");
   }
   // We have received a partial command and this is the first iteration
   if ( PktMgr->CmdRecvTimeoutCnt == 0 ) {
      // Cache the current length, mark first failure 
      PktMgr->LastRecvCmdLen= PktMgr->CmdLen; 
      PktMgr->CmdRecvTimeoutCnt = 1; 
   }
   // We are waiting for data, check for progress
   else {
      // We have a partial command, did we make progress?
      if (PktMgr->LastRecvCmdLen != PktMgr->CmdLen) {
         // We made progress, reset count
         PktMgr->LastRecvCmdLen = PktMgr->CmdLen;
         PktMgr->CmdRecvTimeoutCnt = 1;
      } else { 
         // Mark the lack of progress
         PktMgr->CmdRecvTimeoutCnt += 1;
      }
   }

   // If we've executed 4 run loops of uart_to_ci and have a partial command in the recv buffer, 
   // clear the buffer. Either the command link was interrupted, or the msg header was corrupted
   if (PktMgr->CmdRecvTimeoutCnt >= 4) 
   {
      // Try to find a new command embedded in the broken command
      size = alignSyncWord(PktMgr->RecvBuff, 0, PktMgr->CmdLen);  
      if (size > 0) 
      {
         // Found a new command? 
         PktMgr->CmdLen     = size;
         PktMgr->CmdMsgId   = 0;
         PktMgr->CmdMsgSize = 0;
         CFE_EVS_SendEvent (PKTMGR_CMD_INGEST_ERR_EID, CFE_EVS_ERROR, 
               "Partial command dropped. Found start of new message" );
         return;
      } 

      memset(PktMgr->RecvBuff,0,sizeof(PktMgr->RecvBuff));
      PktMgr->CmdLen     = 0;
      PktMgr->CmdMsgId   = 0;
      PktMgr->CmdMsgSize = 0;

      PktMgr->LastRecvCmdLen    = 0;
      PktMgr->CmdRecvTimeoutCnt = 0; 
      PktMgr->RecvCmdErrCnt++;
      
      CFE_EVS_SendEvent (PKTMGR_CMD_INGEST_ERR_EID, CFE_EVS_ERROR, 
            "Partial command in recv buffer after 4 cycles. Clearing buffer.");
   }
}

/******************************************************************************
** Function: PKTMGR_OutputTelemetry
**
*/
void PKTMGR_OutputTelemetry(void)
{

   int32                     SbStatus;
   uint16                    PktLen;
   CFE_SB_Msg_t              *PktPtr;

   /*
   ** CFE_SB_RcvMsg returns CFE_SUCCESS when it gets a packet, otherwise
   ** no packet was received
   */
   do
   {
      SbStatus = CFE_SB_RcvMsg(&PktPtr, PktMgr->TlmPipe, CFE_SB_POLL);

      if ( (SbStatus == CFE_SUCCESS) && (PktMgr->SuppressSend == FALSE) ) {
          
         PktLen = CFE_SB_GetTotalMsgLength(PktPtr);
         if(PktMgr->DownlinkOn) {

            PKTMGR_SendTelemetry(PktPtr, PktLen);
            
         } /* End if downlink enabled */

      } /* End if SB received msg and output enabled */

   } while(SbStatus == CFE_SUCCESS);

} /* End of PKTMGR_OutputTelemetry() */

/******************************************************************************
** Function: PKTMGR_SendTelemetry
** Write telemetry packets to telemetry console (UART1)
**
*/
void PKTMGR_SendTelemetry(CFE_SB_Msg_t* PktPtr, uint16 PktLen)
{
   uint8 b = 0;
   if (PktMgr->connected) {
      // We start out able to grab this, we won't be able to get it again until TxWake is done
      /**
      CFE_ES_PerfLogExit(UART_TO_CI_MAIN_PERF_ID);
      status = OS_CountSemTake(PktMgr->TxSemaphore);
      CFE_ES_PerfLogEntry(UART_TO_CI_MAIN_PERF_ID);
      
      if (status != OS_SUCCESS)
      {
         CFE_EVS_SendEvent (PKTMGR_TLM_OUTPUT_ENA_INFO_EID,
            CFE_EVS_ERROR,
            "Failed to get Tx Semaphore, result = %d", 
            status);
         return; 
      }
      */
      //Write the synch header
      QUEUE_Push(&PktMgr->SendQueue, PktMgr->syncPattern, sizeof(PktMgr->syncPattern));
      QUEUE_Push(&PktMgr->SendQueue, (uint8 *)PktPtr, PktLen); 

      PktMgr->SentTlmCnt++;

      // Kick off the child task to send stuff
      OS_CountSemGive(PktMgr->ChildSemaphore);
   } 
   else {
      CFE_EVS_SendEvent (PKTMGR_CONSOLE_SEND_ERR_EID, CFE_EVS_ERROR, "Error sending packet on %s. UART not connected. Tlm output suppressed", PktMgr->config.device);
      PktMgr->SuppressSend = TRUE;
   }
} /* PKTMGR_SendTelemetry */


/******************************************************************************
** Function: PKTMGR_IngestCommands
**
** Read a command message into the Rx buffer based on the rx wake signal
**
*/
void PKTMGR_IngestCommands() 
{
   int size = 0;
   uint8 headerSize = 6 + sizeof(PktMgr->syncPattern);
   CFE_SB_MsgPtr_t pSbMsg; 
   // Try reading all available commands
   while (1) 
   {
      PKTMGR_CheckCmdTimeout();

      // Try to read the necessary bytes for a full header
      GETHEADER: if (PktMgr->CmdLen < headerSize) {

         size = QUEUE_Pop( &PktMgr->RecvQueue, &PktMgr->CmdBuff[ PktMgr->CmdLen ], headerSize - PktMgr->CmdLen );
         PktMgr->CmdLen += size;

         // Did we get enough to keep going?
         if (PktMgr->CmdLen < headerSize) {
            // Nothing left to read
            return;
         }
      }

      // Validate Header
      if (PktMgr->CmdLen == headerSize && PktMgr->CmdMsgSize == 0)
      {
         // Check Sync Word and align the start of the buffer
         size = alignSyncWord(PktMgr->CmdBuff, 0, PktMgr->CmdLen);
         if (size == 0) {
            // memset(PktMgr->CmdBuff, 0, sizeof(PktMgr->CmdBuff));
            PktMgr->CmdLen = 0;
            PktMgr->CmdMsgId = 0;
            PktMgr->CmdMsgSize = 0;
            // If we don't find the sync pattern let's see if there's anything
            // left in the RecvQueue, if not, we'll return above
            goto GETHEADER;  
         }

         pSbMsg = (CFE_SB_MsgPtr_t)&PktMgr->CmdBuff[sizeof(PktMgr->syncPattern)];

         // Get message id, size
         PktMgr->CmdMsgId   = CFE_SB_GetMsgId(pSbMsg);
         PktMgr->CmdMsgSize = CFE_SB_GetTotalMsgLength(pSbMsg) + sizeof(PktMgr->syncPattern);

         if (PktMgr->CmdMsgSize > CI_CUSTOM_BUFFER_SIZE) {
            CFE_EVS_SendEvent(PKTMGR_CMD_INGEST_ERR_EID, CFE_EVS_ERROR,
                              "UART_TO_CI: Message received larger than buffer. "
                              "Message ID:0x%x dropped.", PktMgr->CmdMsgId);
            PktMgr->RecvCmdErrCnt++;
          
            // memset(PktMgr->CmdBuff, 0, sizeof(PktMgr->CmdBuff));
            PktMgr->CmdLen = 0;
            PktMgr->CmdMsgId = 0;
            PktMgr->CmdMsgSize = 0;
            continue; 
         }
      }

      // Try to get the rest of the message
      if (PktMgr->CmdLen < PktMgr->CmdMsgSize) {

         size = QUEUE_Pop( &PktMgr->RecvQueue, &PktMgr->CmdBuff[ PktMgr->CmdLen ], PktMgr->CmdMsgSize - PktMgr->CmdLen );
         PktMgr->CmdLen += size;

         // Did we get enough to process a command?
         if (PktMgr->CmdLen < PktMgr->CmdMsgSize) {
            // No, guees we're done for now
            return;
         }
      }

      // This check is for the case where the command size > buffer and it continues
      if ((PktMgr->CmdLen == PktMgr->CmdMsgSize) && (PktMgr->CmdMsgSize != 0)) {
      /*
      ** We have a full message! Let's validate and send it.
      */
      /* CCSDS command checksum check. */
      
         pSbMsg = (CFE_SB_MsgPtr_t)&PktMgr->CmdBuff[sizeof(PktMgr->syncPattern)];

         if (CFE_SB_ValidateChecksum(pSbMsg) == FALSE)
         {
            uint16 cmdCode = CFE_SB_GetCmdCode(pSbMsg);
            CFE_EVS_SendEvent(UART_TO_CI_BASE_EID, CFE_EVS_ERROR,
                              "CI: MID:0x%04x - Cmd Checksum failed. CmdCode:%u",
                              PktMgr->CmdMsgId, cmdCode);
            PktMgr->CmdLen = 0;
            PktMgr->CmdMsgId = 0;
            PktMgr->CmdMsgSize = 0;
            PktMgr->RecvCmdErrCnt++;
            return;
         }
         
         // Send it
         // MsgId check is there to catch an extra case we saw with a corrupted command definition
         if (PktMgr->CmdMsgId != 0) {
            CFE_SB_SendMsg(pSbMsg);
            // Cleanup for next command
            // memset(PktMgr->CmdBuff, 0, sizeof(PktMgr->CmdLen));
            PktMgr->CmdLen = 0;
            PktMgr->CmdMsgId = 0;
            PktMgr->CmdMsgSize = 0;
            PktMgr->RecvCmdCnt++;
         } else {
            CFE_EVS_SendEvent(UART_TO_CI_BASE_EID, CFE_EVS_ERROR,
                              "CI: MID:0x%04x - Received Invalid MsgId",
                              PktMgr->CmdMsgId);
            PktMgr->CmdLen = 0;
            PktMgr->CmdMsgId = 0;
            PktMgr->CmdMsgSize = 0;
            PktMgr->RecvCmdErrCnt++;
            return;
         }

      }
   }
}

/******************************************************************************
** Function: PKTMGR_LoadTbl
**
*/
boolean PKTMGR_LoadTbl(PKTTBL_Tbl* NewTbl)
{

   int      i, PktCnt = 0, FailedSubscription = 0;
   int32    Status;
   boolean  RetStatus = TRUE;

   CFE_SB_MsgPtr_t MsgPtr = NULL;

   PKTMGR_RemoveAllPktsCmd(NULL, MsgPtr);  /* Parameter is unused so OK to be NULL */

   CFE_PSP_MemCpy(&(PktMgr->Tbl), NewTbl, sizeof(PKTTBL_Tbl));

   for (i=0; i < PKTTBL_MAX_PKT_CNT; i++) {

      if(PktMgr->Tbl.Pkt[i].StreamId != PKTTBL_UNUSED_MSG_ID) {
         
         PktCnt++;
         Status = CFE_SB_SubscribeEx(PktMgr->Tbl.Pkt[i].StreamId,
                                     PktMgr->TlmPipe,PktMgr->Tbl.Pkt[i].Qos,
                                     PktMgr->Tbl.Pkt[i].BufLim);

         if(Status != CFE_SUCCESS) {
            
            FailedSubscription++;
            CFE_EVS_SendEvent(PKTMGR_LOAD_TBL_SUBSCRIBE_ERR_EID,CFE_EVS_ERROR,
                              "Error subscribing to stream 0x%4X, BufLim %d, Status %i",
                              PktMgr->Tbl.Pkt[i].StreamId, PktMgr->Tbl.Pkt[i].BufLim, Status);
         }
      }

   } /* End pkt loop */

   if (FailedSubscription == 0) {
      
      CFE_EVS_SendEvent(PKTMGR_LOAD_TBL_INFO_EID, CFE_EVS_INFORMATION,
                        "Loaded new table with %d packets", PktCnt);
   }
   else {
      
      RetStatus = FALSE;
      CFE_EVS_SendEvent(PKTMGR_LOAD_TBL_ERR_EID, CFE_EVS_INFORMATION,
                        "Attempted to load new table with %d packets. Failed %d subscriptions",
                        PktCnt, FailedSubscription);
   }

   return RetStatus;

} /* End PKTMGR_LoadTbl() */


/******************************************************************************
** Function: PKTMGR_LoadTblEntry
**
*/
boolean PKTMGR_LoadTblEntry(uint16 PktIdx, PKTTBL_Pkt* PktArray)
{

   int32    Status;
   boolean  RetStatus = TRUE;
   PKTTBL_Pkt* NewPkt = &(PktMgr->Tbl.Pkt[PktIdx]); 

   CFE_PSP_MemCpy(NewPkt,&PktArray[PktIdx],sizeof(PKTTBL_Pkt));

   Status = SubscribeNewPkt(NewPkt);

   if(Status == CFE_SUCCESS) {
      
      CFE_EVS_SendEvent(PKTMGR_LOAD_TBL_ENTRY_INFO_EID, CFE_EVS_INFORMATION,
                        "Loaded table entry %d with stream 0x%4X, BufLim %d",
                        PktIdx, NewPkt->StreamId, NewPkt->BufLim);
   }
   else {
      
      RetStatus = FALSE;
      CFE_EVS_SendEvent(PKTMGR_LOAD_TBL_ENTRY_SUBSCRIBE_ERR_EID,CFE_EVS_ERROR,
                        "Error subscribing to stream 0x%4X, BufLim %d, Status %i",
                        NewPkt->StreamId, NewPkt->BufLim, Status);
   }

   return RetStatus;

} /* End PKTMGR_LoadTblEntry() */


/******************************************************************************
** Function: PKTMGR_EnableOutputCmd
**
*/
boolean PKTMGR_EnableTlmOutputCmd(void* ObjDataPtr, const CFE_SB_MsgPtr_t MsgPtr)
{
   boolean  RetStatus = TRUE;

   if (PktMgr->fd <= 0) { //Not connected, let's try to connect
      /*Initialize a RS422 Port */
      strncpy((char *) &PktMgr->config.device, CI_CONFIG_SERIAL_PORT, 
               PORT_NAME_SIZE);
      PktMgr->config.baudRate = CI_CONFIG_HIGH_BAUD_RATE;
      PktMgr->config.timeout  = CI_CONFIG_TIMEOUT;
      PktMgr->config.minBytes = CI_CONFIG_MINBYTES;

      PKTMGR_ConsoleConnect();
   }

   if(PktMgr->connected) {
      PktMgr->DownlinkOn = TRUE;
      PktMgr->SuppressSend = FALSE; 
      CFE_EVS_SendEvent(PKTMGR_TLM_OUTPUT_ENA_INFO_EID, CFE_EVS_INFORMATION,
                     "Telemetry output enabled for %s", PktMgr->config.device);
   } else {
      RetStatus = FALSE;
   }

   return RetStatus;

} /* End PKTMGR_EnableOutputCmd() */

/******************************************************************************
** Function: PKTMGR_DisableOutputCmd
**
** Disable telemetry output 
**
*/
boolean PKTMGR_DisableTlmOutputCmd(void* ObjDataPtr, const CFE_SB_MsgPtr_t MsgPtr) {
   PktMgr->DownlinkOn = FALSE;
   PktMgr->SuppressSend = TRUE; 
   CFE_EVS_SendEvent(PKTMGR_TLM_OUTPUT_ENA_INFO_EID, CFE_EVS_INFORMATION,
                  "Telemetry output disabled for %s", PktMgr->config.device);
   
   return true;

}

/******************************************************************************
** Function: PKTMGR_SetRateCmd
**
** Set the command and telemetry rate to either high (19200) or low (9600)
**
*/
boolean PKTMGR_SetRateCmd(void* ObjDataPtr, const CFE_SB_MsgPtr_t MsgPtr) {

   boolean DownlinkStatus;

   DownlinkStatus = PktMgr->DownlinkOn; //Save the downlink status so we re-enable if necessary

   const  PKTMGR_SetRateCmdParam *CmdParam = (const PKTMGR_SetRateCmdParam *) MsgPtr;

   if (CmdParam->Rate == 0) { //low rate
      PktMgr->config.baudRate = CI_CONFIG_LOW_BAUD_RATE;
      CFE_EVS_SendEvent (PKTMGR_TLM_OUTPUT_ENA_INFO_EID,
                        CFE_EVS_INFORMATION,
                        "Configuring %s for low rate",
                        PktMgr->config.device);

   } else if (CmdParam->Rate == 1) { //high rate
      PktMgr->config.baudRate = CI_CONFIG_HIGH_BAUD_RATE;
      CFE_EVS_SendEvent (PKTMGR_TLM_OUTPUT_ENA_INFO_EID,
                        CFE_EVS_INFORMATION,
                        "Configuring %s for high rate",
                        PktMgr->config.device);

   } else {
      CFE_EVS_SendEvent(UART_TO_CI_BASE_EID, CFE_EVS_ERROR,
                        "Attempted to set invalid cmd/tlm rate");
      return false;

   }
   PKTMGR_ConsoleDisconnect();
   PKTMGR_ConsoleConnect();

   PktMgr->DownlinkOn = DownlinkStatus;

   return true;

}

/******************************************************************************
** Function: PKTMGR_AddPktCmd
**
*/
boolean PKTMGR_AddPktCmd(void* ObjDataPtr, const CFE_SB_MsgPtr_t MsgPtr)
{

   const  PKTMGR_AddPktCmdParam *CmdParam = (const PKTMGR_AddPktCmdParam *) MsgPtr;
   int         PktIdx;
   PKTTBL_Pkt  NewPkt;
   boolean     RetStatus = TRUE;
   int32       Status;

   if ( (PktIdx = StreamIdPktIdx(CmdParam->StreamId)) < 0) {

      if ( (PktIdx = UnusedTblPktIdx()) >= 0 ) {

         NewPkt.StreamId = CmdParam->StreamId;
         NewPkt.Qos      = CmdParam->Qos;
         NewPkt.BufLim   = CmdParam->BufLim;
         
         Status = SubscribeNewPkt(&NewPkt);
         if (Status == CFE_SUCCESS) {
            CFE_EVS_SendEvent(PKTMGR_ADD_PKT_INFO_EID, CFE_EVS_INFORMATION,
                              "Added packet 0x%04X, QoS (%d,%d), BufLim %d",
                              NewPkt.StreamId, NewPkt.Qos.Priority, NewPkt.Qos.Reliability, NewPkt.BufLim);
         }
         else {
            CFE_EVS_SendEvent(PKTMGR_ADD_PKT_SUBSCRIPTION_ERR_EID, CFE_EVS_ERROR,
                              "Error aadin packet 0x%04X. Software Bus subscription failed. Return status = %i",
                              CmdParam->StreamId, Status);
         }

      } /* end if found unused entry */
      else
      {

         RetStatus = FALSE;
         CFE_EVS_SendEvent(PKTMGR_ADD_PKT_NO_FREE_ENTRY_ERR_EID, CFE_EVS_ERROR,
                           "Error adding packet 0x%04X, packet table is full. Unused StreamId = 0x%04X",
                           CmdParam->StreamId,PKTTBL_UNUSED_MSG_ID);

      }

   } /* End if didn't find existing entry */
   else
   {
      CFE_EVS_SendEvent(PKTMGR_ADD_PKT_DUPLICATE_ENTRY_EID, CFE_EVS_ERROR,
                        "Error adding packet, stream id 0x%4X exists at entry %d",
                        CmdParam->StreamId, PktIdx);

   } /* End if found existing entry */

   return RetStatus;

} /* End of PKTMGR_AddPktCmd() */

/*******************************************************************
** Function: PKTMGR_RemovePktCmd
**
*/
boolean PKTMGR_RemovePktCmd(void* ObjDataPtr, const CFE_SB_MsgPtr_t MsgPtr)
{

   const PKTMGR_RemovePktCmdParam *RemovePktCmd = (const PKTMGR_RemovePktCmdParam *) MsgPtr;
   int      PktIdx = -1;
   int32    Status;
   boolean  RetStatus = TRUE;

   if ( (PktIdx = StreamIdPktIdx(RemovePktCmd->StreamId)) >= 0)
   {

      CFE_PSP_MemCpy(&(PktMgr->Tbl.Pkt[PktIdx]), &UnusedTblPkt, sizeof(PKTTBL_Pkt));
      Status = CFE_SB_Unsubscribe(RemovePktCmd->StreamId, PktMgr->TlmPipe);

      if(Status == CFE_SUCCESS)
      {
         CFE_EVS_SendEvent(PKTMGR_REMOVE_PKT_INFO_EID, CFE_EVS_INFORMATION,
                           "Removed stream id 0x%4X at table packet index %d",
                           RemovePktCmd->StreamId, PktIdx);
      }
      else
      {
         RetStatus = FALSE;
         CFE_EVS_SendEvent(PKTMGR_REMOVE_PKT_SB_ERR_EID, CFE_EVS_ERROR,
                           "Error removing stream id 0x%4X at table packet index %d. Unsubscribe status 0x%8x",
                           RemovePktCmd->StreamId, PktIdx, Status);
      }

   } /* End if found stream ID in table */
   else
   {

      CFE_EVS_SendEvent(PKTMGR_REMOVE_PKT_NOT_FOUND_ERR_EID, CFE_EVS_ERROR,
                        "Error removing stream id 0x%4X. Packet not found",
                        RemovePktCmd->StreamId);

   } /* End if didn't find stream ID in table */

   return RetStatus;

} /* End of PKTMGR_RemovePktCmd() */


/******************************************************************************
** Function: PKTMGR_RemoveAllPktsCmd
**
** Notes:
**   1. The cFE to_lab code unsubscribes the command and send HK MIDs. I'm not
**      sure why this is done and I'm not sure how the command is used. This 
**      command is intended to help manage TO telemetry packets.
*/
boolean PKTMGR_RemoveAllPktsCmd(void* ObjDataPtr, const CFE_SB_MsgPtr_t MsgPtr)
{

   int      i, PktCnt = 0, FailedUnsubscribe = 0;
   int32    Status;
   boolean  RetStatus = TRUE;

   for (i=0; i < PKTTBL_MAX_PKT_CNT; i++)
   {
       if (PktMgr->Tbl.Pkt[i].StreamId != PKTTBL_UNUSED_MSG_ID )
       {

          PktCnt++;
          CFE_PSP_MemCpy(&(PktMgr->Tbl.Pkt[i]), &UnusedTblPkt, sizeof(PKTTBL_Pkt));
          Status = CFE_SB_Unsubscribe(PktMgr->Tbl.Pkt[i].StreamId, PktMgr->TlmPipe);

          if(Status != CFE_SUCCESS)
          {
             FailedUnsubscribe++;
             CFE_EVS_SendEvent(PKTMGR_REMOVE_ALL_PKTS_UNSUBSCRIBE_ERR_EID,CFE_EVS_ERROR,
                               "Error removing stream id 0x%4X at table packet index %d. Unsubscribe status 0x%8X",
                               PktMgr->Tbl.Pkt[i].StreamId, i, Status);
          }

       } /* End if packet in use */

   } /* End pkt loop */

   CFE_EVS_SendEvent(UART_TO_CI_INIT_DEBUG_EID, UART_TO_CI_INIT_EVS_TYPE, "PKTMGR_RemoveAllPktsCmd() - About to flush pipe\n");
   FlushTlmPipe();
   CFE_EVS_SendEvent(UART_TO_CI_INIT_DEBUG_EID, UART_TO_CI_INIT_EVS_TYPE, "PKTMGR_RemoveAllPktsCmd() - Completed pipe flush\n");

   if (FailedUnsubscribe == 0)
   {
      CFE_EVS_SendEvent(PKTMGR_REMOVE_ALL_PKTS_INFO_EID, CFE_EVS_INFORMATION,
                        "Removed %d table packet entries", PktCnt);
   }
   else
   {
      RetStatus = FALSE;
      CFE_EVS_SendEvent(PKTMGR_REMOVE_ALL_PKTS_ERR_EID, CFE_EVS_INFORMATION,
                        "Attempted to remove %d packet entries. Failed %d unsubscribes",
                        PktCnt, FailedUnsubscribe);
   }

   return RetStatus;

} /* End of PKTMGR_RemoveAllPktsCmd() */


/******************************************************************************
** Function: DestructorCallback
**
** This function is called when the app is killed. This should
** never occur but if it does this will close the network socket.
*/
static void DestructorCallback(void)
{

   CFE_EVS_SendEvent(PKTMGR_DESTRUCTOR_INFO_EID, CFE_EVS_INFORMATION, "Destructor callback -- Closing radio console. Downlink on = %d\n", PktMgr->DownlinkOn);
   
   if (PktMgr->DownlinkOn) {
      
      close(PktMgr->fd);
   
   }

} /* End DestructorCallback() */

/******************************************************************************
** Function: FlushTlmPipe
**
** Remove all of the packets from the input pipe.
**
*/
static void FlushTlmPipe(void)
{

   int32 Status;
   CFE_SB_MsgPtr_t MsgPtr = NULL;

   do
   {
      Status = CFE_SB_RcvMsg(&MsgPtr, PktMgr->TlmPipe, CFE_SB_POLL);

   } while(Status == CFE_SUCCESS);

} /* End FlushTlmPipe() */

/******************************************************************************
** Function: StreamIdPktIdx
**
** Look for a StreamId in the packet table. Return the index of the
** packet if it is found and return -1 if it is not found.
**
*/
static int StreamIdPktIdx(uint16 StreamId)
{

   int i;
   int PktIdx = -1;

   for (i=0; i < PKTTBL_MAX_PKT_CNT; i++)
   {

      if (PktMgr->Tbl.Pkt[i].StreamId == StreamId)
      {
         PktIdx = i;
         break;
      }

   } /* end pkt loop */

   return PktIdx;

} /* End StreamIdPktIdx() */


/******************************************************************************
** Function: SubscribeNewPkt
**
*/
static int32 SubscribeNewPkt(PKTTBL_Pkt* NewPkt)
{

   int32    Status;

   Status = CFE_SB_SubscribeEx(NewPkt->StreamId, PktMgr->TlmPipe, NewPkt->Qos, NewPkt->BufLim);

   return Status;

} /* End SubscribeNewPkt(() */

/******************************************************************************
** Function: UnusedTblPktIdx
**
** Look for an unused packet table entry. Return the index of the
** unused table entry if one is found and return -1 if one is not
** found.
**
*/
static int UnusedTblPktIdx(void)
{

   int i;
   int PktIdx = -1;

   for (i=0; i < PKTTBL_MAX_PKT_CNT; i++)
   {

      if (PktMgr->Tbl.Pkt[i].StreamId == PKTTBL_UNUSED_MSG_ID)
      {
         PktIdx = i;
         break;
      }

   } /* end pkt loop */

   return PktIdx;

} /* End UnusedTblPktIdx() */

/* end of file */
