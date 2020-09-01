#ifndef __RADIO_PROCESS_H__
#define __RADIO_PROCESS_H__

#include <stdio.h>

#include "oobm.h"

// Turn this off (remove it) to disable radio debug prints
#define DEBUG_RADIO_ON          1

// Turn this on to enable the serial enable
#define APP_SERIAL_LOADER_EN	1


typedef struct {
	volatile uint8_t lock;
} t_my_mutex;

void my_mutex_lock(volatile t_my_mutex *my_mutex);
void my_mutex_unlock(volatile t_my_mutex *my_mutex);

void radio_init( void );
void radio_run( void );
void radio_state_off( void );
void radio_state_on( void );
uint8_t is_radio_synced( void );
uint8_t is_radio_on( void );
uint16_t get_radio_position( void );
void radio_send_data_packet( void );
void radio_receive_data_packet( void );
void radio_setchannel( uint8_t channel_num );

void reset_radio_queues( void );
void reset_radio_receive_queue( void );
void reset_radio_send_queue( void );

void radio_process_receive_queue( void );
void radio_process_send_queue( void );

uint8_t radio_send_leon3_data( volatile uint8_t *pData, uint16_t dataLen );
uint8_t radio_send_oobm_data( uint8_t *pData, uint16_t dataLen );

void receive_superframe( volatile uint8_t *packet1, volatile uint8_t *packet2, volatile uint8_t *packet3 );

uint8_t radio_queue_data_send( uint8_t *pData, uint8_t packetType, uint8_t packetLen );
// RADIO TIMINGS
// The RADIO_TACCR_TIME is used to set the timesync of the
// server/client -- we chose 13250 because it is equal to
// 5.3ms at 20Mhz with a timer divider of 8
// this results in about 188.67 packets per second
// #define RADIO_TACCR_TIME                (15624) // (13250) // 20 MHz
#define RADIO_TACCR_TIME		(18749) // 24 MHZ

#define PA_CONFIG_VALUE			(0x80)	// +13dBm (any higher will saturate front-end of badges that are nearby)

#define PACKET_DATA_TYPE_LEON3         (0)     // Pass to the Leon-3
#define PACKET_DATA_TYPE_OOBM          (1)     // Pass to Out of Band Management

#define OUTTER_PACKET_SIZE		(64)
#define OUTTER_PACKET_HEADER_SIZE	(2)

#define SF_HEADER_SIZE			(2)
#define SF_PACKET_SIZE			(62)

#define DATA_PACKET_SIZE		(60)
#define DATA_PACKET_DATA_SIZE		(59)
#define DATA_PACKET_HEADER_SIZE		(1)

#define DATA_PACKET_MAGIC_MASK		(0xFF)
#define DATA_PACKET_MAGIC		0x38

#define PACKET_NUM_MASK			(0xC0)
#define PACKET_INNER_FRAME_MASK		(0x3F)


#define PACKET_FRAME_MASK               (0x7FFF)

#define PACKET_TYPE_MASK                (0x80)
#define PACKET_LEN_MASK			(0x3F)		// Packets are always 64-bytes of data but the data may be truncated

#define QUEUE_PACKET_TYPE_EMPTY		0	// Indicates no packets received yet
#define QUEUE_PACKET_TYPE_DATA		1	// Indicates data packet received
#define QUEUE_PACKET_TYPE_DONE		2	// Indicates done processing

#define STATUS_LED_PACKET_RX		(0x20)
#define STATUS_LED_OOBM_RX		(0x40)

// PACKET Header Format
// [ MAGIC ] 1-byte -- Magic number
// [ 2-bits -- correction position ][ 6-bits inner correction frame # ] -- 1-byte, high order 2-bits indicate the correction position (packet 0, packet 1, packet 2, xor packet) and the 6-bits indicate the length of the packet data
// [ 3-bit superframe #, 13-bits frame number ] -- 2-bytes, high-order 3-bits for superframe index, and 13-bits for the super frame number
// [1-bit packet type][1-bit reserved][6-bits packet length]

#define PACKET_QUEUE_SIZE		(16)
typedef struct RADIO_SEND_QUEUE
{
__attribute__((aligned(2)))	uint8_t		packet_data[PACKET_QUEUE_SIZE][OUTTER_PACKET_SIZE];
} tSendQueue;

typedef struct RADIO_RECEIVE_QUEUE
{
__attribute__((aligned(2)))	uint8_t		packet_data[PACKET_QUEUE_SIZE][OUTTER_PACKET_SIZE];
} tReceiveQueue;


// -----------------
//
#define SF_QUEUE_SIZE		(16)

typedef struct SF_PACKET_BLOCK
{
	__attribute__((aligned(2)))	uint8_t		packet_data[3][SF_PACKET_SIZE-SF_HEADER_SIZE];
} tSFPacketBlock;

typedef struct RADIO_SF_QUEUE_DATA
{
	uint8_t sfNumber;
	uint16_t frameNumber;
	tSFPacketBlock	block;
} tSFQueueData;

typedef struct RADIO_SEND_XOR_DATA
{
	uint16_t innerXORData[SF_PACKET_SIZE];
	uint16_t sfXORData[3][DATA_PACKET_SIZE];
} tSendXORData;

#endif // __RADIO_PROCESS_H__
