#include "radio_process.h"
#include "omsp_radio.h"
#include "hardware.h"
#include "radio_hal.h"
#include "radio_keys.h"
#include "radio_packet.h"
#include "xtea_encrypt.h"
#include "util.h"
#include "app_uart.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RADIOSTATE_OFF		0
#define RADIOSTATE_FINDSYNC	1
#define RADIOSTATE_SYNC		2

#define RADIOPACKET_NONE		0
#define RADIOPACKET_FINDSYNC		1
#define RADIOPACKET_CHECKSYNC		2
#define RADIOPACKET_RX_VIP		3

#define PACKET_RX_NONE			0
#define PACKET_RX_NEW_VIP		1

#define RADIO_DIO0_BIT		0x08
#define RADIO_DIO1_BIT		0x10

#define SYNC_PACKET_LOSS_RESYNC	(40)

// DEBUG PRINT CODES (uncomment this line below to enable)
#define RADIO_DEBUG_ON	1

// Track the number of sync packets we have lost....
volatile uint8_t g_lostSyncPackets;

volatile uint8_t g_radioState;
volatile uint16_t g_timeCurrentRound;
volatile uint16_t g_timeCurrentPosition;

// Radio RX/TX packet information (slot and position)
// --------------------------------------------
volatile uint8_t g_radioPacketType;
//---------------------------------------------

// Informs the main thread there is a new VIP packet to process
volatile uint8_t g_receivePacketStatus;

// VIP Packet Holder
volatile uint8_t g_radioVipPacket[DATA_PACKET_SIZE];

// Contains the last good VIP packet (only packet data)
volatile uint8_t g_lastGoodVipPacket[DATA_PACKET_DATA_SIZE];

//----------------------------
// Used to maintain a sync delta. We average the last 10 sync packets 
// continuously to center our sync to be 2ms before the sync packet
#define SYNC_MAX_TIME_ADJUST	(20)		// We shouldn't drift more than this amount in 10 sync packets
#define SYNC_AVG_TIME_COUNT	(10)		// How many packets to average (tweak this)
volatile uint32_t g_radioAvgSyncTime;
volatile uint8_t g_radioSyncPacketCur;

#define putc_fast( c ) (UART_TXD = c)

uint8_t is_radio_on( void )
{
	return !(g_radioState == RADIOSTATE_OFF);
}

uint8_t is_radio_synced( void )
{
	return (g_radioState == RADIOSTATE_SYNC);
}

uint16_t get_radio_position( void )
{
	return (g_timeCurrentPosition);
}

