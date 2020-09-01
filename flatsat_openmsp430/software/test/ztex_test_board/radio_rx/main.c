#include "omsp_system.h"
#include "hardware.h"
#include <stdlib.h>
#include <stdio.h>

#define RADIO_CSN_BIT		0x1
#define RADIO_CSN_PORT		(P2OUT)

#define RADIO_RESET_PORT	(P2OUT)
#define RADIO_RESET_BIT		(0x4)

//--------------------------------------------------//
//                   Delay function                 //
//--------------------------------------------------//
void delay(unsigned int d) {
   while(d--) {
      nop();
      nop();
   }
}

void radio_reset( void )
{
	unsigned int i;

	RADIO_RESET_PORT &= ~RADIO_RESET_BIT;

	for ( i = 0; i < 500; i++ )
		delay( 65000 );
	
	RADIO_RESET_PORT |= RADIO_RESET_BIT;
	
	for ( i = 0; i < 50; i++ )
		delay( 65000 );

	RADIO_RESET_PORT &= ~RADIO_RESET_BIT;

	for ( i = 0; i < 200; i++ )
		delay( 65000 );
}

void radio_spi_init( void )
{
	// Set everything low (clock will be SMCLK/2 or 8MHz)
	RADIOSPI_CTRL = (0x0000);

	// Set CSN idle high
	RADIO_CSN_PORT |= RADIO_CSN_BIT;
}

void radio_spi_writebyte( unsigned char data )
{
	RADIOSPI_DATA = data;
	RADIOSPI_CTRL |= RADIOSPI_CTRL_EN;

	while ( RADIOSPI_CTRL & RADIOSPI_CTRL_EN )
		;

}

unsigned char radio_spi_readbyte( void )
{
	unsigned char data;

	RADIOSPI_DATA = 0x0;
	RADIOSPI_CTRL |= RADIOSPI_CTRL_EN;

	while ( RADIOSPI_CTRL & RADIOSPI_CTRL_EN )
		;

	data = RADIOSPI_DATA;

	return (data);
}

void radio_writereg( unsigned char regNum, unsigned char value )
{
	RADIO_CSN_PORT &= ~RADIO_CSN_BIT;

	regNum |= 0x80;

	radio_spi_writebyte( regNum );
	radio_spi_writebyte( value );
	
	RADIO_CSN_PORT |= RADIO_CSN_BIT;	
}

void radio_writefifo( unsigned char *data, unsigned int count )
{
	unsigned int i;

	// The FIFO is only 256 bytes in size	
	if ( count > 256 )
		count = 256;

	RADIO_CSN_PORT &= ~RADIO_CSN_BIT;

	// Write in burst mode to FIFO register
	RADIOSPI_DATA = 0x00;
	RADIOSPI_CTRL |= RADIOSPI_CTRL_EN;

	while ( RADIOSPI_CTRL & RADIOSPI_CTRL_EN )
		;

	for ( i = 0; i < count; i++ )
	{
		RADIOSPI_DATA = data[i];
		RADIOSPI_CTRL |= RADIOSPI_CTRL_EN;

		while ( RADIOSPI_CTRL & RADIOSPI_CTRL_EN )
			;
	}
	
	RADIO_CSN_PORT |= RADIO_CSN_BIT;
}

unsigned char radio_readreg( unsigned char regNum )
{
	unsigned char data;

	RADIO_CSN_PORT &= ~RADIO_CSN_BIT;

	radio_spi_writebyte( regNum );
	data = radio_spi_readbyte();

	RADIO_CSN_PORT |= RADIO_CSN_BIT;

	return (data);	
}
 
 
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
 
