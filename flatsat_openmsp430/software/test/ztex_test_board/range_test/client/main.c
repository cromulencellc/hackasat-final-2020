#include "omsp_radio.h"
#include "hardware.h"
#include "radio_hal.h"
#include "radio_process.h"
#include "app_uart.h"
#include <stdlib.h>
#include <stdio.h>

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
/*
volatile char rxdata;
 
wakeup interrupt (PROGRAM_UART_RX_VECTOR) INT_uart_rx(void) {
 
  // Read the received data
  rxdata = UART_RXD;
 
  // Clear the receive pending flag
  UART_STAT = UART_RX_PND;
}
*/
 
 
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

	// Turn on TimerA 
	TACCR0 = 65000;
	TACTL = ID_3 | TASSEL_2 | MC_1;
	TACCTL0 |= CCIE;

	// Setup IO
	P2OUT = 0x01;
	P2DIR = 0xFF; 

	// Init the application UART
	init_app_uart();

	// Init radio
	radio_init();

	printf( "Range Test... client on.\n\n" );	 

	eint();

	while ( 1 )
	{
		LPM0;
	}
}
