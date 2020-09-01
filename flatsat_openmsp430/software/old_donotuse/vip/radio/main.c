#include "omsp_radio.h"
#include "hardware.h"
#include "radio_hal.h"
#include "radio_process.h"
#include "app_uart.h"
#include "radio_keys.h"
#include "serial_loader.h"
#include <stdlib.h>
#include <stdio.h>

#define VIP_RADIO_VERSION	(1)

//--------------------------------------------------//
//                 putChar function                 //
//            (Send a byte to the UART)             //
//--------------------------------------------------//
int putchar (int txdata) {
 
  // Wait until the TX buffer is not full
  while (UART_STAT & UART_TX_FULL);
 
  // Write the output character
  UART_TXD = txdata;
 
  return 0;
}

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
        rxdata = UART_RXD;

        // Clear the receive pending flag
        UART_STAT = UART_RX_PND;

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
 
//--------------------------------------------------//
// Main function with init an an endless loop that  //
// is synced with the interrupts trough the         //
// lowpower mode.                                   //
//--------------------------------------------------//
int main(void) 
{
	WDTCTL = WDTPW | WDTHOLD;           // Disable watchdog timer

	// Setup UART 
	UART_BAUD = BAUD;                   // Init UART
	UART_CTL  = UART_EN | UART_IEN_RX;

	// Turn on TimerA to 15ms sync (note we will use this to seed rng)
	TACCR0 = RADIO_TACCR_TIME;
    	TACTL = ID_3 | TASSEL_2 | MC_1;
    	TACCTL0 |= CCIE;


	// Setup IO
	P2OUT = 0x01;
	P2DIR = 0xFF; 

	init_serial_loader();

	init_app_uart();
	radio_init();

	// Radio banner
	printf( "VIP Radio %d\n\n", VIP_RADIO_VERSION );	 

	// Enable interrupts!
	eint();


	while ( 1 )
	{
		radio_run();

		LPM0;		// Sync. wakeup by IRQ
		while ( is_radio_synced() )
		{
			LPM0;

			// Radio is awake... usually from an RX event...
			// Run the radio process as an event may have occurred
			radio_run();
			app_uart_run();
		}
	}
	
	return 0;
}
