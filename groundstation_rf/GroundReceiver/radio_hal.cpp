#include <Arduino.h>
#include "radio_hal.h"
#include "driver/timer.h"



// Send and receive Queues hold the send/receive data for forward data channel and response channel
//---------------------------------------------------------
volatile tSendQueue g_sendQueue;
volatile tReceiveQueue g_receiveQueue;

tSFQueueData g_receiveSFQueue[SF_QUEUE_SIZE];

//---------------------------------------------------------
//

volatile uint8_t g_receivePacketPos = 0;        // Current position of the the next packet to add from reception
volatile uint8_t g_receiveProcessPos = 0;       // Current position of the packet to process
volatile uint8_t g_receiveProcessCount = 0;

volatile uint8_t g_sendPacketPos = 0;           // Current position of the next packet to transmit
volatile uint8_t g_sendProcessPos = 0;          // Current position of the next packet to add to the send queue
volatile uint8_t g_sendProcessCount = 0;        // Current count of packets in the queue

volatile uint8_t g_receivePacketMissCount = 0;
volatile uint16_t g_timerCounter = 0;           // Used to alternate betweeen RX/TX

volatile uint16_t g_startPacketTime = 0;
volatile uint16_t g_endPacketTime = 0;


volatile uint8_t g_receiveSFPacketPos = 0;      // Current incoming superframe position
volatile uint8_t g_receiveSFProcessPos = 0;     // Current processing superframe position
volatile uint8_t g_receiveSFProcessCount = 0;   // Current count of superframes to process

// Inner frame 
volatile uint8_t g_sendInnerFragmentNumber = 0; // 0-3 (with 3 being the XOR data) 3/4 FEC (inner) 
volatile uint8_t g_sendInnerFrameNumber = 0;    // 0-63

// Super frame
volatile uint8_t g_sendSFFragmentNumber = 0;    // 0-6 (with 6 being the XOR data) 6/7 FEC (outter, or called the SuperFrame)
volatile uint16_t g_sendSFFrameNumber = 0;      // 0-2048 (superframe number)

volatile uint16_t g_leon3DataSendCount = 0;             // Count of data from Leon-3 to send
volatile uint16_t g_leon3DataSendReadPos = 0;           // Position to read from to send (next to send)
volatile uint16_t g_leon3DataSendWritePos = 0;          // Position to write (ready to send)

volatile uint16_t g_oobmDataSendCount = 0;              // Count of data from OOBM to send
volatile uint16_t g_oobmDataSendReadPos = 0;            // Position to read from to send (next to send)
volatile uint16_t g_oobmDataSendWritePos = 0;           // Position to write (ready to send)

volatile uint8_t g_sendSelectPos = 0;                   // Select position (send rotation)

volatile uint8_t g_oobmSendData[MAX_SEND_BUFFER_LENGTH];
volatile uint8_t g_leon3SendData[MAX_SEND_BUFFER_LENGTH];


tSendXORData g_sendXORData;

int bad_ct = 0;
int total_ct = 0;
int loss_fifo[16];
int loss_fifo_len=0;
float g_data_loss_pct=NAN;


uint8_t g_radioState = RADIOSTATE_OFF;

uint32_t g_lastClockCount = 0;
 
portMUX_TYPE radioMux = portMUX_INITIALIZER_UNLOCKED;
portMUX_TYPE sendOOBMDataMux = portMUX_INITIALIZER_UNLOCKED;
portMUX_TYPE sendLeon3DataMux = portMUX_INITIALIZER_UNLOCKED;


TaskHandle_t handle_onRadioTimerTask;
TaskHandle_t handle_onRadioInterruptTask;

void ICACHE_RAM_ATTR Interrupt_0();
void ICACHE_RAM_ATTR Interrupt_1();

String getMacAddress() 
{
  uint8_t baseMac[6];
  // Get MAC address for WiFi station
  esp_read_mac(baseMac, ESP_MAC_WIFI_STA);
  char baseMacChr[18] = {0};
  sprintf(baseMacChr, "%02X:%02X:%02X:%02X:%02X:%02X", baseMac[0], baseMac[1], baseMac[2], baseMac[3], baseMac[4], baseMac[5]);
  return String(baseMacChr);
}

void getMacAddressAndTeamID(char * buf) 
{
  uint8_t baseMac[6];
  // Get MAC address for WiFi station
  esp_read_mac(baseMac, ESP_MAC_WIFI_STA);
  sprintf(buf, "TEAM %d MAC %02X:%02X:%02X:%02X:%02X:%02X\n", 
    MY_TEAM_ID,baseMac[0], 
    baseMac[1], baseMac[2], 
    baseMac[3], baseMac[4], 
    baseMac[5]);
}

void radio_send_data_packet( void )
{
  uint8_t i;
  uint32_t curClockCount;
  char buffer[128];

  g_lastClockCount = ESP.getCycleCount(); //xthal_get_ccount();
  
  // Enter radio MUX
  portENTER_CRITICAL(&radioMux);
    
  if ( g_sendProcessCount > 0 )
  {
    // SEND (TX)
       
    radio_writereg( REG_OP_MODE, 0x03 );   
    
    // If data is in the queue -- send it
    volatile uint8_t *pPacketData = g_sendQueue.packet_data[g_sendPacketPos];

#if 0   // Packet Corruption Test Code
    // Randomly corrupt 1 in 4 packets for testing
    if ( random( 16 ) == 0 )
      pPacketData[0] = (uint8_t)random( 256 );
#endif
    
    // WRITE FIFO
    radio_writefifo( (uint8_t*)pPacketData, OUTTER_PACKET_SIZE );
    
    g_sendPacketPos++;
    
    if ( g_sendPacketPos >= PACKET_QUEUE_SIZE )
            g_sendPacketPos = 0;
    
    g_sendProcessCount--;
  }
  else
  {
    // BUG if you get here
    Serial.print('Q');
  }

  // Exit radio mux
  portEXIT_CRITICAL(&radioMux);
  
}

