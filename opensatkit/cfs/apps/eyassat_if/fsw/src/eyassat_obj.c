/*
** Purpose: Implement an interface to the Eyassat
**
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "app_cfg.h"
#include "eyassat_obj.h"
#include <bsp.h>

/*
** Global File Data
*/

static EYASSATOBJ_Class*  EyasSatObj = NULL;

/*
** Local Function Prototypes
*/


/******************************************************************************
** Function: EYASSATOBJ_Constructor
**
*/
void EYASSATOBJ_Constructor(EYASSATOBJ_Class*  EyasSatObjPtr)
{
 
   EyasSatObj = EyasSatObjPtr;

   CFE_PSP_MemSet((void*)EyasSatObj, 0, sizeof(EYASSATOBJ_Class));

   EyasSatObj->RxWake.sw_pfn = Rx_Wake;
   EyasSatObj->RxWake.sw_arg = NULL;
   EyasSatObj->RxPtr = 0;

   EyasSatObj->newTelemetry = false;
   EyasSatObj->updateConfig = false;
   EyasSatObj->updateDB = false;
   EyasSatObj->updatePID = false;
   EyasSatObj->updateMagCal = false;
   EyasSatObj->updateGyroCal = false;

   EyasSatObj->CmdBufferFull = false;
   
   EyasSatObj->PowerTlmUpdate = false;
   EyasSatObj->AdcsTlmUpdate = false;

   /*Initialize a RS232 Port */
   EyasSatObj->connected = false;
   strncpy((char *) &EyasSatObj->config.device, EYASSAT_CONFIG_SERIAL_PORT, PORT_NAME_SIZE);
   EyasSatObj->config.baudRate = EYASSAT_CONFIG_BAUD_RATE;
   EyasSatObj->config.timeout  = EYASSAT_CONFIG_TIMEOUT;
   EyasSatObj->config.minBytes = EYASSAT_CONFIG_MINBYTES;

   EYASSATOBJ_ConsoleConnect();

   CFE_SB_InitMsg(&(EyasSatObj->InternalPkt), EYASSAT_IF_TLM_INTERNAL_MID, EYASSATOBJ_INTERNAL_LEN, TRUE);
   CFE_SB_InitMsg(&(EyasSatObj->TempPkt), EYASSAT_IF_TLM_TEMP_MID, EYASSATOBJ_TEMP_LEN, TRUE);
   CFE_SB_InitMsg(&(EyasSatObj->PowerPkt), EYASSAT_IF_TLM_POWER_MID, EYASSATOBJ_POWER_LEN, TRUE);
   CFE_SB_InitMsg(&(EyasSatObj->ADCSPkt), EYASSAT_IF_TLM_ADCS_MID, EYASSATOBJ_ADCS_LEN, TRUE);

   if(EyasSatObj->connected) {
      EYASSATOBJ_InitSat();
   }

} /* End EYASSATOBJ_Constructor() */

/******************************************************************************
** Function: Rx_Wake
**
*/
void Rx_Wake(struct termios *tty,void*arg)
{

   EYASSATOBJ_ProcessTelemetry();

}

/******************************************************************************
** Function: CmdBufferFull
**
** Check to see if the command buffer is full
**
*/
boolean CmdBufferFull(void) {

   return EyasSatObj->CmdBufferFull;

} /* End CmdBufferFull() */

/******************************************************************************
** Function: CmdBufferEmpty
**
** Checks if the commmand buffer is empty
**
*/
boolean CmdBufferEmpty(void) {

   return (!EyasSatObj->CmdBufferFull && (EyasSatObj->CmdBufferHead == EyasSatObj->CmdBufferTail));

} /* End CmdBufferEmpty() */

/******************************************************************************
** Function: CmdBufferFreeSpace
**
** Checks for free space in the command buffer
**
*/
uint8 CmdBufferFreeSpace(void) {
   
   uint8 size = EYASSATOBJ_CMD_BUFFER_DEPTH;

   if (!EyasSatObj->CmdBufferFull) {

      if (EyasSatObj->CmdBufferHead >= EyasSatObj->CmdBufferTail) {

            size = EyasSatObj->CmdBufferHead - EyasSatObj->CmdBufferTail;

      } else {

         size = EYASSATOBJ_CMD_BUFFER_DEPTH + EyasSatObj->CmdBufferHead - EyasSatObj->CmdBufferTail;

      }
   }
   return size;
} /* End CmdBufferEmpty() */

/******************************************************************************
** Function: CmdBufferAdvancePointer
**
** Advance the command buffer when adding a command to the buffer
**
*/
void CmdBufferAdvancePointer(void) {

	if(EyasSatObj->CmdBufferFull){

		EyasSatObj->CmdBufferTail = (EyasSatObj->CmdBufferTail + 1) % EYASSATOBJ_CMD_BUFFER_DEPTH;

	}
	EyasSatObj->CmdBufferHead = (EyasSatObj->CmdBufferHead + 1) % EYASSATOBJ_CMD_BUFFER_DEPTH;
	EyasSatObj->CmdBufferFull = (EyasSatObj->CmdBufferHead == EyasSatObj->CmdBufferTail);

} /* CmdBufferAdvancePointer() */

/******************************************************************************
** Function: CmdBufferRetreatPointer
**
** Retreat the command buffer tail pointer after sending a command
**
*/
void CmdBufferRetreatPointer(void) {

	EyasSatObj->CmdBufferFull = false;
	EyasSatObj->CmdBufferTail = (EyasSatObj->CmdBufferTail + 1) % EYASSATOBJ_CMD_BUFFER_DEPTH;

} /* CmdBufferRetreatPointer() */