interrupt (PORT1_VECTOR) INT_button( void )
{
	dint();

	P1IFG = 0;
	if ( P1IN & RADIO_DIO0_BIT )
	{
		if ( g_radioState == RADIOSTATE_FINDSYNC )
		{
			// Possibly received a sync message!
			// Check CRC
			if ( !(radio_readreg( 0x12 ) & 0x20) )
			{
				// CRC Check valid
				tSyncPacket *pSyncPacket;
				uint8_t packetData[SYNC_PACKET_SIZE];

				// Set to base address
				radio_writereg( 0x0D, 0x00 );

				// Read packet data
				radio_readfifo( packetData, SYNC_PACKET_SIZE );
				
				// Decrypt!
				xtea_decrypt( (uint32_t*)packetData, (uint32_t*)packetData, radio_sync_key );
	
				// VALIDATE!
				pSyncPacket = (tSyncPacket*)packetData;

				if ( pSyncPacket->sync_hdr == SYNC_MAGIC_HEADER )
				{
					// Sync'ed!	
					g_timeCurrentRound = pSyncPacket->current_round;
					g_timeCurrentPosition = pSyncPacket->current_position;

					g_radioState = RADIOSTATE_SYNC;	
					g_receivePacketStatus = PACKET_RX_NONE;

					// Set timer to go off in 2ms
					TAR=(RADIO_TACCR_SYNC_TIME);

					// Reset the sync packet time averaging algorithm
					g_radioSyncPacketCur = 0;
					g_radioAvgSyncTime = 0;

					// Reset last good VIP packet
					memset( (void *)g_lastGoodVipPacket, 0, DATA_PACKET_DATA_SIZE );

					// Reset the lost sync packet counter
					g_lostSyncPackets = 0;
				}
			}
			else
			{
			}
		}
		else if ( g_radioState == RADIOSTATE_SYNC )
		{
			// Parse for the different radio packets
			if ( g_radioPacketType == RADIOPACKET_CHECKSYNC )
			{
				if ( !(radio_readreg( 0x12 ) & 0x20) )
				{
					tSyncPacket *pSyncPacket;
					uint8_t packetData[SYNC_PACKET_SIZE];

					// Set to base address
					radio_writereg( 0x0D, 0x00 );

					// Read packet data
					radio_readfifo( packetData, SYNC_PACKET_SIZE );
				
					// Decrypt!
					xtea_decrypt( (uint32_t*)packetData, (uint32_t*)packetData, radio_sync_key );
					// VALIDATE!
					pSyncPacket = (tSyncPacket*)packetData;
				
					if ( pSyncPacket->sync_hdr == SYNC_MAGIC_HEADER )
					{
						// VALID!
						if ( g_timeCurrentRound != pSyncPacket->current_round || g_timeCurrentPosition != pSyncPacket->current_position )
						{
							// OUT OF SYNC
							g_lostSyncPackets++;
#ifdef RADIO_DEBUG_ON
							putc_fast('E');
#endif
						}
						else
						{
							uint16_t cur_time = TAR;

							// IN SYNC
							g_lostSyncPackets = 0;

							// Adjust sync
							if ( g_radioSyncPacketCur < SYNC_AVG_TIME_COUNT )
							{
								g_radioSyncPacketCur++;
								g_radioAvgSyncTime += cur_time;
							}
							else
							{
								uint16_t avgTime = (uint16_t)(g_radioAvgSyncTime / SYNC_AVG_TIME_COUNT);
								uint16_t delta;

								if ( avgTime < RADIO_TACCR_SYNC_TIME )
								{
									delta = RADIO_TACCR_SYNC_TIME - avgTime;

									if ( delta > SYNC_MAX_TIME_ADJUST )
										delta = SYNC_MAX_TIME_ADJUST;

									TAR += delta;
								}
								else if ( avgTime > RADIO_TACCR_SYNC_TIME )
								{
									delta = avgTime - RADIO_TACCR_SYNC_TIME;

									if ( delta > SYNC_MAX_TIME_ADJUST )
										delta = SYNC_MAX_TIME_ADJUST;

									TAR -= delta;
								}

								g_radioSyncPacketCur = 0;
								g_radioAvgSyncTime = 0;
							}

#ifdef RADIO_DEBUG_ON
							printf( "*%u", cur_time );
#endif

							// IN SYNC
						}
					}
					else
					{
						// BAD HEADER
						g_lostSyncPackets++;
					}
				}
				else
				{
#ifdef RADIO_DEBUG_ON
					putc_fast('G');
					printf( "%u", TAR );
#endif
					// BAD CRC
					g_lostSyncPackets++;
					
				}
			}
			else if ( g_radioPacketType == RADIOPACKET_RX_VIP )
			{
				if ( !(radio_readreg( 0x12 ) & 0x20) )
				{
__attribute__((aligned(2)))		uint8_t tempPacket[DATA_PACKET_SIZE];

					// VALID CRC
#ifdef RADIO_DEBUG_ON
					putc_fast('H');
#endif

					// Set to base address
					radio_writereg( 0x0D, 0x00 );

					// Process receive data 
					// Read packet data
					radio_readfifo( tempPacket, DATA_PACKET_SIZE );
			
					// DECRYPT JUST THE FIRST 8-bytes	
					memcpy( (void *)g_radioVipPacket, tempPacket, DATA_PACKET_SIZE );

					// Inform main loop to process packet
					g_receivePacketStatus = PACKET_RX_NEW_VIP;
				}
				else
				{
					// TODO: Invalid CRC
					// putchar( '>' );
				}
			}
			else
			{
			}
		}
		else
		{
		}
		
		// Clear interrupts
		radio_writereg( 0x12, 0xFF );
	}
	else if ( P1IN & RADIO_DIO1_BIT )
	{
		if ( g_radioPacketType == RADIOPACKET_CHECKSYNC )
		{
			g_lostSyncPackets++;
#ifdef RADIO_DEBUG_ON
			putc_fast( 'X' );
			//printf( "%u", TAR );
#endif
		}
		else
		{
#ifdef RADIO_DEBUG_ON
			putc_fast('^');
#endif
		}
		
		// Clear interrupts
		radio_writereg( 0x12, 0xFF );
	}

	eint();
	
	LPM0_EXIT;
}