// Process asynchronously the receive queue
void radio_process_receive_queue( void )
{
        if ( g_receiveProcessCount > 4 )
        {
                volatile uint8_t *pFramePacketData[3];
                uint16_t xorData[OUTTER_PACKET_SIZE/2];

                uint8_t i, j;
                uint8_t packet_idx[6][2];
                uint16_t packet_frame_num;
                uint8_t packet_frame_count;

                // We have at least 3 packets in the receive queue -- process the block of four for correction
                // Process block of 4
                j = g_receiveProcessPos;
                for ( i = 0; i < 4; i++ )
                {
                        uint8_t correction_inner_pos = (g_receiveQueue.packet_data[j][1] & PACKET_NUM_MASK) >> 6;
                        uint8_t current_frame_num = (g_receiveQueue.packet_data[j][1] & PACKET_INNER_FRAME_MASK);

                        // printf( "P:%d,%d\n", correction_inner_pos, current_frame_num );

                        packet_idx[i][0] = correction_inner_pos; // Store header
                        packet_idx[i][1] = j;                    // Store index

                        if ( i == 0 )
                                packet_frame_num = current_frame_num;
                        else
                        {
                                if ( current_frame_num != packet_frame_num )
                                        break;
                        }

                        j++;
                        if ( j >= PACKET_QUEUE_SIZE )
                                j = 0;
                }

                packet_frame_count = i;

                
                if(++total_ct==256)  // quick and dirty rolling-ish average.
                {
                  total_ct=128; // was going to do a proper rolling buffer, but I think the numbers
                  bad_ct/=2;  // are so sparse it will look weird no matter what.                                    
                }
                g_data_loss_pct = (float)bad_ct*100.0/(float)total_ct;
                
                if ( packet_frame_count < 3 )
                {
                        // Consume packets from the receive queue
                        g_receiveProcessPos += packet_frame_count;
                        if ( g_receiveProcessPos >= PACKET_QUEUE_SIZE )
                                g_receiveProcessPos -= PACKET_QUEUE_SIZE;       // Wrap

                        g_receiveProcessCount -= packet_frame_count;

                        //Serial.print('U');
                        // putc_fast('U');

                        // Skip em -- we can't correct -- we need at least 3 of the same frame number!
                        bad_ct++;
                        g_data_loss_pct = (float)bad_ct*100.0/(float)total_ct;
                        return;
                }

                // Decrypt
                for ( i = 8; i < OUTTER_PACKET_SIZE; i+= 8 )
                        xtea_decrypt( ((uint32_t*)(g_receiveQueue.packet_data[packet_idx[0][1]]+i)), radio_write_key );

                for ( i = 8; i < OUTTER_PACKET_SIZE; i+= 8 )
                        xtea_decrypt( ((uint32_t*)(g_receiveQueue.packet_data[packet_idx[1][1]]+i)), radio_write_key );

                for ( i = 8; i < OUTTER_PACKET_SIZE; i+= 8 )
                        xtea_decrypt( ((uint32_t*)(g_receiveQueue.packet_data[packet_idx[2][1]]+i)), radio_write_key );

// Prepare for corrections -- count the number of packets in this frame number
                //
                // IF COUNT==3 correct
                if ( packet_frame_count == 3 )
                {
                        uint8_t offset;
                        uint8_t packet_1, packet_2, packet_3;


                        packet_1 = packet_idx[0][1];
                        packet_2 = packet_idx[1][1];
                        packet_3 = packet_idx[2][1];

                        // Build XOR packet, skip inner frame header
                        for ( offset = 1; offset < (OUTTER_PACKET_SIZE/2); offset++ )
                                xorData[offset] = ((uint16_t*)g_receiveQueue.packet_data[packet_1])[offset] ^ ((uint16_t*)g_receiveQueue.packet_data[packet_2])[offset] ^ ((uint16_t*)g_receiveQueue.packet_data[packet_3])[offset];

                        if ( packet_idx[0][0] == 1 )
                        {
                                // Missing first packet, XOR recovered it
                                pFramePacketData[0] = (uint8_t*)xorData;

                                pFramePacketData[1] = g_receiveQueue.packet_data[packet_1];

                                pFramePacketData[2] = g_receiveQueue.packet_data[packet_2];
                        }
                        else if ( packet_idx[1][0] == 2 )
                        {
                                // Missing second packet, XOR recovered it
                                pFramePacketData[0] = g_receiveQueue.packet_data[packet_1];

                                pFramePacketData[1] = (uint8_t*)xorData;

                                pFramePacketData[2] = g_receiveQueue.packet_data[packet_2];
                        }
                        else if ( packet_idx[2][0] == 3 )
                        {
                                // Missing third packet, XOR recovered it
                                pFramePacketData[0] = g_receiveQueue.packet_data[packet_1];

                                pFramePacketData[1] = g_receiveQueue.packet_data[packet_2];

                                pFramePacketData[2] = (uint8_t*)xorData;
                        }
                        else
                        {
                                // Missing XOR packet -- ignore it
                                pFramePacketData[0] = g_receiveQueue.packet_data[packet_1];

                                pFramePacketData[1] = g_receiveQueue.packet_data[packet_2];

                                pFramePacketData[2] = g_receiveQueue.packet_data[packet_3];
                        }
                }
                else if ( packet_frame_count == 4 )
                {
                        uint8_t packet_1, packet_2, packet_3;

                        // Have all 4 packets -- ignore the XOR packet
                        packet_1 = packet_idx[0][1];
                        packet_2 = packet_idx[1][1];
                        packet_3 = packet_idx[2][1];

                        pFramePacketData[0] = g_receiveQueue.packet_data[packet_1];

                        pFramePacketData[1] = g_receiveQueue.packet_data[packet_2];

                        pFramePacketData[2] = g_receiveQueue.packet_data[packet_3];
                }

                // Receive superframe -- skipping over inner frame headers
                receive_superframe( pFramePacketData[0]+(OUTTER_PACKET_HEADER_SIZE), pFramePacketData[1]+(OUTTER_PACKET_HEADER_SIZE), pFramePacketData[2]+(OUTTER_PACKET_HEADER_SIZE) );
                
                // Consume packets from the receive queue
                g_receiveProcessPos += packet_frame_count;
                if ( g_receiveProcessPos >= PACKET_QUEUE_SIZE )
                        g_receiveProcessPos -= PACKET_QUEUE_SIZE;       // Wrap

                g_receiveProcessCount -= packet_frame_count;
        }
}

