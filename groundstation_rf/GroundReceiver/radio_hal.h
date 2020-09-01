#ifndef __RADIO_HAL_H__
#define __RADIO_HAL_H__

#include <stdint.h>
#include <string>

/*
#include "sdkconfig.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/event_groups.h"
#include "esp_sleep.h"
#include "esp32-hal-psram.h"
#include "esp_himem.h"
*/

#include <SPI.h>
#include "driver/timer.h"

#include "usb_uart.h"

#include "GroundReceiver.h"

extern "C" {
#include "radio_keys.h"
#include "util.h"
}


#define MAX_SEND_BUFFER_LENGTH      (2048)  // Send buffers (TX/RX) for OOBM and Leon-3 Data over the radio interface

#define DLOSS_FIFO_LEN              64

// Timeout of search for preamble (preamble window counter)
#define RADIO_PREAMBLE_RX_TIMEOUT       40

#define RADIOSTATE_OFF              (0)
#define RADIOSTATE_READPACKET       (1)   // Sending packets
#define RADIOSTATE_WRITEPACKET      (2)   // Reading packets

#define PACKET_DATA_TYPE_LEON3           (0)   // Leon-3 data packet type
#define PACKET_DATA_TYPE_OOBM            (1)   // OOBM (Out of Band Management) data packet type

#define OUTTER_PACKET_SIZE              (64)
#define OUTTER_PACKET_HEADER_SIZE       (2)

#define SF_HEADER_SIZE                  (2)
#define SF_PACKET_SIZE                  (62)

#define DATA_PACKET_SIZE                (60)
#define DATA_PACKET_DATA_SIZE           (59)
#define DATA_PACKET_HEADER_SIZE         (1)

#define DATA_PACKET_MAGIC_MASK          (0xFF)
#define DATA_PACKET_MAGIC               0x38

#define PACKET_NUM_MASK                 (0xC0)
#define PACKET_INNER_FRAME_MASK         (0x3F)


#define PACKET_FRAME_MASK               (0x7FFF)

#define PACKET_TYPE_MASK                (0x80)
#define PACKET_LEN_MASK                 (0x3F)          // Packets are always 64-bytes of data but the data may be truncated

#define QUEUE_PACKET_TYPE_EMPTY         0       // Indicates no packets received yet
#define QUEUE_PACKET_TYPE_DATA          1       // Indicates data packet received
#define QUEUE_PACKET_TYPE_DONE          2       // Indicates done processing


// PACKET Header Format
// [ MAGIC ] 1-byte -- Magic number
// [ 2-bits -- correction position ][ 6-bits inner correction frame # ] -- 1-byte, high order 2-bits indicate the correction position (packet 0, packet 1, packet 2, xor packet) and the 6-bits indicate the length of the packet data
// [ 3-bit superframe #, 13-bits frame number ] -- 2-bytes, high-order 3-bits for superframe index, and 13-bits for the super frame number
// [1-bit packet type][1-bit reserved][6-bits packet length]

#define PACKET_QUEUE_SIZE               (16)
typedef struct RADIO_SEND_QUEUE
{
__attribute__((aligned(2)))     uint8_t         packet_data[PACKET_QUEUE_SIZE][OUTTER_PACKET_SIZE];
} tSendQueue;

typedef struct RADIO_RECEIVE_QUEUE
{
__attribute__((aligned(2)))     uint8_t         packet_data[PACKET_QUEUE_SIZE][OUTTER_PACKET_SIZE];
} tReceiveQueue;

// -----------------
//
#define SF_QUEUE_SIZE           (16)

typedef struct SF_PACKET_BLOCK
{
        __attribute__((aligned(2)))     uint8_t         packet_data[3][SF_PACKET_SIZE-SF_HEADER_SIZE];
} tSFPacketBlock;

typedef struct RADIO_SF_QUEUE_DATA
{
        uint8_t sfNumber;
        uint16_t frameNumber;
        tSFPacketBlock  block;
} tSFQueueData;

typedef struct RADIO_SEND_XOR_DATA
{
        uint16_t innerXORData[SF_PACKET_SIZE];
        uint16_t sfXORData[3][DATA_PACKET_SIZE];
} tSendXORData;


