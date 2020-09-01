#include "app_uart.h"
#include "hardware.h"
#include "util.h"

#define MAX_LINE_LENGTH		(32)
#define MAX_DATA_LENGTH		(280)

void init_app_uart( void )
{
	APP_UART_BAUD = APP_BAUD;
	APP_UART_CTL = UART_EN | UART_IEN_RX;
}

void app_uart_write( uint8_t *pData, uint16_t dataLen )
{
	uint16_t pos = 0;
	for ( pos = 0; pos < dataLen; pos++ )
	{
		while ( APP_UART_STAT & UART_TX_FULL )
			;

		APP_UART_TXD = pData[pos];
	}
}
