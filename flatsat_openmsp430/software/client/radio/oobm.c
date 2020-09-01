#include "oobm.h"



t_circbuf g_oobm_buf_rx;
t_circbuf g_oobm_buf_tx;

uint8_t g_temp_buf[2052];
uint8_t g_oob_tbuf[2052];

#define putc_fast( c ) (CONSOLE_UART_TXD = c)

// ----------------------------------------------------------------------------
// called by main loop to handle oob transfers
// ----------------------------------------------------------------------------
void oobmRun()
{
  if ( g_oobm_buf_tx.count )    // outgoing data
  { 
    uint16_t ct=0;
    ct = bufGet(&g_oobm_buf_tx,g_temp_buf,2052);
    radio_send_oobm_data(g_temp_buf,ct);    
  }

  if (g_oobm_buf_rx.count<4)
    return;

  // check if theres a complete packet
  while(1)
  {
    uint16_t packet_len = oobmCheckFrame();
    if(!packet_len)
      break;
    // have a whole packet from radio. Chop it off and process it.
    bufGet(&g_oobm_buf_rx,g_oob_tbuf,packet_len);
    uint8_t dest_nyb = g_oob_tbuf[1]&0x0f;
    switch(dest_nyb)
    {
      case NODE_OOBM:
        oobmCmd(g_oob_tbuf+4,packet_len-4);     // got msg for management cpu
        break;
      case NODE_LEON:                           
        leonGotRadioData( g_oob_tbuf+4, packet_len-4 );
        break;
    }
  }
    
}
// ----------------------------------------------------------------------------
// Realigns oobm rx buf as needed and checks for full frames. If theres a whole
// frame in there, return its length, else return 0.
//
//   message format:
//
//   [ 0xA5, 0x5X, HH, LL, ... ]
//
// X is the uplink node nybble (0: LoRa Radio, 1: Manager CPU, 2: Flight CPU)
// HH & LL are the uint16_t big-endian length of remaining bytes.
// 
// ----------------------------------------------------------------------------
inline uint16_t oobmCheckFrame()
{
  while(g_oobm_buf_rx.count>=2)
  {
    uint8_t b0 = bufPeekByte(&g_oobm_buf_rx,0); 
    if (b0!=0xA5)
    { 
      bufGetByte(&g_oobm_buf_rx);  // chop and retry
      continue;
    }
    uint8_t b1 = bufPeekByte(&g_oobm_buf_rx,1);
    if ((b1&0xf0) != 0x50)
    {
      bufGetByte(&g_oobm_buf_rx); // chop and retry
      continue;
    }
    if(g_oobm_buf_rx.count<4)  
    {
      return 0;
    }
    uint16_t HH,LL;    
    HH = bufPeekByte(&g_oobm_buf_rx,2);
    LL = bufPeekByte(&g_oobm_buf_rx,3);
    uint16_t payload_len = (HH<<8) | LL;

    if (payload_len>1024+1024)
    { // len sanity check fail
      bufGetByte(&g_oobm_buf_rx);
      continue;      
    }
    // got whole packet. return its total length.   
    if (g_oobm_buf_rx.count>=payload_len+4)
      return payload_len+4;
    return 0;
  }
  return 0;
}

// ----------------------------------------------------------------------------
// called by radio when it has something for us
// ----------------------------------------------------------------------------
void oobmGotRadioData( uint8_t *pData, uint8_t dataLen )
{
  // loopback hard coded
  //radio_send_oobm_data(pData,dataLen);

  bufAdd(&g_oobm_buf_rx,pData,dataLen);

}
// ----------------------------------------------------------------------------
// Make a uart frame with optional payload, and add to outgoing 
// ----------------------------------------------------------------------------
void oobmSendFrame(uint8_t node_id, volatile uint8_t *payload, uint16_t len)
{
	printf( "OO:%d\n", len );
  uint8_t buf[1028+1024];
  if (len>1024+1024)
    return;  
  buf[0]=0xa5;
  buf[1]=0x50 | node_id;
  buf[2]=(len>>8)&0xff;
  buf[3]=(len&0xff);
  uint16_t i;
  for(i=0;i<len;i++)
    buf[4+i]=payload[i];     
  bufAdd(&g_oobm_buf_tx,buf, len + 4);
}
// ----------------------------------------------------------------------------
void oobmSendStatusCode(uint8_t code)
{
  uint8_t buf[2];
  buf[0]=CMD_STATUS;
  buf[1]=code; 
  oobmSendFrame(NODE_OOBM, buf, 2);
}
// ----------------------------------------------------------------------------
// For radio enumeration, Send ESP32 mac_address and team_id via uart
// ----------------------------------------------------------------------------
void oobmSendHail()
{
  uint8_t buf[2];
  buf[0]=CMD_HAIL;
  buf[1]=MY_TEAM_ID; 
  oobmSendFrame(NODE_OOBM, buf, 2);
}
// ----------------------------------------------------------------------------
void oobmCmd(uint8_t * buf, uint16_t len)
{
  uint8_t cmd_id = buf[0];
  
  switch(cmd_id)
  {
    case CMD_HAIL:
      oobmSendHail();
      break;
    case CMD_LOOPBACK:
      oobmSendFrame(NODE_OOBM,buf,len);
      break;
    case CMD_LEON3_RESET_ON:
      oobmSendFrame(NODE_OOBM,buf,len);

      // Pull reset_n low -- to hold it in reset
      P2OUT &= ~LEON3_RESET_PIN;
      break;

    case CMD_LEON3_RESET_OFF:
      oobmSendFrame(NODE_OOBM,buf,len);

      // Set reset_n high -- to hold it in reset
      P2OUT |= LEON3_RESET_PIN;
      break;

    case CMD_LEON3_RESET_STATE:
      {
        uint8_t response[2];
        response[0] = cmd_id;
	response[1] = ((P2OUT & LEON3_RESET_PIN) > 0);

      }
      break;

    default:
      oobmSendStatusCode(EC_CMD_UNKNOWN);
      break;
  }  
}
// ----------------------------------------------------------------------------
