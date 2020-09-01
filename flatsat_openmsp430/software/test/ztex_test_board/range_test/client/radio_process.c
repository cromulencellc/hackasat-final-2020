#include "radio_process.h"
#include "omsp_radio.h"
#include "hardware.h"
#include "radio_hal.h"
#include "sram_hal.h"
#include "radio_packet.h"

#include <string.h>

#define RANGE_PACKET_SIZE		32

#define RADIO_DIO0_BIT		0x08
#define RADIO_DIO1_BIT		0x10

#define RADIO_MODE_TX		0
#define RADIO_MODE_RX		1

volatile uint8_t g_responsePacket[RANGE_PACKET_SIZE];
volatile uint8_t g_responseAvailable;
volatile uint8_t g_packetNumber;
volatile uint8_t g_radioMode;

wakeup interrupt (PORT1_VECTOR) INT_button( void )
{
	dint();
	P1IFG = 0;
	if ( P1IN & RADIO_DIO0_BIT )
	{
		if ( g_radioMode == RADIO_MODE_RX )
		{
		if ( !(radio_readreg( 0x12 ) & 0x20 ) )
		{
			// Set base address	
			radio_writereg( 0x0D, 0x00 );
				
			// Clear interrupts
			radio_writereg( 0x12, 0xFF );
		
			// Read packet and ready it for processing!
			radio_readfifo( g_responsePacket, RANGE_PACKET_SIZE );

			g_responseAvailable = 1;

			printf( "RSSI: %d, %d\n", radio_readreg( 0x1A ), g_responsePacket[0] );
		}
		}
		else if ( g_radioMode == RADIO_MODE_TX )
		{
			putchar('R');
			radio_receive();
			g_radioMode = RADIO_MODE_RX;
		}
	}
	else if ( P1IN & RADIO_DIO1_BIT )
	{
		// Timeout
		putchar( 'X' );

		radio_writereg( 0x12, 0x80 );	
	}
	eint();

	LPM0_EXIT;
}

void radio_init( void )
{
	g_responseAvailable = 0;
	g_packetNumber = 0;

	// Setup radio
	radio_reset();
	radio_spi_init();

	// Enter sleep mode
	radio_writereg( 0x01, 0x00 );

	// Enter LORA mode and SLEEP
	radio_writereg( 0x01, 0x80 );

	while ( radio_readreg( 0x01 ) != 0x80 )
	{
		// TODO: DEBUG code change this later
	}

	// LORA standby mode
	radio_writereg( 0x01, 0x81 );

	// Set the PA configuration to the PA_BOOST
	radio_writereg( 0x09, 0x80 );   // PA_BOOST

	// Setup modem parameters
	radio_writereg( 0x1D, 0x96 );	// SF=6, BW=500, CR=4/6, Implicit header and CRC enable
	radio_writereg( 0x1E, 0x64 );

	// Symbol timeout
	radio_writereg( 0x1F, RADIO_PREAMBLE_RX_TIMEOUT );

	// Preamble length
	radio_writereg( 0x21, RADIO_PROGRAMMED_PREAMBLE_LEN );

	// FROM DATASHEET: write register 0x37 to 0x0C
	radio_writereg( 0x37, 0x0c );
	radio_writereg( 0x31, 0x05 );

	// Setup DIO pins
	P1DIR = 0x00;
	P1IE = 0x18;

	// Initial radio setup complete.
}

void radio_send( void )
{
	// SEND VIP PACKET
	uint16_t channel;

	// SEND PACKET
	// NOW SEND
    channel = 20;

	channel = channel % RADIO_DATA_CHANNEL_COUNT;

        channel = RADIO_BASE_CHANNEL_OFFSETHI + (channel * RADIO_DATA_CHANNEL_INCREMENT);

        radio_writereg( 0x06, (uint8_t)((channel >> 8) & 0xFF) );
        radio_writereg( 0x07, (uint8_t)(channel & 0xFF) );

        // Write payload length
        radio_writereg( 0x22, RANGE_PACKET_SIZE );

        // Reset fifo address pointer
        radio_writereg( 0x0D, 0x80 );

	// Set DIO mapping
	radio_writereg( 0x40, 0x40 );

        // Clear interrupts
        radio_writereg( 0x12, 0xFF );

	// WRITE FIFO
        radio_writefifo( g_responsePacket, RANGE_PACKET_SIZE );

	// SEND (TX)
        radio_writereg( 0x01, 0x83 );

	// DEBUG
	putchar( 'V' );
}

void radio_receive( void )
{
	// Receive sync
	uint16_t channel;

    channel = 20;

	channel = channel % RADIO_DATA_CHANNEL_COUNT;

	channel = RADIO_BASE_CHANNEL_OFFSETHI + (channel * RADIO_DATA_CHANNEL_INCREMENT);

        radio_writereg( 0x06, (uint8_t)((channel >> 8) & 0xFF) );
        radio_writereg( 0x07, (uint8_t)(channel & 0xFF) );

	// Set preamble length window to maximum timeout
	radio_writereg( 0x1E, 0x64 );
	radio_writereg( 0x1F, 0x80 );

        // Write payload length
        radio_writereg( 0x22, RANGE_PACKET_SIZE );

        // Reset fifo address pointer
        radio_writereg( 0x0D, 0x00 );
	
	// Set DIO mapping
	radio_writereg( 0x40, 0x00 );

	// Clear interrupts
	radio_writereg( 0x12, 0xFF );

	// Goto RXSINGLE
	radio_writereg( 0x01, 0x86 );
}

wakeup interrupt (TIMERA0_VECTOR) TimerA_Interrupt( void )
{
	g_responsePacket[3] = (uint8_t)(g_packetNumber >> 8);
	g_responsePacket[4] = (uint8_t)(g_packetNumber & 0xFF);

	g_radioMode = RADIO_MODE_TX;
	radio_send();

	g_packetNumber++;
}