void receive_superframe( volatile uint8_t *packet1Data, volatile uint8_t *packet2Data, volatile uint8_t *packet3Data )
{
  uint8_t i, j;
        g_receiveSFQueue[g_receiveSFPacketPos].sfNumber = (*((uint16_t*)(packet1Data)) & 0xE000) >> 13;
        g_receiveSFQueue[g_receiveSFPacketPos].frameNumber = (*((uint16_t*)(packet1Data)) & 0x1FFF);

        // Maybe check the other packets to make sure their SF information is the same???

        // Now copy in the data (skip the superframe header) 
        for ( i = 0; i < (DATA_PACKET_SIZE/2); i++ )
        {
          ((uint16_t*)g_receiveSFQueue[g_receiveSFPacketPos].block.packet_data[0])[i] = ((uint16_t*)(packet1Data+SF_HEADER_SIZE))[i];
          ((uint16_t*)g_receiveSFQueue[g_receiveSFPacketPos].block.packet_data[1])[i] = ((uint16_t*)(packet2Data+SF_HEADER_SIZE))[i];
          ((uint16_t*)g_receiveSFQueue[g_receiveSFPacketPos].block.packet_data[2])[i] = ((uint16_t*)(packet3Data+SF_HEADER_SIZE))[i];
        }

        g_receiveSFPacketPos++;
        if ( g_receiveSFPacketPos >= SF_QUEUE_SIZE )
                g_receiveSFPacketPos = 0;

        g_receiveSFProcessCount++;

        if ( g_receiveSFProcessCount >= 7 )
        {
                // Process a superframe
                tSFPacketBlock *pSFData[6];
                tSFPacketBlock xorData;

                uint8_t sf_idx[7][2];
                uint16_t sf_frame_num;
                uint8_t sf_frame_count;

                // We have at least 7 superframes in the receive queue -- process the block of 7 for correction
                // Process block of 4
                j = g_receiveSFProcessPos;
                for ( i = 0; i < 7; i++ )
                {
                        uint8_t correction_inner_pos = (g_receiveSFQueue[j].sfNumber); // SF number

                        //printf( "SF: %d, FN: %d\n", g_receiveSFQueue[j].sfNumber, g_receiveSFQueue[j].frameNumber );

                        sf_idx[i][0] = correction_inner_pos; // Store sf number
                        sf_idx[i][1] = j;                    // Store index

                        if ( i == 0 )
                                sf_frame_num = g_receiveSFQueue[j].frameNumber;
                        else
                        {
                                if ( g_receiveSFQueue[j].frameNumber != sf_frame_num )
                                        break;
                        }

                        j++;
                        if ( j >= SF_QUEUE_SIZE )
                                j = 0;
                }

                sf_frame_count = i;
                if ( sf_frame_count < 6 )
                {
                        // Consume sfs from the receive queue
                        g_receiveSFProcessPos += sf_frame_count;
                        if ( g_receiveSFProcessPos >= SF_QUEUE_SIZE )
                                g_receiveSFProcessPos -= SF_QUEUE_SIZE;       // Wrap

                        g_receiveSFProcessCount -= sf_frame_count;

                        // putc_fast('M');

                        // Skip em -- we can't correct -- we need at least 3 of the same frame number!
                        return;
                }

                // Prepare for corrections -- count the number of packets in this frame number
                //
                // IF COUNT==6 corrections needed
                if ( sf_frame_count == 6 )
                {
                        uint8_t offset;
                        uint8_t sf_1, sf_2, sf_3, sf_4, sf_5, sf_6;

                        sf_1 = sf_idx[0][1];
                        sf_2 = sf_idx[1][1];
                        sf_3 = sf_idx[2][1];
                        sf_4 = sf_idx[3][1];
                        sf_5 = sf_idx[4][1];
                        sf_6 = sf_idx[5][1];

                        // Build XOR packet
                        for ( i = 0; i < 3; i++ )
                        {
                                for ( offset = 0; offset < ((SF_PACKET_SIZE-SF_HEADER_SIZE)/2); offset++ )
                                        ((uint16_t*)xorData.packet_data[i])[offset] = ((uint16_t*)g_receiveSFQueue[sf_1].block.packet_data[i])[offset] ^ ((uint16_t*)g_receiveSFQueue[sf_2].block.packet_data[i])[offset] ^ ((uint16_t*)g_receiveSFQueue[sf_3].block.packet_data[i])[offset] ^ ((uint16_t*)g_receiveSFQueue[sf_4].block.packet_data[i])[offset] ^ ((uint16_t*)g_receiveSFQueue[sf_5].block.packet_data[i])[offset] ^ ((uint16_t*)g_receiveSFQueue[sf_6].block.packet_data[i])[offset];
                        }


                        if ( sf_idx[0][0] == 1 )
                        {
                                pSFData[0] = &xorData;
                                pSFData[1] = &(g_receiveSFQueue[sf_1].block);
                                pSFData[2] = &(g_receiveSFQueue[sf_2].block);
                                pSFData[3] = &(g_receiveSFQueue[sf_3].block);
                                pSFData[4] = &(g_receiveSFQueue[sf_4].block);
                                pSFData[5] = &(g_receiveSFQueue[sf_5].block);
                        }
                        else if ( sf_idx[1][0] == 2 )
                        {
                                pSFData[0] = &(g_receiveSFQueue[sf_1].block);
                                pSFData[1] = &xorData;
                                pSFData[2] = &(g_receiveSFQueue[sf_2].block);
                                pSFData[3] = &(g_receiveSFQueue[sf_3].block);
                                pSFData[4] = &(g_receiveSFQueue[sf_4].block);
                                pSFData[5] = &(g_receiveSFQueue[sf_5].block);

                        }
                        else if ( sf_idx[2][0] == 3 )
                        {
                                pSFData[0] = &(g_receiveSFQueue[sf_1].block);
                                pSFData[1] = &(g_receiveSFQueue[sf_2].block);
                                pSFData[2] = &xorData;
                                pSFData[3] = &(g_receiveSFQueue[sf_3].block);
                                pSFData[4] = &(g_receiveSFQueue[sf_4].block);
                                pSFData[5] = &(g_receiveSFQueue[sf_5].block);

                        }
                        else if ( sf_idx[3][0] == 4 )
                        {
                                pSFData[0] = &(g_receiveSFQueue[sf_1].block);
                                pSFData[1] = &(g_receiveSFQueue[sf_2].block);
                                pSFData[2] = &(g_receiveSFQueue[sf_3].block);
                                pSFData[3] = &xorData;
                                pSFData[4] = &(g_receiveSFQueue[sf_4].block);
                                pSFData[5] = &(g_receiveSFQueue[sf_5].block);

                        }
                        else if ( sf_idx[4][0] == 5 )
                        {
                                pSFData[0] = &(g_receiveSFQueue[sf_1].block);
                                pSFData[1] = &(g_receiveSFQueue[sf_2].block);
                                pSFData[2] = &(g_receiveSFQueue[sf_3].block);
                                pSFData[3] = &(g_receiveSFQueue[sf_4].block);
                                pSFData[4] = &xorData;
                                pSFData[5] = &(g_receiveSFQueue[sf_5].block);
                        }
                        else if ( sf_idx[5][0] == 6 )
                        {
                                pSFData[0] = &(g_receiveSFQueue[sf_1].block);
                                pSFData[1] = &(g_receiveSFQueue[sf_2].block);
                                pSFData[2] = &(g_receiveSFQueue[sf_3].block);
                                pSFData[3] = &(g_receiveSFQueue[sf_4].block);
                                pSFData[4] = &(g_receiveSFQueue[sf_5].block);
                                pSFData[5] = &xorData;
                        }
                        else
                        {
                                // Missing XOR packet
                                pSFData[0] = &(g_receiveSFQueue[sf_1].block);
                                pSFData[1] = &(g_receiveSFQueue[sf_2].block);
                                pSFData[2] = &(g_receiveSFQueue[sf_3].block);
                                pSFData[3] = &(g_receiveSFQueue[sf_4].block);
                                pSFData[4] = &(g_receiveSFQueue[sf_5].block);
                                pSFData[5] = &(g_receiveSFQueue[sf_6].block);
                        }
                }
                else
                {
                        // No corrections needed
                        uint8_t sf_1, sf_2, sf_3, sf_4, sf_5, sf_6;

                        sf_1 = sf_idx[0][1];
                        sf_2 = sf_idx[1][1];
                        sf_3 = sf_idx[2][1];
                        sf_4 = sf_idx[3][1];
                        sf_5 = sf_idx[4][1];
                        sf_6 = sf_idx[5][1];

                        pSFData[0] = &(g_receiveSFQueue[sf_1].block);
                        pSFData[1] = &(g_receiveSFQueue[sf_2].block);
                        pSFData[2] = &(g_receiveSFQueue[sf_3].block);
                        pSFData[3] = &(g_receiveSFQueue[sf_4].block);
                        pSFData[4] = &(g_receiveSFQueue[sf_5].block);
                        pSFData[5] = &(g_receiveSFQueue[sf_6].block);
                }

                // Process packets!
                for ( i = 0; i < 6; i++ )
                {
                        for ( j = 0; j < 3; j++ )
                        {
                                uint8_t packetType = ((pSFData[i]->packet_data[j])[0] & PACKET_TYPE_MASK) >> 7;
                                uint8_t packetLen = (pSFData[i]->packet_data[j])[0] & PACKET_LEN_MASK;

                                if ( packetLen == 0 )
                                        continue;

                                if ( packetType == PACKET_DATA_TYPE_LEON3 )
                                {
                                        // Pass to the Leon-3
                                        process_leon3_data( pSFData[i]->packet_data[j]+1, packetLen );
                                }
                                else if ( packetType == 1 )
                                {
                                        process_oobm_data( pSFData[i]->packet_data[j]+1, packetLen );
                                }
                        }
                }

                // Consume sfs from the receive queue
                g_receiveSFProcessPos += sf_frame_count;
                if ( g_receiveSFProcessPos >= SF_QUEUE_SIZE )
                        g_receiveSFProcessPos -= SF_QUEUE_SIZE;       // Wrap

                g_receiveSFProcessCount -= sf_frame_count;
        }
}