/******************************************************************************
** Function: CmdBufferPutCmd
**
** Add a command to the command buffer
**
*/
void CmdBufferPutCmd(char* Cmd) {

   if (EyasSatObj->CmdBufferFull) {
      CFE_EVS_SendEvent (EYASSATOBJ_CMD_INFO_EID, CFE_EVS_ERROR, "Command buffer full. Removing the oldest command");
   }

   memset(EyasSatObj->commandBuffer[EyasSatObj->CmdBufferHead], '\0', EYASSATOBJ_CMD_BUFFER_SIZE);
   CFE_PSP_MemCpy(&(EyasSatObj->commandBuffer[EyasSatObj->CmdBufferHead]), Cmd, EYASSATOBJ_CMD_BUFFER_SIZE);
   CmdBufferAdvancePointer();

} /* CmdBufferPutCmd() */

/******************************************************************************
** Function: EYASSATOBJ_Execute
**
** Execute main object function.
**
*/
void EYASSATOBJ_Execute(void)
{

   EyasSatObj->ExecCnt++;

   // If we have new telemetry from the eyassat process it and generate cFS telemetry messages
   if (EyasSatObj->newTelemetry) {
      EYASSATOBJ_ProcessPacketGroup(EyasSatObj->RxPtr);
      EyasSatObj->newTelemetry = false;
   }

   // If power telemetry is enabled and ADCS power is on, update the ADCS table parameters if needed
   if((EyasSatObj->InternalPkt.PwrTlm == 1) && (((EyasSatObj->PowerPkt.SwitchStatus & 0x02) >> 1) == 1)) {
      if (EyasSatObj->updateConfig) {
         EYASSATOBJ_UpdateConfigParams();
         EyasSatObj->updateConfig = false;

      } else if (EyasSatObj->updateDB) {
         EYASSATOBJ_UpdateDBParams();
         EyasSatObj->updateDB = false;

      } else if (EyasSatObj->updatePID) {
         EYASSATOBJ_UpdatePIDParams();
         EyasSatObj->updatePID = false;

      } else if (EyasSatObj->updateMagCal) {
         EYASSATOBJ_UpdateMagCalParams();
         EyasSatObj->updateMagCal = false;

      } else if (EyasSatObj->updateGyroCal) {
         EYASSATOBJ_UpdateGyroCalParams();
         EyasSatObj->updateGyroCal = false;
      }
   }
   
   // Send commands if we have them in the buffer. Eyassat requires that one command be processed
   // before next command is received.  This requires 3 seconds between commands (at tlm delay of 2s). 
   // The app run loop is 500ms, so we send one command from the buffer every 6 run loops
   if (!CmdBufferEmpty() && ((EyasSatObj->ExecCnt % 6) == 0)){
      EYASSATOBJ_SendCommand();
   }

} /* EYASSATOBJ_Execute() */

/******************************************************************************
** Function: EYASSATOBJ_ConsoleConnect
**
** We'll attempt to connect to /dev/console_b on startup
** If we don't connect succesfully, the execute function will try again
**
*/
void EYASSATOBJ_ConsoleConnect(void)
{
   
   EyasSatObj->fd = IO_TransRS422Init(&EyasSatObj->config);
   
   int sc = ioctl(EyasSatObj->fd, RTEMS_IO_RCVWAKEUP, &EyasSatObj->RxWake);

   if (sc < 0) {
      CFE_EVS_SendEvent (EYASSATOBJ_CONNECT_EID,
               CFE_EVS_ERROR,
               "Failed to register for wakeup callback on %s",
               EyasSatObj->config.device);
   }

   if (EyasSatObj->fd < 0) {
		CFE_EVS_SendEvent (EYASSATOBJ_CONNECT_EID,
                     CFE_EVS_INFORMATION,
                     "Failed to connect to EyaSat on %s\n",
                     EyasSatObj->config.device);
	} else {
      CFE_EVS_SendEvent (EYASSATOBJ_CONNECT_EID,
                  CFE_EVS_INFORMATION,
                  "Successfully connected to EyasSat on %s. Configuring console.",
                  EyasSatObj->config.device);
      EyasSatObj->connected = true;
   }

} /* EYASSATOBJ_ConsoleConnect() */

/******************************************************************************
** Function: EYASSATOBJ_ConsoleDisconnect
**
** Closes EyasSat console file descriptor
**
*/
void EYASSATOBJ_ConsoleDisconnect(void)
{
   if (EyasSatObj->connected) {
      IO_TransRS422Close(EyasSatObj->fd);
      CFE_EVS_SendEvent (EYASSATOBJ_CONNECT_EID,
            CFE_EVS_INFORMATION,
            "Disconnected from EyasSat console on %s",
            EyasSatObj->config.device);
      EyasSatObj->fd = 0;
      EyasSatObj->connected = false;
      EyasSatObj->RxPtr = 0;
      EyasSatObj->newTelemetry = false;
   } else {
      CFE_EVS_SendEvent (EYASSATOBJ_CONNECT_EID, CFE_EVS_INFORMATION,
            "Attempted to disconnect from disconnected EyasSat port");
   }
}

