#include "omsp_radio.h"
#include "cdh_common.h"
#include "radio_hal.h"
#include "radio_process.h"
#include "uart.h"
#include "radio_keys.h"
#include "serial_loader.h"
#include <stdlib.h>
#include <stdio.h>

#include "oobm.h"

//--------------------------------------------------//
//                 putChar function                 //
//            (Send a byte to the UART)             //
//--------------------------------------------------//
int putchar (int txdata) {
 
  // Wait until the TX buffer is not full
  while (CONSOLE_UART_STAT & UART_TX_FULL);
 
  // Write the output character
  CONSOLE_UART_TXD = txdata;
 
  return 0;
}

#ifdef APP_SERIAL_LOADER_EN 
//--------------------------------------------------//
//        UART RX interrupt service routine         //
//         (receive a byte from the UART)           //
//--------------------------------------------------//
#define MAX_SERIAL_LINE_LEN     32
unsigned char g_serialLine[MAX_SERIAL_LINE_LEN];
unsigned char g_serialLineLen;

wakeup interrupt (PROGRAM_UART_RX_VECTOR) INT_uart_rx(void)
{
        char rxdata;

        // Read the received data
        rxdata = CONSOLE_UART_RXD;

        // Clear the receive pending flag
        CONSOLE_UART_STAT = UART_RX_PND;

        if ( rxdata == '\n' || rxdata == '\r' )
        {
                g_serialLine[g_serialLineLen] = '\0';

                process_serial_loader_line( g_serialLine );
                g_serialLineLen = 0;
        }
        else if ( g_serialLineLen < MAX_SERIAL_LINE_LEN-1 )
        {
                g_serialLine[g_serialLineLen++] = rxdata;
                putchar('>');
        }
        // ELSE: Ignore 
        LPM0_EXIT;
}
#endif
 
//--------------------------------------------------//
// Main function with init an an endless loop that  //
// is synced with the interrupts trough the         //
// lowpower mode.                                   //
//--------------------------------------------------//
int main(void) 
{
	WDTCTL = WDTPW | WDTHOLD;           // Disable watchdog timer

	// Setup Console UART 
	CONSOLE_UART_BAUD = CONSOLE_UART_BAUD_VALUE;                   // Init UART
	CONSOLE_UART_CTL  = UART_EN | UART_IEN_RX;

	// Turn on TimerA to 17.15ms sync (note we will use this to seed rng)
	TACCR0 = RADIO_TACCR_TIME;
    	TACTL = ID_3 | TASSEL_2 | MC_1;
    	TACCTL0 |= CCIE;


	// Setup IO
	P2OUT = (LEON3_RESET_PIN | 0x03);	// Set CSN high for radio and config flash and Leon-3 high
	P2DIR = 0xFF; 

	radio_init();
	conflash_init();
	uartInit();

#ifdef DEBUG_RADIO_ON
	printf( "\r\n===========RADIO RESET===========\r\n" );
	printf( "+++DEBUG BUILD+++ radio.\r\n" );	 
	printf( "RADIO Team ID: %d\r\n", MY_TEAM_ID );
#endif
	// Enable interrupts!
	eint();

	while ( 1 )
	{
		LPM0;

		// Radio is awake... usually from an RX event...
		// Run the radio process as an event may have occurred
		radio_run();
		uartRun();
		oobmRun();
	}
	
	return 0;
}