void radio_process_send_queue( void )
{
        uint16_t i, j;
        __attribute__((aligned(2))) uint8_t packetData[OUTTER_PACKET_SIZE];

        // Does the send queue have data already? -- don't add any more
        if ( g_sendProcessCount > 3 )
                return;

        uint8_t srcLen = 0;
        uint8_t packetType = 0;
        uint16_t srcPos;
        uint16_t destPos;
        
        if ( g_leon3DataSendCount > 0 && g_sendSelectPos % 5 != 0 )
        {
                // Select Leon-3 data

                // Enter MUX
                portENTER_CRITICAL( &sendLeon3DataMux );
                
                uint16_t toSendCount = g_leon3DataSendCount;
                if ( toSendCount > DATA_PACKET_DATA_SIZE )
                        toSendCount = DATA_PACKET_DATA_SIZE;

                destPos = 0;
                srcPos = g_leon3DataSendReadPos;

                for ( destPos = 0; destPos < toSendCount; destPos++ )
                {
                        packetData[destPos+5] = g_leon3SendData[srcPos];

                        srcPos++;
                        if ( srcPos >= MAX_SEND_BUFFER_LENGTH )
                                srcPos = 0;

                }

                srcLen = toSendCount;

                g_leon3DataSendReadPos += toSendCount;
                if ( g_leon3DataSendReadPos >= MAX_SEND_BUFFER_LENGTH )
                        g_leon3DataSendReadPos -= MAX_SEND_BUFFER_LENGTH;

                g_leon3DataSendCount -= toSendCount;

                // Exit MUX
                portEXIT_CRITICAL( &sendLeon3DataMux );

                packetType = PACKET_DATA_TYPE_LEON3;
        }
        else if ( g_oobmDataSendCount > 0 )
        {
                // Select OOBM data

                // Enter MUX
                portENTER_CRITICAL( &sendOOBMDataMux );
                
                uint16_t toSendCount = g_oobmDataSendCount;
                if ( toSendCount > DATA_PACKET_DATA_SIZE )
                        toSendCount = DATA_PACKET_DATA_SIZE;

                destPos = 0;
                srcPos = g_oobmDataSendReadPos;

                for ( destPos = 0; destPos < toSendCount; destPos++ )
                {
                        packetData[destPos+5] = g_oobmSendData[srcPos];

                        srcPos++;
                        if ( srcPos >= MAX_SEND_BUFFER_LENGTH )
                                srcPos = 0;
                }

                srcLen = toSendCount;

                g_oobmDataSendReadPos += toSendCount;
                if ( g_oobmDataSendReadPos >= MAX_SEND_BUFFER_LENGTH )
                        g_oobmDataSendReadPos -= MAX_SEND_BUFFER_LENGTH;

                g_oobmDataSendCount -= toSendCount;

                // Exit MUX
                portEXIT_CRITICAL( &sendOOBMDataMux );

                packetType = PACKET_DATA_TYPE_OOBM;
        }
        else
        {
                //Serial.print('E'); //putc_fast('E');
                // Empty packet
                srcLen = 0;

                packetType = 0;
        }
        
        
        // update our selection -- pick leon-3 data 4 times out of 5 to send (4/5 of the bandwidth available -- MAX)
        g_sendSelectPos++;
        if ( g_sendSelectPos >= 5 )
                g_sendSelectPos = 0;

        // Create packet header
        packetData[0] = DATA_PACKET_MAGIC;
        packetData[1] = ((g_sendInnerFragmentNumber & 0x3) << 6) | (g_sendInnerFrameNumber & 0x3F);

        // Create superframe header
        *((uint16_t*)(packetData+2)) = ((g_sendSFFragmentNumber & 0x7) << 13) | (g_sendSFFrameNumber & 0x1FFF);

        // Create data header
        packetData[4] = ((packetType & 0x1) << 7) | (srcLen & 0x3F);    // bit 6 is reserved bit (nothing for now)

        // Calculate inner XOR data
        for ( i = 0; i < (SF_PACKET_SIZE/2); i++ )
                g_sendXORData.innerXORData[i] ^= ((uint16_t*)packetData)[i+1];

        // Calculate the outter frame's XOR data        
        for ( i = 0; i < (DATA_PACKET_SIZE/2); i++ )
                g_sendXORData.sfXORData[g_sendInnerFragmentNumber][i] ^= ((uint16_t*)packetData)[i+2];

        // Add to send queue...
        for ( i = 0; i < (OUTTER_PACKET_SIZE/2); i++ )
                ((uint16_t*)g_sendQueue.packet_data[g_sendProcessPos])[i] = ((uint16_t*)packetData)[i];

        // Encrypt
        for ( i = 0; i < OUTTER_PACKET_SIZE; i+= 8 )
                xtea_encrypt( ((uint32_t*)(g_sendQueue.packet_data[g_sendProcessPos]+i)), radio_read_key );

        g_sendProcessPos++;

        if ( g_sendProcessPos >= PACKET_QUEUE_SIZE )
                g_sendProcessPos = 0;

        g_sendProcessCount++;

        g_sendInnerFragmentNumber++;
        if ( g_sendInnerFragmentNumber >= 3 )
        {
                // Send Inner Packet XOR Data
                
                // Queue up XOR packet
                // Add header to send queue
                g_sendQueue.packet_data[g_sendProcessPos][0] = DATA_PACKET_MAGIC;
                g_sendQueue.packet_data[g_sendProcessPos][1] = ((g_sendInnerFragmentNumber & 0x3) << 6) | (g_sendInnerFrameNumber & 0x3F);
                
                // Add XOR data to send queue
                for ( i = 1; i < (OUTTER_PACKET_SIZE/2); i++ )
                        ((uint16_t*)g_sendQueue.packet_data[g_sendProcessPos])[i] = g_sendXORData.innerXORData[i-1];

                // Encrypt
                for ( i = 0; i < OUTTER_PACKET_SIZE; i+= 8 )
                  xtea_encrypt( ((uint32_t*)(g_sendQueue.packet_data[g_sendProcessPos]+i)), radio_read_key );
                  
                // Update queue with send packet        
                g_sendProcessPos++;

                if ( g_sendProcessPos >= PACKET_QUEUE_SIZE )
                        g_sendProcessPos = 0;

                g_sendProcessCount++;
                // End queue update

                // Reset inner XOR calculation
                for ( i = 0; i < (SF_PACKET_SIZE/2); i++ )
                        g_sendXORData.innerXORData[i] = 0;

                g_sendInnerFragmentNumber = 0;
                g_sendInnerFrameNumber++;

                g_sendSFFragmentNumber++;
        }

        if ( g_sendSFFragmentNumber >= 6 )
        {
                // Queue up a set of inner packets
                for ( j = 0; j < 3; j++ )
                {
                        // Create superframe header
                        // Add header to send queue
                        uint16_t superFrameHeader = ((g_sendSFFragmentNumber & 0x7) << 13) | (g_sendSFFrameNumber & 0x1FFF);
                        
                        g_sendQueue.packet_data[g_sendProcessPos][0] = DATA_PACKET_MAGIC;
                        g_sendQueue.packet_data[g_sendProcessPos][1] = ((j & 0x3) << 6) | (g_sendInnerFrameNumber & 0x3F);
                        
                        ((uint16_t*)g_sendQueue.packet_data[g_sendProcessPos])[1] = superFrameHeader;

                        // Calculate inner XOR data for the superframe header
                        g_sendXORData.innerXORData[0] ^= superFrameHeader;

                        // Add XOR Data to send queue and calculate inner XOR data
                        for ( i = 0; i < (DATA_PACKET_SIZE/2); i++ )
                        {
                                ((uint16_t*)g_sendQueue.packet_data[g_sendProcessPos])[i+2] = g_sendXORData.sfXORData[j][i];
                                g_sendXORData.innerXORData[i+1] ^= g_sendXORData.sfXORData[j][i];
                        }

                        // Encrypt
                        for ( i = 0; i < OUTTER_PACKET_SIZE; i+= 8 )
                          xtea_encrypt( ((uint32_t*)(g_sendQueue.packet_data[g_sendProcessPos]+i)), radio_read_key );
                  
                        // Update queue with send packet        
                        g_sendProcessPos++;

                        if ( g_sendProcessPos >= PACKET_QUEUE_SIZE )
                                g_sendProcessPos = 0;

                        g_sendProcessCount++;
                        // End queue update
                }

                // We've sent the XOR data for the superframe, now send the XOR data for the inner frame
                // Add header to send queue
                g_sendQueue.packet_data[g_sendProcessPos][0] = DATA_PACKET_MAGIC;
                g_sendQueue.packet_data[g_sendProcessPos][1] = ((j & 0x3) << 6) | (g_sendInnerFrameNumber & 0x3F);

                // Add XOR Data to send queue and calculate inner XOR data
                for ( i = 0; i < (SF_PACKET_SIZE/2); i++ )
                        ((uint16_t*)g_sendQueue.packet_data[g_sendProcessPos])[i+1] = g_sendXORData.innerXORData[i];

                // Encrypt
                for ( i = 0; i < OUTTER_PACKET_SIZE; i+= 8 )
                  xtea_encrypt( ((uint32_t*)(g_sendQueue.packet_data[g_sendProcessPos]+i)), radio_read_key );
                  
                // Update queue with send packet        
                g_sendProcessPos++;

                if ( g_sendProcessPos >= PACKET_QUEUE_SIZE )
                        g_sendProcessPos = 0;

                g_sendProcessCount++;
                // End queue update

                // Increment the inner frame -- no need to length constrain it -- it will wraparound    
                g_sendInnerFrameNumber++;

                // Reset inner XOR calculation
                for ( i = 0; i < (SF_PACKET_SIZE/2); i++ )
                        g_sendXORData.innerXORData[i] = 0;

                // Reset outter XOR calculation
                for ( j = 0; j < 3; j++ )
                {
                        for ( i = 0; i < (DATA_PACKET_SIZE/2); i++ )
                                g_sendXORData.sfXORData[j][i] = 0;
                }

                g_sendSFFragmentNumber = 0;
                g_sendSFFrameNumber++;
        }
}