/******************************************************************************
** Function: EYASSATOBJ_ProcessTelemetry
**
** Processing incoming telemetry from the EyasSat C&DH board
** Populate EyasSat Object with EyasSat Telemetry values
**
*/
void EYASSATOBJ_ProcessTelemetry(void)
{
   int bytesRead = 0;

   bytesRead = IO_TransRS422Read(EyasSatObj->fd,(uint8 *)&EyasSatObj->telemetryBuffer[EyasSatObj->RxPtr], 1);
   // OS_printf("bytes read: %d\n",bytesRead);
   if ((EyasSatObj->RxPtr > 3) && (EyasSatObj->telemetryBuffer[EyasSatObj->RxPtr-3] == 45) &&
       (EyasSatObj->telemetryBuffer[EyasSatObj->RxPtr-2] == 45) &&
       (EyasSatObj->telemetryBuffer[EyasSatObj->RxPtr-1] == 45) &&
       (EyasSatObj->telemetryBuffer[EyasSatObj->RxPtr] == 45)) 
   { //end of transmission
   	// printf("Received EyasSat Telemetry - Bytes Read: %d\n", EyasSatObj->RxPtr);
      EyasSatObj->newTelemetry = true;
   }
   EyasSatObj->RxPtr += bytesRead;
   
} /* EYASSATOBJ_ProcessTelemetry() */

/******************************************************************************
** Function: ProcessPacketGroup
** 
** Receives an entire group of EyasSat packets per processing cycle
** Based on the packet id, it calls the appropriate processing function
**
*/
void EYASSATOBJ_ProcessPacketGroup(int totalBytesRead)
{
	char packetType;
	int packetTypeOffset = 13;

	for (int i = 0; i < totalBytesRead; i++) {
		if (strncmp(&EyasSatObj->telemetryBuffer[i],"ES",2)==0) {
			packetType = EyasSatObj->telemetryBuffer[i+packetTypeOffset];
			switch(packetType) {
				case 'I':
					// printf("Detected the start of an Internal packet at index %d\n", i);
               EYASSATOBJ_ProcessInternalPacket(i);
					break;
				case 'T':
               EYASSATOBJ_ProcessTempPacket(i);
					// printf("Detected the start of an Temp packet at index %d\n", i);
					break;
				case 'P':
               EYASSATOBJ_ProcessPowerPacket(i);
					// printf("Detected the start of an Power packet at index %d\n", i);
					break;
				case 'A':
               EYASSATOBJ_ProcessADCSPacket(i);
					// printf("Detected the start of an ADCS packet at index %d\n", i);
					break;
				case '-':
					// printf("Detected the start of an tail sequence packet at index %d\n", i);
					break;
				case 'R':
					// printf("Detected the start of a command response packet at index %d\n", i);
					break;
				default:
               CFE_EVS_SendEvent (EYASSATOBJ_CONNECT_EID, CFE_EVS_DEBUG, "Received unknown packet type on EyasSat IF %d",packetType);
               // OS_printf("%s\n", EyasSatObj->telemetryBuffer);
					break;
			}
		}
	}

   // Received an entire packet set point back and clear buffer
   EyasSatObj->RxPtr = 0;
   memset(EyasSatObj->telemetryBuffer, '\0', sizeof(EyasSatObj->telemetryBuffer));

} /* EYASSATOBJ_ProcessPacketGroup */

/******************************************************************************
** Function: EYASSATOBJ_ProcessInternalPacket
** 
** Processes the EyasSat Internal telemetry packet and populates the appropriate structure
**
*/
void EYASSATOBJ_ProcessInternalPacket(int pktIndex)
{
   int   status;
   char  CallSign[3];
   char  TimeString[8];

   if ((status = sscanf(&EyasSatObj->telemetryBuffer[pktIndex], "%3c %8c I: TelemDelay=%hhu CmdTimeOut=%hu Pwr=%hhu ADCS=%hhu Exp1=%hhu\n\n", 
   						CallSign, TimeString, &EyasSatObj->InternalPkt.TimeDelay, &EyasSatObj->InternalPkt.CmdTimeOut, 
                     &EyasSatObj->InternalPkt.PwrTlm, &EyasSatObj->InternalPkt.AdcsTlm, &EyasSatObj->InternalPkt.ExpTlm)) == 7) {

      EyasSatObj->InternalPkt.Hdr.PacketId = 1;
      CFE_PSP_MemCpy(&(EyasSatObj->InternalPkt.Hdr.CallSign), CallSign, sizeof(EyasSatObj->InternalPkt.Hdr.CallSign));
      CFE_PSP_MemCpy(&(EyasSatObj->InternalPkt.Hdr.TimeString), TimeString, sizeof(EyasSatObj->InternalPkt.Hdr.TimeString));

      CFE_SB_TimeStampMsg((CFE_SB_Msg_t *) &(EyasSatObj->InternalPkt));
      // CFE_SB_SendMsg((CFE_SB_Msg_t *) &(EyasSatObj->InternalPkt));
   
   } else {
      CFE_EVS_SendEvent (EYASSATOBJ_CONNECT_EID, CFE_EVS_CRITICAL, "EyasSat Internal Packet Conversion Error: %d", status);
      // printf("Received EyasSat Internal Telemetry: %s\n", packetBuffer);
   }

} /* EYASSATOBJ_ProcessInternalPacket */

