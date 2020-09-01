#include "app_uart.h"
#include "radio_process.h"
#include "hardware.h"
#include "util.h"

// These codes are sent out to the APP processor to inform the app processor the states of the radio
#define RADIOSTATE_OFF			0
#define RADIOSTATE_FINDSYNC		1
#define RADIOSTATE_SYNC			2

// Keeps track of the previous state to inform the app processor when it changes
volatile uint8_t g_radioStatePrevious;


void init_app_uart( void )
{
	APP_UART_BAUD = APP_BAUD;
	APP_UART_CTL = UART_EN | UART_IEN_RX;

	g_radioStatePrevious = RADIOSTATE_OFF;
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

void app_uart_run( void )
{
	uint16_t currentRadioState;

	if ( is_radio_synced() )
		currentRadioState = RADIOSTATE_SYNC;
	else
	{
		if ( is_radio_on() )
			currentRadioState = RADIOSTATE_FINDSYNC;
		else
			currentRadioState = RADIOSTATE_OFF;
	}

	if ( currentRadioState != g_radioStatePrevious )
	{
		uint8_t radioData[6];
		g_radioStatePrevious = currentRadioState;

		radioData[0] = 'R';
		radioData[1] = 'S';
		radioData[2] = ':';
		radioData[3] = '0' + currentRadioState;
		radioData[4] = '\n';

		// Write out radio state
		app_uart_write( radioData, 5 );
	}
}