interrupt (TIMERA0_VECTOR) TimerA_Interrupt( void )
{
	dint();
	// RUN RADIO -- CODE HERE is VERY TIME SENSITIVE
	// WE MUST COMPLETE in UNDER 15ms ALWAYS or ELSE time will skew
	g_timeCurrentPosition++;
	if ( g_timeCurrentPosition >= 17460 )
	{
		// NEW ROUND
		g_timeCurrentRound++;
		g_timeCurrentPosition = 0;

	}

	if ( g_radioState != RADIOSTATE_SYNC )
		return;

	// Where are we?
	if ( g_timeCurrentPosition < 6041 )
	{
		uint16_t current_position = g_timeCurrentPosition % 863;

		if ( current_position < 861 )
		{
			if ( (current_position%41) == 40 )
				radio_checksync( );
		}
		else if ( current_position == 861 )
			radio_receive_vip();
	}
	else if ( g_timeCurrentPosition < 12082 )
	{
		uint16_t current_position = g_timeCurrentPosition % 863;

		if ( current_position < 861 )
		{
			if ( (current_position%41) == 40 )
				radio_checksync( );
		}
		else if ( current_position == 861 )
			radio_receive_vip();
	}
	else if ( g_timeCurrentPosition < 12182 )
	{
		uint16_t mod_value = g_timeCurrentPosition % 10;

		if ( mod_value == 7 )
			radio_checksync();
		else if ( mod_value == 8 )
			radio_receive_vip();
	}
	else if ( g_timeCurrentPosition < 14771 )
	{
		uint16_t current_position = (g_timeCurrentPosition - 12182) % 863;

		if ( current_position < 861 )
		{
			if ( (current_position%41) == 40 )
				radio_checksync( );
		}
		else if ( current_position == 861 )
			radio_receive_vip();
	}
	else if ( g_timeCurrentPosition < 17360 )
	{
		uint16_t current_position = (g_timeCurrentPosition - 14771) % 863;

		if ( current_position < 861 )
		{
			if ( (current_position%41) == 40 )
				radio_checksync( );
		}
		else if ( current_position == 861 )
			radio_receive_vip();
	}
	else
	{
		uint16_t mod_value = g_timeCurrentPosition % 10;

		if ( mod_value == 7 )
			radio_checksync();
		else if ( mod_value == 8 )
			radio_receive_vip();
	}
	eint();
}

void radio_init( void )
{
	g_radioState = RADIOSTATE_OFF;
	g_receivePacketStatus = PACKET_RX_NONE;

	// Setup radio
	radio_reset();
	radio_spi_init();

	// Enter sleep mode
	radio_writereg( 0x01, 0x00 );

	// Enter LORA mode and SLEEP
	radio_writereg( 0x01, 0x80 );

	while ( radio_readreg( 0x01 ) != 0x80 )
	{
		puts("CRITICAL ERROR: Radio MODE Fail" );
	}

	// LORA standby mode
	radio_writereg( 0x01, 0x81 );

	// Set the PA configuration to the PA_BOOST
	radio_writereg( 0x09, 0x80 );	// PA_BOOST

	radio_writereg( 0x0A, 0x09 );

	// Setup modem parameters
	radio_writereg( 0x1D, 0x96 );	// SF=6, BW=500, CR=4/6, Implicit header and CRC enable
	radio_writereg( 0x1E, 0x64 );

	// Symbol timeout
	radio_writereg( 0x1F, RADIO_PREAMBLE_RX_TIMEOUT );

	// Preamble length
	radio_writereg( 0x21, RADIO_PROGRAMMED_PREAMBLE_LEN );

	// Setup payload length (for control channel)
	radio_writereg( 0x22, SYNC_PACKET_SIZE );
	radio_writereg( 0x23, SYNC_PACKET_SIZE );

	// FROM DATASHEET: write register 0x37 to 0x0C
	radio_writereg( 0x37, 0x0C );
	radio_writereg( 0x31, 0x05 );

	// Initial radio setup complete.
	P1DIR = 0x00;
	P1IE = (RADIO_DIO0_BIT | RADIO_DIO1_BIT | MODE_SELECT_RUN_PIN | MODE_SELECT_PROG_PIN | MODE_SELECT_DBG_PIN);		// DIO0 and DIO1
}