wakeup interrupt (UART_RX_VECTOR) INT_uart_rx(void) {
 
  // Read the received data
  rxdata = UART_RXD;
 
  // Clear the receive pending flag
  UART_STAT = UART_RX_PND;
 
  // Exit the low power mode
  LPM0_EXIT;
}
 
 
//--------------------------------------------------//
// Main function with init an an endless loop that  //
// is synced with the interrupts trough the         //
// lowpower mode.                                   //
//--------------------------------------------------//
int main(void) 
{
	unsigned char txdata[32];
	unsigned int i;
	unsigned int good_packet_count;
	unsigned int bad_packet_count;
	unsigned int good_last_100_packet_count;
	unsigned int bad_last_100_packet_count;
	unsigned int packet_total_count;

    WDTCTL = WDTPW | WDTHOLD;           // Disable watchdog timer
 
    UART_BAUD = BAUD;                   // Init UART
    UART_CTL  = UART_EN | UART_IEN_RX;


    P2OUT = 0x01;
    P2DIR = 0xFF; 

	// Init TX DATA
	for ( i = 0; i < 32; i++ )
		txdata[i] = i;
    
    radio_spi_init();

	radio_reset();

	printf( "Testing radio.\n\n" );	 
	printf( "Going to LORA mode.\n" );
	// Enter sleep mode
	radio_writereg( 0x01, 0x00 );
	delay(65000);
	delay(65000);

	// Enter LORA mode and SLEEP
	radio_writereg( 0x01, 0x80 );
	delay(65000);
	delay(65000);

	while ( radio_readreg( 0x01 ) != 0x80 )
		printf ("Not in LORA yet.\n" );

	// LORA standby mode
	radio_writereg( 0x01, 0x81 );

	// Set the PA configuration to the PA_BOOST
	radio_writereg( 0x09, 0x80 );	// PA_BOOST

	// Select frequency 914.75 MHz
	radio_writereg( 0x06, 0xE4 );
	radio_writereg( 0x07, 0xC0 );
	radio_writereg( 0x08, 0x00 );

	// Modem configuration
	radio_writereg( 0x1D, 0x4E );
	
	// Modem configuration
	radio_writereg( 0x1E, 0x74 );

	// Set the payload length register to 32 bytes
	radio_writereg( 0x22, 0x20 );

	// Set the payload max length register to 32 bytes
	radio_writereg( 0x23, 0x20 );

	// Reset packet counters
	good_packet_count = 0;
	bad_packet_count = 0;
	good_last_100_packet_count = 0;
	bad_last_100_packet_count = 0;
	packet_total_count = 0;
	
	printf( "Start RX:\n" );
	radio_writereg( 0x01, 0x85 );
	for (;;)
	{

	// Now check the radio register to see when RX completes
	while ( (radio_readreg( 0x12 ) & 0x40) == 0 )
		delay( 65000 );

	if ( radio_readreg( 0x12 ) & 0x80 )
		printf( "RX Timeout.\n" );

	// Check CRC
	if ( (radio_readreg( 0x12 ) & 0x20) )
	{
		bad_packet_count++;
		printf( "Bad RSSI: %d\n", radio_readreg( 0x1A ) + -125 );
	}
	else
	{
		printf( "GOOD RSSI: %d\n", radio_readreg( 0x1A ) + -125 );
		good_packet_count++;
	}
	
	// Clear RX done flag
	radio_writereg( 0x12, 0xFF );

	// Do maths
	packet_total_count++;

	if ( (packet_total_count % 100) == 0 )	
	{
		unsigned int diff_good = (good_packet_count - good_last_100_packet_count);
		unsigned int diff_bad = (bad_packet_count - bad_last_100_packet_count);
		printf( "Good %d and Bad %d out of 100) %d Total\n", diff_good, diff_bad, packet_total_count );

		good_last_100_packet_count = good_packet_count;
		bad_last_100_packet_count = bad_packet_count; 
	}
	/*
	printf( "Reg 0x01: %X\n", radio_readreg( 0x01 ) );
	printf( "Reg 0x12: %X\n", radio_readreg( 0x12 ) );
	*/
	}

	return 0;
}