void write_leon3_byte( uint8_t dataIn )
{
  // Enter MUX
  portENTER_CRITICAL( &sendLeon3DataMux );
  
  g_leon3SendData[g_leon3DataSendWritePos++] = dataIn;

  if ( g_leon3DataSendWritePos == MAX_SEND_BUFFER_LENGTH )
    g_leon3DataSendWritePos = 0;

  g_leon3DataSendCount++;

  // Exit MUX
  portEXIT_CRITICAL( &sendLeon3DataMux );
}

void write_leon3(uint8_t * inbuf, signed len)
{
  uint8_t * p = inbuf;
  portENTER_CRITICAL( &sendLeon3DataMux );
  for(signed i=0;i<len;i++)
  {
    g_leon3SendData[g_leon3DataSendWritePos++] = *p++;    
    if ( g_leon3DataSendWritePos == MAX_SEND_BUFFER_LENGTH )
      g_leon3DataSendWritePos = 0;    
    g_leon3DataSendCount++;    
  }
  portEXIT_CRITICAL( &sendLeon3DataMux );
}

void write_oobm_byte( uint8_t dataIn )
{
  // Enter MUX
  portENTER_CRITICAL( &sendOOBMDataMux );
  
  g_oobmSendData[g_oobmDataSendWritePos++] = dataIn;

  if ( g_oobmDataSendWritePos == MAX_SEND_BUFFER_LENGTH )
    g_oobmDataSendWritePos = 0;

  g_oobmDataSendCount++;

  // Exit MUX
  portEXIT_CRITICAL( &sendOOBMDataMux );
}