/******************************************************************************
** Function: EYASSATOBJ_ProcessTempPacket
** 
** Processes the EyasSat Temperature telemetry packet and populates the appropriate structure
**
*/
void EYASSATOBJ_ProcessTempPacket(int pktIndex) {

   int   status;
   char  CallSign[3];
   char  TimeString[8];
   
   if ((status = sscanf(&EyasSatObj->telemetryBuffer[pktIndex], "%3c %8c T: DH=%f Exp=%f Ref=%f Panel_A=%f Panel_B=%f Base=%f Top_A=%f Top_B=%f\n\n", 
   						CallSign, TimeString, &EyasSatObj->TempPkt.DHTemp, &EyasSatObj->TempPkt.ExpTemp, &EyasSatObj->TempPkt.RefTemp,
                      &EyasSatObj->TempPkt.PanelATemp, &EyasSatObj->TempPkt.PanelBTemp, &EyasSatObj->TempPkt.BaseTemp,
                       &EyasSatObj->TempPkt.TopATemp, &EyasSatObj->TempPkt.TopBTemp)) == 10) {

      EyasSatObj->TempPkt.Hdr.PacketId = 2;
      CFE_PSP_MemCpy(&(EyasSatObj->TempPkt.Hdr.CallSign), CallSign, sizeof(EyasSatObj->TempPkt.Hdr.CallSign));
      CFE_PSP_MemCpy(&(EyasSatObj->TempPkt.Hdr.TimeString), TimeString, sizeof(EyasSatObj->TempPkt.Hdr.TimeString));

      CFE_SB_TimeStampMsg((CFE_SB_Msg_t *) &(EyasSatObj->TempPkt));
      // CFE_SB_SendMsg((CFE_SB_Msg_t *) &(EyasSatObj->TempPkt));
   
   } else {
      CFE_EVS_SendEvent (EYASSATOBJ_CONNECT_EID, CFE_EVS_CRITICAL, "EyasSat Temperature Packet Conversion Error: %d", status);
      // printf("Received EyasSat Temp Telemetry: %s\n", packetBuffer);
   }
   
   
} /* EYASSATOBJ_ProcessTempPacket */

/******************************************************************************
** Function: EYASSATOBJ_ProcessPowerPacket
** 
** Processes the EyasSat Power telemetry packet and populates the appropriate structure
**
*/
void EYASSATOBJ_ProcessPowerPacket(int pktIndex) {
   
   int   status;
   char  CallSign[3];
   char  TimeString[8];

   if ((status = sscanf(&EyasSatObj->telemetryBuffer[pktIndex], "%3c %8c P: Sep=%hhu, V_Batt=%f, I_Batt=%f, V_SA=%f, I_SA=%f, I_MB=%f,  V_5v=%f, I_5v=%f, V_3v=%f, I_3v=%f, S=%hx,  T_Batt=%f, T_SA1=%f, T_SA2=%f\n\n", 
   						CallSign, TimeString, &EyasSatObj->PowerPkt.SepStatus, &EyasSatObj->PowerPkt.VBatt, &EyasSatObj->PowerPkt.IBatt,
                      &EyasSatObj->PowerPkt.VSA, &EyasSatObj->PowerPkt.ISA, &EyasSatObj->PowerPkt.IMB,
                       &EyasSatObj->PowerPkt.V5V, &EyasSatObj->PowerPkt.I5V, &EyasSatObj->PowerPkt.V3V,
                       &EyasSatObj->PowerPkt.I3V, &EyasSatObj->PowerPkt.SwitchStatus, &EyasSatObj->PowerPkt.BattTemp,
                       &EyasSatObj->PowerPkt.SA1Temp, &EyasSatObj->PowerPkt.SA2Temp)) == 16) {

      EyasSatObj->PowerPkt.Hdr.PacketId = 3;
      CFE_PSP_MemCpy(&(EyasSatObj->PowerPkt.Hdr.CallSign), CallSign, sizeof(EyasSatObj->PowerPkt.Hdr.CallSign));
      CFE_PSP_MemCpy(&(EyasSatObj->PowerPkt.Hdr.TimeString), TimeString, sizeof(EyasSatObj->PowerPkt.Hdr.TimeString));

      CFE_SB_TimeStampMsg((CFE_SB_Msg_t *) &(EyasSatObj->PowerPkt));
      EyasSatObj->PowerTlmUpdate = true;
      // CFE_SB_SendMsg((CFE_SB_Msg_t *) &(EyasSatObj->PowerPkt));
   
   } else {
      CFE_EVS_SendEvent (EYASSATOBJ_CONNECT_EID, CFE_EVS_CRITICAL, "EyasSat Power Packet Conversion Error: %d", status);
      // printf("Received EyasSat Power Telemetry: %s\n", packetBuffer);
   }
   
} /* EYASSATOBJ_ProcessPowerPacket */

