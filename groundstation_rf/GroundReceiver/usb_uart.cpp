// ----------------------------------------------------------------------------
// Calling this code "USB" is really a misnomer since it is a plain UART
// going through a CP2102 USB-UART adaptor, but it helps describe which comms 
// port we're dealing with here.
// ----------------------------------------------------------------------------
#include "usb_uart.h"
#include "oled.h"
// ----------------------------------------------------------------------------
t_circbuf     g_uart_buf_tx;              // buf of stuff to send
t_circbuf     g_uart_buf_rx;              // buf of stuff received

t_circbuf     g_oobm_buf_rx;              // incoming radio data from oobm

uint32_t      g_tm_comms_last = 0;        // for checking if comms idle
uint8_t       g_tbuf[FRAME_MAX];

// We use this buffer in this uart code, but the radio can interrupt us to
// poke in bytes received from flatsat.

portMUX_TYPE uart_tx_mtx = portMUX_INITIALIZER_UNLOCKED;

#define uart_num UART_NUM_0

uart_config_t uart_config = {
    .baud_rate = USB_UART_BAUDRATE,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    .rx_flow_ctrl_thresh = 122,
};
QueueHandle_t uart_queue;

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void uartInit()
{
  bufClear(&g_uart_buf_tx);
  bufClear(&g_uart_buf_rx);
  
  bufClear(&g_oobm_buf_rx);
  
  // Serial.setRxBufferSize(FRAME_MAX+1);
  // Serial.begin( USB_UART_BAUDRATE );

  // Configure UART parameters
  ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
  
  // Set UART pins(TX: IO16 (UART2 default), RX: IO17 (UART2 default), RTS: IO18, CTS: IO19)
  // ESP_ERROR_CHECK(uart_set_pin(UART_NUM_2, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, 18, 19));
  
  // Setup UART buffered IO with event queue
  const int uart_buffer_size = (1024 * 2)+4;
  
  // Install UART driver using an event queue here
  ESP_ERROR_CHECK(uart_driver_install(uart_num, uart_buffer_size, uart_buffer_size, 10, &uart_queue, 0));  
      
}
// ----------------------------------------------------------------------------
// called by main loop to handle USB UART transfers
// ----------------------------------------------------------------------------
void uartService(uint32_t tm_now)
{

  //if ( Serial.available() > 0 ) // incoming data

  size_t len = 0;
  ESP_ERROR_CHECK(uart_get_buffered_data_len(uart_num, &len));    
  if(len>0)
  {
    g_tm_comms_last = tm_now;
    uartRx();
    ledBlip();
  }  
  if ( g_uart_buf_tx.count )    // outgoing data
  { 
    g_tm_comms_last = tm_now;
    uartTx();
    ledBlip();
  }
}
// ----------------------------------------------------------------------------
// send outgoing
// ----------------------------------------------------------------------------
void uartTx()
{
  unsigned i,ct=0;
  
  //int avail = Serial.availableForWrite();

  portENTER_CRITICAL( &uart_tx_mtx );
  ct = bufGet(&g_uart_buf_tx,g_tbuf,FRAME_MAX);
  portEXIT_CRITICAL( &uart_tx_mtx );

  // if dbg printing enabled, show tx data on bottom line of oled
  _dbgHex1(g_tbuf,ct);
  
  //Serial.write(g_tbuf, ct);   // note: this will block if not enough room 
                              // in arduino's 64 byte tx buffer

  uart_write_bytes(uart_num, (const char*)g_tbuf, ct);                              
}
// ----------------------------------------------------------------------------
// Make a uart frame with optional payload, and add to outgoing 
// ----------------------------------------------------------------------------
int uartSendFrame(uint8_t node_id, volatile uint8_t *payload, uint16_t len)
{
  uint8_t tbuf[FRAME_MAX];
  
  if (len>PAYLOAD_MAX)
  {
    return -1;  
  }
  
  tbuf[0]=0xa5;
  tbuf[1]=0x50 | node_id;
  tbuf[2]=len>>8;
  tbuf[3]=len&0xff;
  
  for(uint16_t i=0;i<len;i++)
    tbuf[4+i]=payload[i];
  
  portENTER_CRITICAL( &uart_tx_mtx );     
  bufAdd(&g_uart_buf_tx,tbuf, len + 4);
  portEXIT_CRITICAL( &uart_tx_mtx );  
}
// ----------------------------------------------------------------------------
// Realigns uart rx buffer to frame boundary, if needed, and if theres a whole 
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
uint16_t uartCheckFrame(t_circbuf * cbuf)
{
  while(cbuf->count>=2)
  {
    uint8_t b0 = bufPeekByte(cbuf,0); 
    if (b0!=0xA5)
    { 
      bufGetByte(cbuf);  // chop and retry
      continue;
    }
    
    uint8_t b1 = bufPeekByte(cbuf,1);
    if ((b1&0xfc) != 0x50)
    {
      bufGetByte(cbuf); // chop and retry
      continue;
    }

    if(cbuf->count<4)  
      return 0;

    uint16_t HH = bufPeekByte(cbuf,2) & 0xff;
    uint16_t LL = bufPeekByte(cbuf,3) & 0xff;
    uint16_t payload_len = (HH<<8) | LL;

    if (payload_len>PAYLOAD_MAX)
    {       
      bufGetByte(cbuf);   // len sanity check fail
      continue;      
    }
    
    // got whole packet. return its total length.   
    
    if (cbuf->count>=(payload_len+4))
      return payload_len+4;

    // dont have whole payload yet.
    return 0;
  }
  
  return 0;
}
// ----------------------------------------------------------------------------
// receive from uart host
// ----------------------------------------------------------------------------
void uartRx()
{
  size_t len = 0;
  ESP_ERROR_CHECK(uart_get_buffered_data_len(uart_num, &len));
  
  // enqueue incoming bytes
  // int ba = Serial.available();
  int ba = len;
  if(ba>FRAME_MAX)
    ba=FRAME_MAX;

  //int ct = Serial.readBytes(g_tbuf,ba);
  int ct = uart_read_bytes(uart_num, g_tbuf, ba, 100);

  
  bufAdd(&g_uart_buf_rx,g_tbuf,ct);

  #if DEBUG_LOGGING  // if dbg printing enabled,
  // show rx data on next-to-bottom oled line.
  ct = bufPeek(&g_uart_buf_rx,g_tbuf,PAYLOAD_MAX);
  _dbgHex0(g_tbuf,ct);
  #endif

  if (g_uart_buf_rx.count<4)
    return;

  // check if theres a complete packet
  while(1)
  {
    uint16_t packet_len = uartCheckFrame(&g_uart_buf_rx);
    if(!packet_len)
      break;

    // have a whole packet via uart. Chop it off and process it.
    
    bufGet(&g_uart_buf_rx,g_tbuf,packet_len);

    uint8_t dest_nyb = g_tbuf[1]&0x0f;
    switch(dest_nyb)
    {
      case NODE_RADIO:
        handleRadioCmd(g_tbuf+4,packet_len-4);   // got msg for this device
        break;
      case NODE_OOBM:
        write_oobm(g_tbuf,packet_len);     // got msg for management cpu
        break;
      case NODE_LEON:
        write_leon3(g_tbuf+4,packet_len-4);    // got msg for flight cpu
        break;
    }
  }

}
// ----------------------------------------------------------------------------
// Handle uart message intended for this device
// ----------------------------------------------------------------------------
void handleRadioCmd(uint8_t *buf, uint16_t len)
{
  uint8_t cmd_id = buf[0];
  
  switch(cmd_id)
  {
    case CMD_HAIL:
      uartSendHail();
      break;
    case CMD_STATUS:
      uartSendStatus();
      break;
    case CMD_LOOPBACK:
      uartSendFrame(NODE_RADIO, buf, len);
      break;
    case CMD_PKT_LOSS:
      uartSendPktLoss();
      break;    
    default:
      uartSendStatusCode(EC_CMD_UNKNOWN);
      break;
  }
}
// ----------------------------------------------------------------------------
void uartSendPktLoss()
{
  uint8_t buf[5];
  buf[0]=CMD_PKT_LOSS;
  *(float *)(buf+1) = g_data_loss_pct;
  uartSendFrame(NODE_RADIO, buf, 5);
  
}
// ----------------------------------------------------------------------------
void uartSendStatusCode(uint8_t code)
{
  uint8_t buf[2];
  buf[0]=CMD_STATUS;
  buf[1]=code; 
  uartSendFrame(NODE_RADIO, buf, 2);
}
// ----------------------------------------------------------------------------
// For radio enumeration, Send ESP32 mac_address and team_id via uart
// ----------------------------------------------------------------------------
void uartSendHail()
{
  uint8_t buf[8];
  buf[0]=CMD_HAIL;
  buf[1]=MY_TEAM_ID; 
  esp_read_mac(buf+2, ESP_MAC_WIFI_STA);
  uartSendFrame(NODE_RADIO, buf, 8);
}
// ----------------------------------------------------------------------------
void uartSendStatus()
{
  uint8_t buf[8];
  buf[0]=CMD_STATUS;
  uartSendFrame(NODE_RADIO, buf, 8);
}
// ----------------------------------------------------------------------------
// Take incoming leon3 data from radio, frame it, and send out uart.
// ----------------------------------------------------------------------------
void process_leon3_data( volatile uint8_t *buf, uint8_t len )
{
  uartSendFrame( NODE_LEON, buf, len);
}
// ----------------------------------------------------------------------------
// Take incoming oobm data from radio, buffer until we have whole uart frame,
// then add to outgoing.
// ----------------------------------------------------------------------------
void process_oobm_data( volatile uint8_t *buf, uint8_t len )
{
  uint8_t tbuf[FRAME_MAX];
  bufAdd(&g_oobm_buf_rx,(uint8_t *)buf,len); 
  while(1)
  {
    uint16_t packet_len = uartCheckFrame(&g_oobm_buf_rx);
    if(!packet_len)
      break;
    // have a msg from oobm to put in uart tx
    bufGet(&g_oobm_buf_rx,tbuf,packet_len);
    portENTER_CRITICAL( &uart_tx_mtx );    
    bufAdd(&g_uart_buf_tx,tbuf, packet_len);
    portEXIT_CRITICAL( &uart_tx_mtx );    
  }
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