void write_oobm(uint8_t * inbuf, signed len)
{
  uint8_t * p = inbuf;
  portENTER_CRITICAL( &sendOOBMDataMux );
  for(signed i=0;i<len;i++)
  {
    g_oobmSendData[g_oobmDataSendWritePos++] = *p++;
    if ( g_oobmDataSendWritePos == MAX_SEND_BUFFER_LENGTH )
      g_oobmDataSendWritePos = 0;
    g_oobmDataSendCount++;   
  }
  portEXIT_CRITICAL( &sendOOBMDataMux );
}

void IRAM_ATTR onRadioTimer( void ) 
{
  // Notify complexHandlerTask that the buffer is full.
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  
  vTaskNotifyGiveFromISR( handle_onRadioTimerTask, &xHigherPriorityTaskWoken );
  
  if ( xHigherPriorityTaskWoken ) 
  {
    portYIELD_FROM_ISR();
  }
}

void onRadioTimerTask( void* param1 )
{
  while ( true )
  {
    // Sleep until the ISR gives us something to do, or for 1 second
    uint32_t tcount = ulTaskNotifyTake(pdFALSE, pdMS_TO_TICKS(1000));
    
    if ( g_radioState == RADIOSTATE_OFF )
      continue;
  
    g_timerCounter++;
  
    if ( g_timerCounter % 2 == 0 )
    {
      g_radioState = RADIOSTATE_WRITEPACKET;
      radio_send_data_packet( );
    }
    else
    {
      // Handle DIO0 interrupt during write packet -- packet is written -- go to RX
      //g_radioState = RADIOSTATE_READPACKET;
  
      // Go into RX MODE
      //radio_writereg( REG_OP_MODE, 0x05 ); // Go into RX mode
    }
  
    //Serial.print( 'T' );

    radio_process_receive_queue();
    radio_process_send_queue();
  }
}

