#ifndef __RADIO_PROCESS_H__
#define __RADIO_PROCESS_H__

#include <stdio.h>

// RADIO TIMINGS
// The RADIO_TACCR_TIME is used to set the timesync of the
// server/client -- we chose 34300 because it is equal to
// 17.15ms at 16Mhz with a timer divider of 8
// this results in about 299.439 seconds per round at 17460
// ticks per round
#define RADIO_TACCR_TIME                (34300)

#define RADIO_TACCR_SYNC_TIME           (25300)


void radio_init( void );
void radio_run( void );
void radio_state_off( void );
void radio_state_on( void );
void radio_findsync( void );
void radio_checksync( void );
void radio_receive_vip( void );
uint8_t is_radio_synced( void );
uint8_t is_radio_on( void );
uint16_t get_radio_position( void );
void radio_process_vip_packet( void );

typedef struct SYNC_PACKET_STRUCT
{
	uint8_t sync_hdr;
	uint8_t sync_extra_data_len;
	uint16_t current_round;
	uint16_t current_position;
	uint8_t sync_extra_data[26];
} tSyncPacket;


#define SYNC_PACKET_SIZE		32
#define SYNC_MAGIC_HEADER		0xA7

#define DATA_PACKET_SIZE		88
#define DATA_PACKET_DATA_SIZE		86

#define VIP_PACKET_HEADER		0x3E


typedef struct DATA_PACKET_STRUCT
{
	uint8_t packet_hdr;
	uint8_t packet_ts;		// Packed version of round
	uint8_t packet_data[DATA_PACKET_DATA_SIZE];
} tDataPacket;

#endif // __RADIO_PROCESS_H__