/******************************************************************************
** Function: EYASSATOBJ_ProcessADCSPacket
** 
** Processes the EyasSat ADCS telemetry packet and populates the appropriate structure
**
*/
void EYASSATOBJ_ProcessADCSPacket(int pktIndex) {
   
   int   status;
   char  CallSign[3];
   char  TimeString[8];

   if ((status = sscanf(&EyasSatObj->telemetryBuffer[pktIndex], "%3c %8c A: s_T=%hu, s_B=%hu, s_Y=%f, Y=%f, P=%f, R=%f, M_X=%f, M_Y=%f, M_Z=%f, R_X=%f, R_Y=%f, R_Z=%f, A_X=%f, A_Y=%f, A_Z=%f, X=%hhu, Y=%hhu, Vw=%f, Vw_Cmd=%f, Hw=%f, PWM_out=%hu, alg=%hhu, deltaT=%f, Y_Cmd=%f, PE=%f, db=%f, e=%f, Kp=%f, Ki=%f, Kd=%f, db_sf=%f\n\n", 
   						CallSign, TimeString, &EyasSatObj->ADCSPkt.SunTop, &EyasSatObj->ADCSPkt.SunBottom, &EyasSatObj->ADCSPkt.SunYawAng, &EyasSatObj->ADCSPkt.Yaw, &EyasSatObj->ADCSPkt.Pitch,
                     &EyasSatObj->ADCSPkt.Roll, &EyasSatObj->ADCSPkt.MagX, &EyasSatObj->ADCSPkt.MagY, &EyasSatObj->ADCSPkt.MagZ, &EyasSatObj->ADCSPkt.RotX, &EyasSatObj->ADCSPkt.RotY,
                     &EyasSatObj->ADCSPkt.RotZ, &EyasSatObj->ADCSPkt.AccX, &EyasSatObj->ADCSPkt.AccY, &EyasSatObj->ADCSPkt.AccZ, &EyasSatObj->ADCSPkt.XRod,&EyasSatObj->ADCSPkt.YRod, 
                     &EyasSatObj->ADCSPkt.ActWheelSpd, &EyasSatObj->ADCSPkt.CmdWheelSpd, &EyasSatObj->ADCSPkt.WheelAngMom, &EyasSatObj->ADCSPkt.PWM, &EyasSatObj->ADCSPkt.CtrlMode, 
                     &EyasSatObj->ADCSPkt.DeltaT, &EyasSatObj->ADCSPkt.YawCmd, &EyasSatObj->ADCSPkt.PointingError, &EyasSatObj->ADCSPkt.Deadband, &EyasSatObj->ADCSPkt.Extra, 
                     &EyasSatObj->ADCSPkt.Kp, &EyasSatObj->ADCSPkt.Ki, &EyasSatObj->ADCSPkt.Kd, &EyasSatObj->ADCSPkt.DeadBandScaleFactor) == 33)) {

      EyasSatObj->ADCSPkt.Hdr.PacketId = 5;
      CFE_PSP_MemCpy(&(EyasSatObj->ADCSPkt.Hdr.CallSign), CallSign, sizeof(EyasSatObj->ADCSPkt.Hdr.CallSign));
      CFE_PSP_MemCpy(&(EyasSatObj->ADCSPkt.Hdr.TimeString), TimeString, sizeof(EyasSatObj->ADCSPkt.Hdr.TimeString));

      CFE_SB_TimeStampMsg((CFE_SB_Msg_t *) &(EyasSatObj->ADCSPkt));
      EyasSatObj->AdcsTlmUpdate = true;
      // CFE_SB_SendMsg((CFE_SB_Msg_t *) &(EyasSatObj->ADCSPkt));

   } else {
      CFE_EVS_SendEvent (EYASSATOBJ_CONNECT_EID, CFE_EVS_CRITICAL, "EyasSat ADCS Packet Conversion Error: %d", status);
      // printf("Received EyasSat ADCS Telemetry: %s\n", &EyasSatObj->telemetryBuffer);
   }
   
} /* EYASSATOBJ_ProcessADCSPacket */

/******************************************************************************
** Function: EYASSATOBJ_InitSat
**
** Function to turn on pwr_tlm, adcs_tlm, and enable IMU on startup
**
*/
void EYASSATOBJ_InitSat() {

   char Cmd[EYASSATOBJ_CMD_BUFFER_SIZE];

   // Enable pwr_tlm
   snprintf(Cmd, sizeof(Cmd), "%.2s%d\n","dp", 1);
   CmdBufferPutCmd(Cmd);

   // Do it again.  The first eyassat command never works
   snprintf(Cmd, sizeof(Cmd), "%.2s%d\n","dp", 1);
   CmdBufferPutCmd(Cmd);

   // Enable 3.3V
   snprintf(Cmd, sizeof(Cmd), "%.2s%d\n","p1", 1);
   CmdBufferPutCmd(Cmd);

   // Enable ADCS power
   snprintf(Cmd, sizeof(Cmd), "%.2s%d\n","p2", 1);
   CmdBufferPutCmd(Cmd);

   // Enable IMU
   snprintf(Cmd, sizeof(Cmd), "%.2s\n","a9");
   CmdBufferPutCmd(Cmd);

   // Enable adcs_tlm
   snprintf(Cmd, sizeof(Cmd), "%.2s%d\n","da", 1);
   CmdBufferPutCmd(Cmd);

}

/******************************************************************************
** Function: EYASSATOBJ_MagCalCmd
**
** Command to set magnetometer calibration offsets
**
*/
boolean EYASSATOBJ_MagCalCmd(void* DataObjPtr, const CFE_SB_MsgPtr_t MsgPtr) {

   char Cmd[EYASSATOBJ_CMD_BUFFER_SIZE];
   
   const EYASSATOBJ_MagCalCmdMsg *CmdMsg = (const EYASSATOBJ_MagCalCmdMsg *) MsgPtr;

   CFE_EVS_SendEvent (EYASSATOBJ_CMD_INFO_EID,
                      CFE_EVS_INFORMATION,
                      "ADCS mag cal command received with parameters %f,%f,%f",
                      CmdMsg->MagCalX, CmdMsg->MagCalY, CmdMsg->MagCalZ );

   // Send X value
   snprintf(Cmd, sizeof(Cmd), "%.2s%.2f\n","a1", CmdMsg->MagCalX);
   CmdBufferPutCmd(Cmd);

   // Send Y value
   snprintf(Cmd, sizeof(Cmd), "%.2s%.2f\n","a2", CmdMsg->MagCalY);
   CmdBufferPutCmd(Cmd);

   // Send Z value
   snprintf(Cmd, sizeof(Cmd), "%.2s%.2f\n","a3", CmdMsg->MagCalZ);
   CmdBufferPutCmd(Cmd);

   return TRUE;
}