void IRAM_ATTR Interrupt_DIO0()
{
  // Notify complexHandlerTask that the buffer is full.
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  
  vTaskNotifyGiveFromISR( handle_onRadioInterruptTask, &xHigherPriorityTaskWoken );
  
  if ( xHigherPriorityTaskWoken ) 
  {
    portYIELD_FROM_ISR();
  }
}

void onRadioInterruptTask( void* param1 )
{
  char buffer[128];
  while ( true )
  {
    // Sleep until the ISR gives us something to do, or for 1 second
    uint32_t tcount = ulTaskNotifyTake(pdFALSE, pdMS_TO_TICKS(1000));
    
    if ( g_radioState == RADIOSTATE_READPACKET )
    {
      uint32_t curClock = ESP.getCycleCount();
      // Enter radio mux
      portENTER_CRITICAL(&radioMux);
      
      radio_readfifo( (uint8_t*)g_receiveQueue.packet_data[g_receivePacketPos], OUTTER_PACKET_SIZE );
      
      xtea_decrypt( (uint32_t*)(g_receiveQueue.packet_data[g_receivePacketPos]), radio_write_key );
  
      if ( (g_receiveQueue.packet_data[g_receivePacketPos][0] & DATA_PACKET_MAGIC_MASK) == DATA_PACKET_MAGIC )
      {
        g_receivePacketPos++;
    
        if ( g_receivePacketPos >= PACKET_QUEUE_SIZE )
                g_receivePacketPos = 0;
    
        // Add a packet to the receive processing queue
        g_receiveProcessCount++;
    
        // Reset the receive miss count 
        g_receivePacketMissCount = 0;
      }
  
      // Exit radio mux
      portEXIT_CRITICAL(&radioMux);
      
      //Serial.print( 'A' );
      //sprintf( buffer, "C:%d", curClock - g_lastClockCount );
      //Serial.println( buffer );
    }
    else if ( g_radioState == RADIOSTATE_WRITEPACKET )
    {
      
      
      
      // Enter radio MUX
      portENTER_CRITICAL(&radioMux);
      
      // Handle DIO0 interrupt during write packet -- packet is written -- go to RX
      g_radioState = RADIOSTATE_READPACKET;
  
      // Go into RX MODE
      radio_writereg( REG_OP_MODE, 0x05 ); // Go into RX mode
  
      // Exit radio MUX
      portEXIT_CRITICAL(&radioMux);
  
      //Serial.print( 'S' );
    }
  }
}

void IRAM_ATTR Interrupt_DIO1()
{
  
  
}


void radio_reset( void )
{
  pinMode( RADIO_SS, OUTPUT );
  pinMode( DIO0_PIN, INPUT );
  //pinMode( DIO1_PIN, INPUT );
  //pinMode( DIO2_PIN, INPUT );
  
  digitalWrite( RADIO_SS, HIGH );
  pinMode( RADIO_RESET, OUTPUT );

  digitalWrite(RADIO_RESET, HIGH);
  delay( 20 );
  digitalWrite(RADIO_RESET, LOW);
  delay(20);
  digitalWrite(RADIO_RESET, HIGH);
  delay(50);
  
  // Do nothing -- we can't reset the radio since we don't control the reset pin
}

void radio_spi_init( void )
{
  //SPI.setClockDivider( SPI_CLOCK_DIV2 );
  SPI.begin(); //  SCK, MISO, MOSI, RADIO_SS );
}

void radio_writereg( uint8_t regNum, uint8_t value )
{
  SPI.beginTransaction(SPISettings (SPISPEED, MSBFIRST, SPI_MODE0));
  digitalWrite(RADIO_SS, LOW);

  SPI.transfer( regNum | 0x80 );
  SPI.transfer( value );
  
  digitalWrite(RADIO_SS, HIGH);
  SPI.endTransaction( );
}

uint8_t radio_readreg( uint8_t regNum )
{
  uint8_t value;
  SPI.beginTransaction(SPISettings (SPISPEED, MSBFIRST, SPI_MODE0));
  digitalWrite(RADIO_SS, LOW);

  SPI.transfer( regNum & 0x7F );
  value = SPI.transfer( 0 );
  
  digitalWrite(RADIO_SS, HIGH);
  SPI.endTransaction( );

  return (value);
}

void radio_writebyte( uint8_t value )
{
  SPI.beginTransaction(SPISettings (SPISPEED, MSBFIRST, SPI_MODE0));
  digitalWrite(RADIO_SS, LOW);

  SPI.transfer( value );
  
  digitalWrite(RADIO_SS, HIGH);
  SPI.endTransaction( );
}