void radio_checksync( void )
{
	// Follow the channel
	uint16_t channel = ((g_timeCurrentRound << 4) + g_timeCurrentPosition) ^ radio_sync_spread_key;

	channel = channel % RADIO_TIME_CHANNEL_COUNT;

	channel = RADIO_BASE_CHANNEL_OFFSETHI + (channel * RADIO_TIME_CHANNEL_INCREMENT);

	radio_writereg( 0x06, (uint8_t)((channel >> 8) & 0xFF) );
	radio_writereg( 0x07, (uint8_t)(channel & 0xFF) );

	// Ready Frequency Synthesizer (RX)
	radio_writereg( 0x01, 0x84 );

	// Set preamble length window to the preamble timeout
	radio_writereg( 0x1E, 0x64 );
	radio_writereg( 0x1F, RADIO_PREAMBLE_RX_TIMEOUT );

	// Set payload length	
	radio_writereg( 0x22, SYNC_PACKET_SIZE );

	// Reset fifo address pointer
	radio_writereg( 0x0D, 0x00 );

	// Clear interrupts
	radio_writereg( 0x12, 0xFF );

	// Goto RXSINGLE
	radio_writereg( 0x01, 0x86 );

	// Set radio packet type
	g_radioPacketType = RADIOPACKET_CHECKSYNC;

#ifdef RADIO_DEBUG_ON
	putc_fast('#');
#endif
}

void radio_findsync( void )
{
	// Select random channel (use TAR to as random seed)
	uint16_t channel = (TAR);
	//uint16_t channel = 0;
	channel = channel % RADIO_TIME_CHANNEL_COUNT;

	channel = RADIO_BASE_CHANNEL_OFFSETHI + (channel * RADIO_TIME_CHANNEL_INCREMENT);

	radio_writereg( 0x06, (uint8_t)((channel >> 8) & 0xFF) );
	radio_writereg( 0x07, (uint8_t)(channel & 0xFF) );

	// Ready Frequency Synthesizer
	radio_writereg( 0x01, 0x84 );

	// Set preamble length window to maximum
	// 1023 symbols or roughly 130 milliseconds
	radio_writereg( 0x1E, 0x67 );
	radio_writereg( 0x1F, 0xFF );

	// Set payload length	
	radio_writereg( 0x22, SYNC_PACKET_SIZE );
	
	// Reset fifo address pointer
	radio_writereg( 0x0D, 0x00 );

	// Clear interrupts
	radio_writereg( 0x12, 0xFF );
	
	// Goto RXSINGLE
	radio_writereg( 0x01, 0x86 );

	// Goto findsync
	g_radioState = RADIOSTATE_FINDSYNC;

	g_radioPacketType = RADIOPACKET_FINDSYNC;

#ifdef RADIO_DEBUG_ON
	putc_fast('F');
#endif
}

//==============================================//
// radio_run_findsync
//	This function will dwell on a random
// channel and wait for a sync packet
// for a maximum of two seconds. If it finds sync
// it will program the timerA interrupt to run
//==============================================//
void radio_run( void )
{
        // Enable this simple check here to turn off the radio when not in RUN mode
        // MAKE THE CHECK so that if EITHER PROG or DBG is HIGH we are OFF
        // or else they may just use a little hardware trick to defeat us       
        // if they try to fuck with us -- ie disconnect RUN, don't go back into RADIOSTATE_ON =p
// #if RADIO_DEBUG_ON
	if ( g_radioState == RADIOSTATE_OFF )
		g_radioState = RADIOSTATE_FINDSYNC;
/*
#else
        if ( MODE_SELECT_PIN & (MODE_SELECT_DBG_PIN | MODE_SELECT_PROG_PIN) )
                g_radioState = RADIOSTATE_OFF;
        else if ( (MODE_SELECT_PIN & MODE_SELECT_RUN_PIN) && g_radioState == RADIOSTATE_OFF )
                g_radioState = RADIOSTATE_FINDSYNC;
#endif
*/

	// NOTE: The interrupts advance state from findsync and sync
	switch( g_radioState )
	{
	case RADIOSTATE_OFF:
		// Do nothing
		break;

	case RADIOSTATE_FINDSYNC:
		// Run sync
		radio_findsync();
		break;

	case RADIOSTATE_SYNC:
		if ( g_receivePacketStatus == PACKET_RX_NEW_VIP )
			radio_process_vip_packet();

		if ( g_lostSyncPackets >= SYNC_PACKET_LOSS_RESYNC )
			radio_state_on(); // RESYNC
	
		// Do nothing
		break;
	}
}