/******************************************************************************
** Function: EYASSATOBJ_GyroCalCmd
**
** Command to set gyro calibration offsets
**
*/
boolean EYASSATOBJ_GyroCalCmd(void* DataObjPtr, const CFE_SB_MsgPtr_t MsgPtr) {

   char Cmd[EYASSATOBJ_CMD_BUFFER_SIZE];
   
   const EYASSATOBJ_GyroCalCmdMsg *CmdMsg = (const EYASSATOBJ_GyroCalCmdMsg *) MsgPtr;

   CFE_EVS_SendEvent (EYASSATOBJ_CMD_INFO_EID,
                      CFE_EVS_INFORMATION,
                      "ADCS gyro cal command received with parameters %f,%f,%f",
                      CmdMsg->GyroCalX, CmdMsg->GyroCalY, CmdMsg->GyroCalZ );  
   
   
   // Send X value
   snprintf(Cmd, sizeof(Cmd), "%.2s%.2f\n","a4", CmdMsg->GyroCalX);
   CmdBufferPutCmd(Cmd);

   // Send Y value
   snprintf(Cmd, sizeof(Cmd), "%.2s%.2f\n","a5", CmdMsg->GyroCalY);
   CmdBufferPutCmd(Cmd);

   // Send Z value
   snprintf(Cmd, sizeof(Cmd), "%.2s%.2f\n","a6", CmdMsg->GyroCalZ);
   CmdBufferPutCmd(Cmd);

   return TRUE;

}

/******************************************************************************
** Function: EYASSATOBJ_SendCommand
** Process incoming commands from the SB message and forward them to the
** EyasSat C&DH board via /dev/console_c (UART3)
**
*/
void EYASSATOBJ_SendCommand()
{

   if (EyasSatObj->connected) {

      /* Send the string */
      for(int i=0; i<EYASSATOBJ_CMD_BUFFER_SIZE; i++) {
         
         // Reached the end of the command.  Retreat the pointer
         if (EyasSatObj->commandBuffer[EyasSatObj->CmdBufferTail][i] == '\0') {
            CmdBufferRetreatPointer();
            return;
         }
         /* Send 1 char */
         if (IO_TransRS422Write(EyasSatObj->fd, (uint8 *)&EyasSatObj->commandBuffer[EyasSatObj->CmdBufferTail][i], 1) != 1 ) {
            CFE_EVS_SendEvent (EYASSATOBJ_CONNECT_EID, CFE_EVS_CRITICAL, "EyasSat command write failure");
         }
      }
   } else {
      CFE_EVS_SendEvent (EYASSATOBJ_CONNECT_EID, CFE_EVS_CRITICAL, "EyasSat command failure.  UART not connected.");
   }
} /* EYASSATOBJ_SendCommands */

/******************************************************************************
** Function:  EYASSATOBJ_ResetStatus
**
*/
void EYASSATOBJ_ResetStatus(void)
{

   EyasSatObj->ExecCnt = 0;
   
} /* End EYASSATOBJ_ResetStatus() */


/******************************************************************************
** Function: EYASSATOBJ_GetTblPtr
**
*/
const EYASSATTBL_Struct* EYASSATOBJ_GetTblPtr(void)
{

   return &(EyasSatObj->Tbl);

} /* End EYASSATOBJ_GetTblPtr() */

/******************************************************************************
** Function: EYASSATOBJ_UpdateConfigParams
**
*/
boolean EYASSATOBJ_UpdateConfigParams() {

   boolean  RetStatus = TRUE;

   char Cmd[EYASSATOBJ_CMD_BUFFER_SIZE];

   // Send Yaw Cmd
   snprintf(Cmd, sizeof(Cmd), "%.2s%.1f\n","ao", EyasSatObj->Tbl.Config.YawCmd);
   CmdBufferPutCmd(Cmd);

   // Send PWM_Baseline
   snprintf(Cmd, sizeof(Cmd), "%.2s%d\n","am", EyasSatObj->Tbl.Config.PWM_Baseline);
   CmdBufferPutCmd(Cmd);

   // Send Mode
   snprintf(Cmd, sizeof(Cmd), "%.2s%d\n","ac", EyasSatObj->Tbl.Config.CtrlMode);
   CmdBufferPutCmd(Cmd);

   return RetStatus;

} /* End EYASSATOBJ_UpdateConfigParams() */

/******************************************************************************
** Function: EYASSATOBJ_UpdateConfigParams
**
*/
boolean EYASSATOBJ_UpdateDBParams() {

   boolean  RetStatus = TRUE;

   char Cmd[EYASSATOBJ_CMD_BUFFER_SIZE];

   // Deadband
   snprintf(Cmd, sizeof(Cmd), "%.2s%.2f\n","ab", EyasSatObj->Tbl.DB.Deadband);
   CmdBufferPutCmd(Cmd);

   // Deadband Scale Factor
   snprintf(Cmd, sizeof(Cmd), "%.2s%.2f\n","as", EyasSatObj->Tbl.DB.DeadBandScaleFactor);
   CmdBufferPutCmd(Cmd);

   // Extra
   snprintf(Cmd, sizeof(Cmd), "%.2s%.2f\n","ae", EyasSatObj->Tbl.DB.Extra);
   CmdBufferPutCmd(Cmd);

   return RetStatus;

} /* End EYASSATOBJ_UpdateDBParams() */