uint8_t radio_spi_readbyte( void )
{
  uint8_t value = 0;
  SPI.beginTransaction(SPISettings (SPISPEED, MSBFIRST, SPI_MODE0));
  digitalWrite(RADIO_SS, LOW);

  value = SPI.transfer( value );
  
  digitalWrite(RADIO_SS, HIGH);
  SPI.endTransaction( );

  return (value);
}

void radio_writefifo( uint8_t *pData, uint32_t count )
{
  uint32_t i;
  if ( count > 256 )
    count = 256;

  SPI.beginTransaction(SPISettings (SPISPEED, MSBFIRST, SPI_MODE0));
  digitalWrite(RADIO_SS, LOW);

  SPI.transfer( 0x80 ); // Write burst mode to FIFO

  for ( i = 0; i < count; i++ )
    SPI.transfer( pData[i] );

  digitalWrite(RADIO_SS, HIGH);
  SPI.endTransaction( );
}

void radio_readfifo( uint8_t *pData, uint32_t count )
{
  uint32_t i;
  if ( count > 256 )
    count = 256;

  SPI.beginTransaction(SPISettings (SPISPEED, MSBFIRST, SPI_MODE0));
  digitalWrite(RADIO_SS, LOW);

  SPI.transfer( 0x0 ); // Read burst mode to FIFO

  for ( i = 0; i < count; i++ )
    pData[i] = SPI.transfer( 0 );

  digitalWrite(RADIO_SS, HIGH);
  SPI.endTransaction( ); 
}

void radio_setchannel( uint8_t channel_num )
{
  uint16_t channel = channel_num % RADIO_CHANNEL_COUNT;
 
  channel = RADIO_BASE_CHANNEL_OFFSETHI + (channel * RADIO_CHANNEL_INCREMENT);
  
  radio_writereg( REG_FRF_MSB, (uint8_t)((channel >> 8) & 0xFF) );
  radio_writereg( REG_FRF_MID, (uint8_t)((channel) & 0xFF) );
}

void radio_init( void )
{
  uint16_t temp;
  
  g_radioState = RADIOSTATE_OFF;

  g_timerCounter = 0;

  // Setup radio
  radio_reset();
  radio_spi_init();

  attachInterrupt(DIO0_PIN, Interrupt_DIO0, RISING);  // Separate interrupts
  //attachInterrupt(DIO1_PIN, Interrupt_DIO1, RISING);  // Separate interrupts 

  // check version
  while ( true )
  {
    uint8_t version = radio_readreg(REG_VERSION);
    if (version != 0x12) { 
      // TODO: DEBUG code change this later
      Serial.println ("Invalid version.\n" ); 
    }
    else
      break;
  }
  
  // Enter sleep mode
  radio_writereg( REG_OP_MODE, 0x00 );
  
  // Enter FSK mode and sleep
  radio_writereg( REG_OP_MODE, 0x00 );
  
  while ( radio_readreg( REG_OP_MODE ) != 0x0 )
  {
    // DEBUG code change this later
    // Serial.println ("Not in FSK mode yet.\n" );
    
    // Enter sleep mode
    radio_writereg( REG_OP_MODE, 0x00 );
    
    // Enter LORA mode and SLEEP
    radio_writereg( REG_OP_MODE, 0x0 );
  }
  
  // FSK standby mode
  radio_writereg( REG_OP_MODE, 0x01 );
  
  // Set the PA configuration to the PA_BOOST
  radio_writereg( REG_PA_CONFIG, PA_CONFIG_VALUE );   // PA_BOOST
  
  radio_writereg( 0x0A, 0x49 );   // RegPaRamp, ModulationShaping=10 or Gaussian Filter BT = 0.5, PaRamp=9 (default)

  // Bit rate (125kbit/s) BitRate[15:8] = 0x01, BitRate[7:0] = 0x00
  radio_writereg( REG_BITRATE_MSB,  0x01 );
  radio_writereg( REG_BITRATE_LSB,  0x00 );   // 32MHz divided by (0x100), or 125,000 bit/s

  // Frequency Deviation, 125KHz
  radio_writereg( REG_FDEV_MSB, 0x08 );
  radio_writereg( REG_FDEV_LSB, 0x00 );

  // RegRxConfig, RestartRxOnCollision=off, AfcAutoOn, AgcAutoOn
  //radio_writereg( 0x0d, 0x1E );

  // Set RxBw (RX Filter Bandwidth)
  radio_writereg( 0x12, 0x02 );

  // Set channel to TEAM ID
  radio_setchannel( MY_TEAM_ID );
  
  // Preamble length
  radio_writereg( 0x25, 0x0 ); // Preamble length MSB
  radio_writereg( 0x26, 0x4 ); // Preamble length LSB

  // Sync word length
  radio_writereg( 0x27, 0x92 ); // AutoRestartRxMode -> ON, SyncON, SyncSize=2

  // Sync words
  radio_writereg( 0x28, ((MY_TEAM_ID+1) * 17) & 0xFF ); // 0x47 );
  radio_writereg( 0x29, 0x1A );

  // Packet configuration register
  radio_writereg( 0x30, 0x10 );   // Fixed packet length, CRC On
  radio_writereg( 0x31, 0x40 );
  radio_writereg( 0x32, 0x40 );   // 64-bytes in length
  
  temp = radio_readreg( 0x6c );
  
#ifdef DEBUG_RADIO_ON
  Serial.println( "TEMP=%u\n", temp );
#endif
  
  //seed_quick_rand( temp ^ radio_sync_spread_key );
  
  // Initial radio setup complete.
  //P1DIR = 0x00;
  //P1IE = (RADIO_DIO0_BIT | RADIO_DIO1_BIT | MODE_SELECT_RUN_PIN | MODE_SELECT_PROG_PIN | MODE_SELECT_DBG_PIN);

  g_radioState = RADIOSTATE_WRITEPACKET;
}