void radio_state_off( void )
{
	g_radioState = RADIOSTATE_OFF;
}

void radio_state_on( void )
{
	// Force the radio to resync (this will reset everything)
	g_radioState = RADIOSTATE_FINDSYNC;
}

void radio_receive_vip( void )
{
	// RECEIVE on this time slot
	// SEND on this time slot
	uint16_t channel;

	channel = ((g_timeCurrentRound << 4) + g_timeCurrentPosition) ^ radio_vip_spread_key;
	
	channel = channel % RADIO_DATA_CHANNEL_COUNT;
	
	channel = RADIO_BASE_CHANNEL_OFFSETHI + (channel * RADIO_DATA_CHANNEL_INCREMENT);

	radio_writereg( 0x06, (uint8_t)((channel >> 8) & 0xFF) );
	radio_writereg( 0x07, (uint8_t)(channel & 0xFF) );
	
	// Ready Frequency Synthesizer
	radio_writereg( 0x01, 0x84 );

	// Set preamble length window to the preamble timeout
	radio_writereg( 0x1E, 0x64 );
	radio_writereg( 0x1F, RADIO_PREAMBLE_RX_TIMEOUT );
	
	// Set payload length	
	radio_writereg( 0x22, DATA_PACKET_SIZE );

	// Reset fifo address pointer
	radio_writereg( 0x0D, 0x00 );

	// Clear interrupts
	radio_writereg( 0x12, 0xFF );

	// Goto RXSINGLE
	radio_writereg( 0x01, 0x86 );

	// Radio Packet RX DATA
	g_radioPacketType = RADIOPACKET_RX_VIP;		
}

void radio_process_vip_packet( void )
{
__attribute__((aligned(2))) uint8_t vipPacket[DATA_PACKET_SIZE];
	uint8_t i;

	g_receivePacketStatus = PACKET_RX_NONE;

	// Begin decryption
	memcpy( vipPacket, (void *)g_radioVipPacket, DATA_PACKET_SIZE );	

	// Decrypt
	for ( i = 0; i < DATA_PACKET_SIZE; i+=8 )
		xtea_decrypt( (uint32_t*)(vipPacket+i), (uint32_t*)(vipPacket+i), radio_vip_key );

	if ( vipPacket[0] != VIP_PACKET_HEADER )
	{
		// Fail decryption
#ifdef RADIO_DEBUG_ON
		printf( "BAD VIP PACKET -- DECRYPTION FAIL\n" );
#endif
	
		return;
	}

	if ( memcmp( vipPacket+2, (void *)g_lastGoodVipPacket, DATA_PACKET_DATA_SIZE ) != 0 )
	{
		uint8_t pos;
		uint8_t checksum;
		uint8_t hexString[8];

		// Good packet, and it is different...
		memcpy( (void *)g_lastGoodVipPacket, vipPacket+2, DATA_PACKET_DATA_SIZE );

		// Now write it out to the UART
		app_uart_write( (uint8_t *)"VP:", 3 );
		
		checksum = 0;
		for ( pos = 2; pos < (DATA_PACKET_DATA_SIZE+2); pos++ )
		{
			hexString[0] = int_to_hex_char( (vipPacket[pos] >> 4) & 0xF );
			hexString[1] = int_to_hex_char( (vipPacket[pos] & 0xF) );

			checksum += vipPacket[pos];
			app_uart_write( hexString, 2 );
		}

		hexString[0] = ',';
		hexString[1] = int_to_hex_char( (checksum >> 4) & 0xF );
		hexString[2] = int_to_hex_char( (checksum & 0xF) );
		hexString[3] = '\n';

		app_uart_write( hexString, 4 );

		// DEBUG	
#ifdef RADIO_DEBUG_ON
		printf( "VIP:" );	
		for ( pos = 2; pos < DATA_PACKET_DATA_SIZE+2; pos++ )
		{
			printf( "%02x", vipPacket[pos] );	
		}
		printf( ",%02x\r\n", checksum );
#endif
	}
}