/******************************************************************************
** Function: EYASSATOBJ_UpdatePIDParams
**
*/
boolean EYASSATOBJ_UpdatePIDParams() {

   boolean  RetStatus = TRUE;

   char Cmd[EYASSATOBJ_CMD_BUFFER_SIZE];

   // Send Kp
   snprintf(Cmd, sizeof(Cmd), "%.2s%.2f\n","ap", EyasSatObj->Tbl.PID.Kp);
   CmdBufferPutCmd(Cmd);

   // Send Ki
   snprintf(Cmd, sizeof(Cmd), "%.2s%.2f\n","ai", EyasSatObj->Tbl.PID.Ki);
   CmdBufferPutCmd(Cmd);

   // Send Kd
   snprintf(Cmd, sizeof(Cmd), "%.2s%.2f\n","ad", EyasSatObj->Tbl.PID.Kd);
   CmdBufferPutCmd(Cmd);

   return RetStatus;

} /* End EYASSATOBJ_UpdatePIDParams() */

/******************************************************************************
** Function: EYASSATOBJ_UpdateMagCalParams
**
*/
boolean EYASSATOBJ_UpdateMagCalParams() {
   
   boolean  RetStatus = TRUE;

   char Cmd[EYASSATOBJ_CMD_BUFFER_SIZE];

   // Send X value
   snprintf(Cmd, sizeof(Cmd), "%.2s%.2f\n","a1", EyasSatObj->Tbl.MagCal.MagX);
   CmdBufferPutCmd(Cmd);

   // Send Y value
   snprintf(Cmd, sizeof(Cmd), "%.2s%.2f\n","a2", EyasSatObj->Tbl.MagCal.MagY);
   CmdBufferPutCmd(Cmd);

   // Send Z value
   snprintf(Cmd, sizeof(Cmd),"%.2s%.2f\n","a3", EyasSatObj->Tbl.MagCal.MagZ);
   CmdBufferPutCmd(Cmd);

   return RetStatus;

} /* End EYASSATOBJ_UpdateMagCalParams() */

/******************************************************************************
** Function: EYASSATOBJ_UpdateGyroCalParams
**
*/
boolean EYASSATOBJ_UpdateGyroCalParams() {

   boolean  RetStatus = TRUE;

   char Cmd[EYASSATOBJ_CMD_BUFFER_SIZE];

   // Send X value
   snprintf(Cmd, sizeof(Cmd), "%.2s%.2f\n","a4", EyasSatObj->Tbl.GyroCal.GyroX);
   CmdBufferPutCmd(Cmd);

   // Send Y value
   snprintf(Cmd, sizeof(Cmd), "%.2s%.2f\n","a5", EyasSatObj->Tbl.GyroCal.GyroY);
   CmdBufferPutCmd(Cmd);

   // Send Z value
   snprintf(Cmd, sizeof(Cmd), "%.2s%.2f\n","a6", EyasSatObj->Tbl.GyroCal.GyroZ);
   CmdBufferPutCmd(Cmd);

   return RetStatus;

} /* End EYASSATOBJ_UpdateGyroCalParams() */

/******************************************************************************
** Function: EYASSATOBJ_LoadTbl
**
*/
boolean EYASSATOBJ_LoadTbl(EYASSATTBL_Struct* NewTbl)
{

   boolean  RetStatus = TRUE;

   CFE_EVS_SendEvent (EYASSATOBJ_DEBUG_EID, CFE_EVS_DEBUG,"EYASSATOBJ_LoadTbl() Entered");

   CFE_PSP_MemCpy(&(EyasSatObj->Tbl), NewTbl, sizeof(EYASSATTBL_Struct));

   EyasSatObj->updateConfig = true;
   EyasSatObj->updateDB = true;
   EyasSatObj->updatePID = true;
   EyasSatObj->updateMagCal = true;
   EyasSatObj->updateGyroCal = true;

   CFE_EVS_SendEvent(EYASSATOBJ_DEBUG_EID, CFE_EVS_INFORMATION, "EYASSATOBJ_LoadTbl()");

   return RetStatus;

} /* End EYASSATOBJ_LoadTbl() */


/******************************************************************************
** Function: EYASSATOBJ_LoadTblEntry
**
*/
boolean EYASSATOBJ_LoadTblEntry(uint16 ObjId, void* ObjData)
{

   boolean  RetStatus = TRUE;

   CFE_EVS_SendEvent (EYASSATOBJ_DEBUG_EID, CFE_EVS_DEBUG,"EYASSATOBJ_LoadTblEntry() Entered ObjId: %d",ObjId);
   
   switch (ObjId) {
      
	  case EYASSATTBL_OBJ_CONFIG:
         CFE_PSP_MemCpy(&(EyasSatObj->Tbl.Config), ObjData, sizeof(EyasSatObj->Tbl.Config));
         EyasSatObj->updateConfig = true;
         break;
      
      case EYASSATTBL_OBJ_DB:
         CFE_PSP_MemCpy(&(EyasSatObj->Tbl.DB), ObjData, sizeof(EyasSatObj->Tbl.DB));
         EyasSatObj->updateDB = true;
         break;
      
      case EYASSATTBL_OBJ_PID:
         CFE_PSP_MemCpy(&(EyasSatObj->Tbl.PID), ObjData, sizeof(EyasSatObj->Tbl.PID));
         EyasSatObj->updatePID = true;
         break;
		 
      case EYASSATTBL_OBJ_MAGCAL:
         CFE_PSP_MemCpy(&(EyasSatObj->Tbl.MagCal), ObjData, sizeof(EyasSatObj->Tbl.MagCal));
         EyasSatObj->updateMagCal = true;
         break;
	  
      case EYASSATTBL_OBJ_GYROCAL:
         CFE_PSP_MemCpy(&(EyasSatObj->Tbl.GyroCal), ObjData, sizeof(EyasSatObj->Tbl.GyroCal));
         EyasSatObj->updateGyroCal = true;
         break;

      default:
	     RetStatus = FALSE;
   
   } /* End ObjId switch */

   CFE_EVS_SendEvent(EYASSATOBJ_DEBUG_EID, CFE_EVS_INFORMATION, "EYASSATOBJ_LoadTblEntry() ObjId: %hu", ObjId);

   return RetStatus;

} /* End EYASSATOBJ_LoadTblEntry() */