// registers
#define REG_FIFO                 0x00
#define REG_OP_MODE              0x01
#define REG_FRF_MSB              0x06
#define REG_FRF_MID              0x07
#define REG_FRF_LSB              0x08
#define REG_PA_CONFIG            0x09
#define REG_LR_OCP               0X0b
#define REG_LNA                  0x0c
#define REG_FIFO_ADDR_PTR        0x0d
#define REG_FIFO_TX_BASE_ADDR    0x0e
#define REG_FIFO_RX_BASE_ADDR    0x0f
#define REG_FIFO_RX_CURRENT_ADDR 0x10
#define REG_IRQ_FLAGS            0x12
#define REG_RX_NB_BYTES          0x13
#define REG_PKT_RSSI_VALUE       0x1a
#define REG_PKT_SNR_VALUE        0x1b
#define REG_MODEM_CONFIG_1       0x1d
#define REG_MODEM_CONFIG_2       0x1e
#define REG_PREAMBLE_MSB         0x20
#define REG_PREAMBLE_LSB         0x21
#define REG_PAYLOAD_LENGTH       0x22
#define REG_MODEM_CONFIG_3       0x26
#define REG_RSSI_WIDEBAND        0x2c
#define REG_DETECTION_OPTIMIZE   0x31
#define REG_DETECTION_THRESHOLD  0x37
#define REG_SYNC_WORD            0x39
#define REG_DIO_MAPPING_1        0x40
#define REG_VERSION              0x42
#define REG_PaDac        0x4d//add REG_PaDac

#define REG_BITRATE_MSB         0x02
#define REG_BITRATE_LSB         0x03

#define REG_FDEV_MSB            0x04
#define REG_FDEV_LSB            0x05

// modes
#define MODE_LONG_RANGE_MODE     0x80
#define MODE_SLEEP               0x00
#define MODE_STDBY               0x01
#define MODE_TX                  0x03
#define MODE_RX_CONTINUOUS       0x05
#define MODE_RX_SINGLE           0x06


#define PA_CONFIG_VALUE                 (0x80)  // +3dBm (any higher will saturate front-end of badges that are nearby)

// Programmed preamble length
#define RADIO_PROGRAMMED_PREAMBLE_LEN   3

// Timeout of search for preamble (preamble window counter)
#define RADIO_PREAMBLE_RX_TIMEOUT       40      

// Radio channel select (high 16-bits)
#define RADIO_BASE_CHANNEL_OFFSETHI     0xE1B0  // 902.75MHz
#define RADIO_MIDDLE_CHANNEL_OFFSETHI   0xE4C0  // 915MHz
#define RADIO_CHANNEL_MAX               0xE7F0  // 927.75Mhz

#define RADIO_CHANNEL_INCREMENT 0x00C0  // 1MHz
#define RADIO_CHANNEL_COUNT     ((RADIO_CHANNEL_MAX-RADIO_BASE_CHANNEL_OFFSETHI)/RADIO_CHANNEL_INCREMENT)       // (MAX-MIN)/INC = 25 channels


#define DIO0_PIN   (26)
#define DIO1_PIN   (11)
//#define DIO2_PIN   (8)
#define RADIO_SS    (18)
#define RADIO_RESET   (14)   // DO NOT USE -- IDIOTS AT SPARKFUN DID NOT WIRE IT UP

#define SPISPEED    (10000000)

extern volatile tSendQueue g_sendQueue;
extern volatile tReceiveQueue g_receiveQueue;

extern tSFQueueData g_receiveSFQueue[SF_QUEUE_SIZE];

extern volatile uint8_t g_receivePacketPos    ;        // Current position of the the next packet to add from reception
extern volatile uint8_t g_receiveProcessPos;       // Current position of the packet to process
extern volatile uint8_t g_receiveProcessCount;

extern volatile uint8_t g_sendPacketPos;           // Current position of the next packet to transmit
extern volatile uint8_t g_sendProcessPos;          // Current position of the next packet to add to the send queue
extern volatile uint8_t g_sendProcessCount;        // Current count of packets in the queue

extern volatile uint8_t g_receivePacketMissCount;
extern volatile uint16_t g_timerCounter;           // Used to alternate betweeen RX/TX

