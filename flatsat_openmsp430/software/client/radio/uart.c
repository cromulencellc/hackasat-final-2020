// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
#include "uart.h"
// ----------------------------------------------------------------------------

#define putc_fast( c ) (CONSOLE_UART_TXD = c)

// UART buffers
volatile uint8_t g_leon_uart_buf_rx[MAX_DATA_LENGTH];
volatile uint16_t g_leon_uart_buf_rxCount = 0;
volatile uint16_t g_leon_uart_buf_rxReadPos = 0;
volatile uint16_t g_leon_uart_buf_rxWritePos = 0;

volatile uint8_t g_leon_uart_buf_tx[MAX_DATA_LENGTH];
volatile uint16_t g_leon_uart_buf_txCount = 0;
volatile uint16_t g_leon_uart_buf_txReadPos = 0;
volatile uint16_t g_leon_uart_buf_txWritePos = 0;
// ----------------------------------------------------------------------------
volatile uint8_t g_appUartSending = 0;

// ----------------------------------------------------------------------------
wakeup interrupt (APP_UART_RX_VECTOR) INT_app_uart_rx(void)
{
  uint8_t in_char;
  in_char = APP_UART_RXD;
  // Clear the receive pending flag
  APP_UART_STAT = UART_RX_PND;
  if ( g_leon_uart_buf_rxCount < MAX_DATA_LENGTH )
  {
    g_leon_uart_buf_rx[g_leon_uart_buf_rxWritePos++] = in_char;
    if ( g_leon_uart_buf_rxWritePos >= MAX_DATA_LENGTH )
      g_leon_uart_buf_rxWritePos = 0;
    g_leon_uart_buf_rxCount++;
  }
  else
  {
    // RX BUFFER OVERRUN!
    g_leon_uart_buf_rx[g_leon_uart_buf_rxWritePos++] = in_char;
    if ( g_leon_uart_buf_rxWritePos >= MAX_DATA_LENGTH )
      g_leon_uart_buf_rxWritePos = 0;
    g_leon_uart_buf_rxReadPos++;
    if ( g_leon_uart_buf_rxReadPos >= MAX_DATA_LENGTH )
      g_leon_uart_buf_rxReadPos = 0;
  }
  // Exit the low power mode
}
// ----------------------------------------------------------------------------
wakeup interrupt (APP_UART_TX_VECTOR) INT_app_uart_tx( void )
{
  // Clear the tx pending flag
  APP_UART_STAT = UART_TX_PND;
  if ( g_leon_uart_buf_txCount > 0 )
  {
    // Start another transfer
    uint8_t sendByte = g_leon_uart_buf_tx[g_leon_uart_buf_txReadPos++];    
    if ( g_leon_uart_buf_txReadPos >= MAX_DATA_LENGTH )
      g_leon_uart_buf_txReadPos = 0;
    g_leon_uart_buf_txCount--;  
    APP_UART_TXD = sendByte;
  }
  else
    g_appUartSending = 0;         // Exit the low power mode
}
// ----------------------------------------------------------------------------
void uartInit( void )
{
  // init uart to leon 3 cpu
  APP_UART_BAUD = APP_UART_BAUD_VALUE;
  APP_UART_CTL = UART_EN | UART_IEN_RX | UART_IEN_TX;
  g_appUartSending = 0;

  bufClear(&g_oobm_buf_rx);
  bufClear(&g_oobm_buf_tx);
  
  g_leon_uart_buf_rxCount = 0;
  g_leon_uart_buf_rxReadPos = 0;
  g_leon_uart_buf_rxWritePos = 0;
  g_leon_uart_buf_txCount = 0;
  g_leon_uart_buf_txReadPos = 0;
  g_leon_uart_buf_txWritePos = 0;  
  
}
// ----------------------------------------------------------------------------
void uartRun( void )
{

  // Check to send anything to leon uart
  if ( g_leon_uart_buf_txCount > 0 && g_appUartSending == 0 )
  {
    g_appUartSending = 1;
    uint8_t sendByte = g_leon_uart_buf_tx[g_leon_uart_buf_txReadPos++];
    if ( g_leon_uart_buf_txReadPos >= MAX_DATA_LENGTH )
      g_leon_uart_buf_txReadPos = 0;
    g_leon_uart_buf_txCount--;  
    APP_UART_TXD = sendByte;
  }

  // check to receive from leon uart
  if ( g_leon_uart_buf_rxCount > 0 )
  {
    uint16_t toSendLen = g_leon_uart_buf_rxCount;
    if ( (g_leon_uart_buf_rxReadPos + toSendLen) > MAX_DATA_LENGTH )
    {
      uint16_t toSendFirst = (MAX_DATA_LENGTH - g_leon_uart_buf_rxReadPos);
      uint16_t toSendLast = ((g_leon_uart_buf_rxReadPos + toSendLen) - MAX_DATA_LENGTH);
      radio_send_leon3_data( g_leon_uart_buf_rx+g_leon_uart_buf_rxReadPos, toSendFirst );
      radio_send_leon3_data( g_leon_uart_buf_rx, toSendLast );
    }
    else
    {
      radio_send_leon3_data( g_leon_uart_buf_rx+g_leon_uart_buf_rxReadPos, toSendLen );
    }
    g_leon_uart_buf_rxCount -= toSendLen;
    g_leon_uart_buf_rxReadPos += toSendLen;
    if ( g_leon_uart_buf_rxReadPos >= MAX_DATA_LENGTH )
      g_leon_uart_buf_rxReadPos -= MAX_DATA_LENGTH;
  }
}

// ----------------------------------------------------------------------------
void leonGotRadioData( volatile uint8_t *pData, uint8_t dataLen )
{
  uint8_t i;
  // ECHO Testing purposes only
  //radio_send_leon3_data( pData, dataLen );
  // Add to buffer!
  for ( i = 0; i < dataLen; i++ )
  {
    if ( g_leon_uart_buf_txCount >= MAX_DATA_LENGTH )
      break;
    g_leon_uart_buf_tx[g_leon_uart_buf_txWritePos++] = pData[i];
    if ( g_leon_uart_buf_txWritePos >= MAX_DATA_LENGTH )
         g_leon_uart_buf_txWritePos = 0;  
    g_leon_uart_buf_txCount++;
  }
}
// ----------------------------------------------------------------------------
// EOF
// ----------------------------------------------------------------------------