/******************************************************************************
** Function: EYASSATOBJ_DiscreteCmd
**
** Process EyasSat command messages with no command parameters
**
*/
boolean EYASSATOBJ_DiscreteCmd(void* DataObjPtr, const CFE_SB_MsgPtr_t MsgPtr) {

   char Cmd[EYASSATOBJ_CMD_BUFFER_SIZE];
   
   const EYASSATOBJ_DiscreteCmdMsg *CmdMsg = (const EYASSATOBJ_DiscreteCmdMsg *) MsgPtr;
   CFE_EVS_SendEvent (EYASSATOBJ_CMD_INFO_EID,
                  CFE_EVS_DEBUG,
                  "EyasSat discrete command sent with parameter %.2s",
                  CmdMsg->CmdCode);

   snprintf(Cmd, sizeof(Cmd), "%.2s\n",CmdMsg->CmdCode);
   CmdBufferPutCmd(Cmd);

   return TRUE;

} /* End EYASSATOBJ_DiscreteCmd() */

/******************************************************************************
** Function: EYASSATOBJ_Uint8Cmd
**
** Process EyasSat command messages with uint8 command parameters
**
*/
boolean EYASSATOBJ_Uint8Cmd(void* DataObjPtr, const CFE_SB_MsgPtr_t MsgPtr) {

   char Cmd[EYASSATOBJ_CMD_BUFFER_SIZE];
   
   const EYASSATOBJ_Uint8CmdMsg *CmdMsg = (const EYASSATOBJ_Uint8CmdMsg *) MsgPtr;
   CFE_EVS_SendEvent (EYASSATOBJ_CMD_INFO_EID,
                  CFE_EVS_DEBUG,
                  "EyasSat uint8 command sent with parameter %.2s %d",
                  CmdMsg->CmdCode, CmdMsg->CmdParameter);

   snprintf(Cmd, sizeof(Cmd), "%.2s%hhu\n", CmdMsg->CmdCode, CmdMsg->CmdParameter);
   CmdBufferPutCmd(Cmd);

   return TRUE;

} /* End EYASSATOBJ_Uint8Cmd() */

/******************************************************************************
** Function: EYASSATOBJ_Uint16Cmd
**
** Process EyasSat command messages with uint16 command parameters
**
*/
boolean EYASSATOBJ_Uint16Cmd(void* DataObjPtr, const CFE_SB_MsgPtr_t MsgPtr) {

   char Cmd[EYASSATOBJ_CMD_BUFFER_SIZE];

   const EYASSATOBJ_Uint16CmdMsg *CmdMsg = (const EYASSATOBJ_Uint16CmdMsg *) MsgPtr;
   CFE_EVS_SendEvent (EYASSATOBJ_CMD_INFO_EID,
                  CFE_EVS_DEBUG,
                  "EyasSat uint16 command sent with parameter %.2s %d",
                  CmdMsg->CmdCode, CmdMsg->CmdParameter);

   snprintf(Cmd, sizeof(Cmd), "%.2s%hu\n", CmdMsg->CmdCode, CmdMsg->CmdParameter);

   CmdBufferPutCmd(Cmd);

   return TRUE;

} /* End EYASSATOBJ_Uint16Cmd() */

/******************************************************************************
** Function: EYASSATOBJ_FloatCmd
**
** Process EyasSat command messages with float command parameters
**
*/
boolean EYASSATOBJ_FloatCmd(void* DataObjPtr, const CFE_SB_MsgPtr_t MsgPtr) {

   char Cmd[EYASSATOBJ_CMD_BUFFER_SIZE];

   const EYASSATOBJ_FloatCmdMsg *CmdMsg = (const EYASSATOBJ_FloatCmdMsg *) MsgPtr;
   CFE_EVS_SendEvent (EYASSATOBJ_CMD_INFO_EID,
                  CFE_EVS_DEBUG,
                  "EyasSat float command sent with parameter %.2s %f",
                  CmdMsg->CmdCode, CmdMsg->CmdParameter);

   snprintf(Cmd, sizeof(Cmd), "%.2s%f\n", CmdMsg->CmdCode, CmdMsg->CmdParameter);

   CmdBufferPutCmd(Cmd);

   return TRUE;

}  /* End EYASSATOBJ_FloatCmd() */

/******************************************************************************
** Function: EYASSATOBJ_ConnectCmd
**
** Command to connect to the EyasSat UART port. Connection occurs on startup
** This commmand is only required if a disconnect has been performed manually
**
*/
boolean EYASSATOBJ_ConnectCmd(void* DataObjPtr, const CFE_SB_MsgPtr_t MsgPtr) {

   EYASSATOBJ_ConsoleConnect();
   return TRUE;

}

/******************************************************************************
** Function: EYASSATOBJ_DisconnectCmd
**
** Command to disconnect from the EyasSat UART port.
**
*/
boolean EYASSATOBJ_DisconnectCmd(void* DataObjPtr, const CFE_SB_MsgPtr_t MsgPtr) {

   EYASSATOBJ_ConsoleDisconnect();
   return TRUE;

}

/* end of file */