extern volatile uint16_t g_startPacketTime;
extern volatile uint16_t g_endPacketTime;


extern volatile uint8_t g_receiveSFPacketPos;      // Current incoming superframe position
extern volatile uint8_t g_receiveSFProcessPos;     // Current processing superframe position
extern volatile uint8_t g_receiveSFProcessCount;   // Current count of superframes to process

// Inner frame 
extern volatile uint8_t g_sendInnerFragmentNumber; // 0-3 (with 3 being the XOR data) 3/4 FEC (inner) 
extern volatile uint8_t g_sendInnerFrameNumber;    // 0-63

// Super frame
extern volatile uint8_t g_sendSFFragmentNumber;    // 0-6 (with 6 being the XOR data) 6/7 FEC (outter, or called the SuperFrame)
extern volatile uint16_t g_sendSFFrameNumber;      // 0-2048 (superframe number)

extern volatile uint16_t g_leon3DataSendCount;             // Count of data from Leon-3 to send
extern volatile uint16_t g_leon3DataSendReadPos;           // Position to read from to send (next to send)
extern volatile uint16_t g_leon3DataSendWritePos;          // Position to write (ready to send)

extern volatile uint16_t g_oobmDataSendCount;              // Count of data from OOBM to send
extern volatile uint16_t g_oobmDataSendReadPos;            // Position to read from to send (next to send)
extern volatile uint16_t g_oobmDataSendWritePos;           // Position to write (ready to send)

extern volatile uint8_t g_sendSelectPos;                   // Select position (send rotation)

extern volatile uint8_t g_oobmSendData[MAX_SEND_BUFFER_LENGTH];
extern volatile uint8_t g_leon3SendData[MAX_SEND_BUFFER_LENGTH];

extern portMUX_TYPE radioMux;
extern portMUX_TYPE sendOOBMDataMux;
extern uint8_t g_radioState;
extern uint32_t g_lastClockCount;

 
extern portMUX_TYPE sendLeon3DataMux;

extern float g_data_loss_pct;

extern TaskHandle_t handle_onRadioTimerTask;
extern TaskHandle_t handle_onRadioInterruptTask;

void ICACHE_RAM_ATTR Interrupt_0();
void ICACHE_RAM_ATTR Interrupt_1();

   
String getMacAddress();
void radio_init( void );
void radio_reset( void );
void radio_spi_init( void );
void radio_spi_writebyte( uint8_t data );
uint8_t radio_spi_readbyte( void );
void radio_writereg( uint8_t regNum, uint8_t value );
void radio_writefifo( uint8_t *data, unsigned int count );
void radio_readfifo( uint8_t *data, unsigned int count );
uint8_t radio_readreg( uint8_t regNum );

void write_leon3_byte( uint8_t dataIn );
void write_oobm_byte( uint8_t dataIn );
void write_leon3(uint8_t * inbuf, signed len);
void write_oobm(uint8_t * inbuf, signed len);


void radio_send_data_packet( void );
// Process asynchronously the receive queue
void radio_process_receive_queue( void );
void receive_superframe( volatile uint8_t *packet1Data, volatile uint8_t *packet2Data, volatile uint8_t *packet3Data );
void radio_process_send_queue( void );
uint8_t hex_to_int( uint8_t dataIn );
uint8_t int_to_hex( uint8_t dataIn );
uint16_t write_byte_circular_buffer( uint8_t byteOut, volatile uint8_t *pDest, uint16_t bufferPos );


void IRAM_ATTR onRadioTimer( void ) ;
void onRadioTimerTask( void* param1 );
void IRAM_ATTR Interrupt_DIO0();
void onRadioInterruptTask( void* param1 );
void IRAM_ATTR Interrupt_DIO1();
void radio_reset( void );
void radio_spi_init( void );
void radio_writereg( uint8_t regNum, uint8_t value );
uint8_t radio_readreg( uint8_t regNum );
void radio_writebyte( uint8_t value );
uint8_t radio_spi_readbyte( void );
void radio_writefifo( uint8_t *pData, uint32_t count );
void radio_readfifo( uint8_t *pData, uint32_t count );
void radio_setchannel( uint8_t channel_num );
void radio_init( void );



#endif // __RADIO_HAL_H__
