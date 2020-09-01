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

volatile uint8_t g_responsePacket[RANGE_PACKET_SIZE];

wakeup interrupt (PORT1_VECTOR) INT_button( void )
{
	dint();
	P1IFG = 0;
	if ( P1IN & RADIO_DIO0_BIT )
	{
		if ( !(radio_readreg( 0x12 ) & 0x20 ) )
		{
			// Set base address	
			radio_writereg( 0x0D, 0x00 );
				
			// Clear interrupts
			radio_writereg( 0x12, 0xFF );
		
			// Read packet and ready it for processing!
			radio_readfifo( g_responsePacket, RANGE_PACKET_SIZE );

			// Add in rx'ed RSSI
			g_responsePacket[0] = radio_readreg( 0x1A );
			g_responsePacket[1] = 0xAC;

			radio_sendresponse();
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

void radio_sendresponse( void )
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
	radio_writereg( 0x1E, 0x67 );
	radio_writereg( 0x1F, 0xFF );

        // Write payload length
        radio_writereg( 0x22, RANGE_PACKET_SIZE );

        // Reset fifo address pointer
        radio_writereg( 0x0D, 0x00 );

	// Clear interrupts
	radio_writereg( 0x12, 0xFF );

	// Goto RXSINGLE
	radio_writereg( 0x01, 0x86 );
}
