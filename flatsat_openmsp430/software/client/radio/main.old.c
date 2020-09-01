#include "omsp_radio.h"
#include "hardware.h"
#include "radio_hal.h"
#include "radio_process.h"
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
volatile char rxdata;
 
wakeup interrupt (PROGRAM_UART_RX_VECTOR) INT_uart_rx(void) {
 
  // Read the received data
  rxdata = UART_RXD;
 
  // Clear the receive pending flag
  UART_STAT = UART_RX_PND;
 
  // Exit the low power mode
  // LPM0_EXIT;
}
 
//--------------------------------------------------//
// Main function with init an an endless loop that  //
// is synced with the interrupts trough the         //
// lowpower mode.                                   //
//--------------------------------------------------//
int main(void) 
{
	uint8_t i;
	WDTCTL = WDTPW | WDTHOLD;           // Disable watchdog timer


	// Setup UART 
	UART_BAUD = BAUD;                   // Init UART
	UART_CTL  = UART_EN | UART_IEN_RX;

	// Turn on TimerA to 15ms sync (note we will use this to seed rng)
	TACCR0 = 30000;
    	TACTL = ID_3 | TASSEL_2 | MC_1;
    	TACCTL0 |= CCIE;


	// Setup IO
	P2OUT = 0x01;
	P2DIR = 0xFF; 

	radio_init();

	printf( "TACCR0 = %d\n", TACCR0 );
	printf( "TACTL = %X\n", TACTL );
	printf( "TACCTL0 = %X\n", TACCTL0 );
	// Enable interrupts!
	eint();
	while ( 1 )
	{

	}

	printf( "Testing radio.\n\n" );	 
	printf( "\r\n===========RADIO RESET===========\r\n" );


	while ( 1 )
	{
		printf( "Looking for sync.\n" );
		radio_findsync();

		for ( i = 0; i < 100; i++ )	
			delay(0xFFFF);	
		//LPM0;		// Sync. wakeup by IRQ

		while ( is_radio_synced() )
		{
			printf( "[OUT]Sync\n" );
			LPM0; 	// Sync. wakeup by IRQ
		}
	}
/*
	while ( 1 )
	{
		RADIO_CSN_PORT &= ~RADIO_CSN_BIT;	
		radio_spi_writebyte( 0x01 );
		regValue = radio_spi_readbyte();	
		RADIO_CSN_PORT |= RADIO_CSN_BIT;

		printf( "Reg 0x01: %X\n", regValue );	

		delay(65000);	
		
		RADIO_CSN_PORT &= ~RADIO_CSN_BIT;	
		radio_spi_writebyte( 0x02 );
		regValue = radio_spi_readbyte();	
		RADIO_CSN_PORT |= RADIO_CSN_BIT;

		printf( "Reg 0x02: %X\n", regValue );	

		delay(65000);	
		
		RADIO_CSN_PORT &= ~RADIO_CSN_BIT;	
		radio_spi_writebyte( 0x03 );
		regValue = radio_spi_readbyte();	
		RADIO_CSN_PORT |= RADIO_CSN_BIT;

		printf( "Reg 0x03: %X\n", regValue );	

		delay(65000);	
		delay(65000);	
		delay(65000);	
	}
*/
/*
	printf( "Going to LORA mode.\n" );
	// Enter sleep mode
	radio_writereg( 0x01, 0x00 );

	// Enter LORA mode and SLEEP
	radio_writereg( 0x01, 0x80 );

	while ( radio_readreg( 0x01 ) != 0x80 )
		printf ("Not in LORA yet.\n" );

	// LORA standby mode
	radio_writereg( 0x01, 0x81 );

	// Set the PA configuration to the PA_BOOST
	radio_writereg( 0x09, 0x80 );	// PA_BOOST

	// Modem configuration
	radio_writereg( 0x1D, 0x4E );
	
	// Modem configuration
	radio_writereg( 0x1E, 0x64 );

	// Set the payload length register to 32 bytes
	radio_writereg( 0x22, 0x20 );

	// Set FIFO base address to base address of TX buffer
	radio_writereg( 0x0D, 0x80 );
	// Write the TX Data to the FIFO using burst mode
	radio_writefifo( txdata, 32 );

	for (;;)
	{
	printf( "Start TX:\n" );

	radio_writereg( 0x01, 0x83 );

	// Now check the radio register to see when TX completes
	while ( (radio_readreg( 0x12 ) & 0x08) == 0 )
		printf( "." );

	// Clear TX done flag
	radio_writereg( 0x12, 0x08 );
	
	printf( "TX Complete.\n" );

	printf( "Reg 0x01: %X\n", radio_readreg( 0x01 ) );
	printf( "Reg 0x12: %X\n", radio_readreg( 0x12 ) );
	}
*/
	return 0;
}
