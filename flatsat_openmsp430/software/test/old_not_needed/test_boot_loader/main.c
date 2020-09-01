#include "hardware.h"
#include "conflash.h"
#include "util.h"
#include "serial_loader.h"
#include <stdlib.h>
#include <stdio.h>

//--------------------------------------------------//
//                   Delay function                 //
//--------------------------------------------------//
void delay(unsigned int d) {
   while(d--) {
      nop();
      nop();
   }
}
 
//--------------------------------------------------//
//                 putChar function                 //
//            (Send a byte to the UART)             //
//--------------------------------------------------//
int putchar (int txdata) 
{
 
  // Wait until the TX buffer is not full
  while (UART_STAT & UART_TX_FULL);
 
  // Write the output character
  UART_TXD = txdata;
 
  return 0;
}

/*
#define SERIAL_LINE_MODE_WAIT_PROG	0
#define SERIAL_LINE_MODE_PROG		1
#define SERIAL_LINE_MODE_CRC		2

#define MAX_ADDRESS_COUNT (16384)

#define APPDATA_LOC_ADDRESS		0x60000L
#define APPDATA_MEM1_ADDRESS		0x62000L
#define APPDATA_MEM2_ADDRESS		0x66000L
#define APPDATA_MEM_SIZE		0x4000	// 16KB

unsigned char g_serialLineMode;
uint32_t g_addressStart;
uint32_t g_curAddress;

void setup_serial_line_program( void )
{
	unsigned char load_app;
	unsigned char loc[2];
	g_serialLineMode = SERIAL_LINE_MODE_WAIT_PROG;
	g_addressStart = 0;

	conflash_init();

	// READ LOC
	conflash_read_bytes( APPDATA_LOC_ADDRESS, loc, 2 );

	printf( "Read data: %X %X\n", loc[0], loc[1] );

	// CHECK the loc -- note: it may not be set (previously erased)
	load_app = 0;
	if ( loc[0] == 0xA8 )
	{
		if ( loc[1] == 0x2 )
		{
			load_app = 2;
			g_addressStart = APPDATA_MEM1_ADDRESS;
		}
		else if ( loc[1] == 0x1 )
		{
			load_app = 1;
			g_addressStart = APPDATA_MEM2_ADDRESS;
		}
		else
			g_addressStart = APPDATA_MEM1_ADDRESS;
	}
	else
		g_addressStart = APPDATA_MEM1_ADDRESS;

	if ( load_app )
	{
		uint32_t offset;
		uint32_t addressBase;	
		unsigned int i;
		unsigned char data[128];

		if ( load_app == 1 )
			addressBase = APPDATA_MEM1_ADDRESS;
		else if ( load_app == 2 )
			addressBase = APPDATA_MEM2_ADDRESS;

		printf( "Loading app processor from flash %d.\n", load_app );

		(*APPLOADER_CTRL) = 0x1;
		for ( offset = 0; offset < APPDATA_MEM_SIZE; )
		{
			conflash_read_bytes( (addressBase+offset), data, 128 );
			offset+=128;

			for ( i = 0; i < 128; i+=2 )
			{
				uint16_t value = 0;
				value = ((uint16_t)data[i] << 8);
				value |= ((uint16_t)data[i+1]);

				(*APPLOADER_DATA) = value;
			}	
		}
		(*APPLOADER_CTRL) = 0x0;	

		printf( "Loading app processor complete.\n" );
	}
	else
		printf( "No app processor code found in flash. Please program device.\n" );

	printf( "Start address is: %X %X %X\n", (uint8_t)((g_addressStart >> 16) & 0xFF), (uint8_t)((g_addressStart >> 8) & 0xFF), (uint8_t)(g_addressStart & 0xFF) );
}

void process_serial_line( unsigned char *serialLine )
{
	switch ( g_serialLineMode )
	{
	case SERIAL_LINE_MODE_WAIT_PROG:
		if ( serialLine[0] == 'P' && serialLine[1] == 'R' && serialLine[2] == 'O' && serialLine[3] == 'G' )
		{
			unsigned char i;

			printf( "WAIT\n" );
	
			for ( i = 0; i < 4; i++ )
			{	
				conflash_write_enable();
				conflash_ss_erase( (g_addressStart + ((uint32_t)i<<12)) );

				while( conflash_wip_check() )
					; // Do nothing
			}

			
			g_serialLineMode = SERIAL_LINE_MODE_PROG;

			g_curAddress = g_addressStart;
  
			printf( "READY\n" ); 
		}
		break;

	case SERIAL_LINE_MODE_PROG:
		{
			unsigned char data[2];

			data[0] = read_hex_uint8( (char*)serialLine );
			data[1] = read_hex_uint8( (char*)serialLine+2 );
			
			// Turn on write!
			conflash_write_enable();
			conflash_page_program( g_curAddress, data, 2 );

			g_curAddress+=2;

			if ( (uint16_t)(g_curAddress - g_addressStart) >= MAX_ADDRESS_COUNT )
				g_serialLineMode = SERIAL_LINE_MODE_CRC;	
		}
	
		break;

	case SERIAL_LINE_MODE_CRC:
		{
			uint32_t startAddress;
			uint32_t flashChecksum;
			uint32_t userChecksum;
			uint32_t offset;
			unsigned char i;
			unsigned char data[128];

			userChecksum = 0;
		
			userChecksum = ((uint32_t)read_hex_uint8( (char*)serialLine ) << 24);
			userChecksum |= ((uint32_t)read_hex_uint8( (char*)serialLine+2 ) << 16);
			userChecksum |= ((uint32_t)read_hex_uint8( (char*)serialLine+4 ) << 8 );
			userChecksum |= ((uint32_t)read_hex_uint8( (char*)serialLine+6 ));
			
			// Verify CRC
			startAddress = g_addressStart;

			flashChecksum = 0;
			for ( offset = 0; offset < 16384; )
			{
				conflash_read_bytes( (startAddress+offset), data, 128 );
				offset += 128;

				for ( i = 0; i < 128; i+=2 )
				{
					uint16_t value = 0;
					value = ((uint16_t)data[i] << 8);
					value |= ((uint16_t)data[i+1]);

					flashChecksum += value;
				}
			}

			printf ("RX CHECKSUM: %02X%02X%02X%02X\n", (uint8_t)((userChecksum >> 24) & 0xFF), (uint8_t)((userChecksum >> 16) & 0xFF), (uint8_t)((userChecksum>>8) & 0xFF), (uint8_t)(userChecksum & 0xFF) );
			printf ("VF CHECKSUM: %02X%02X%02X%02X\n", (uint8_t)((flashChecksum >> 24) & 0xFF), (uint8_t)((flashChecksum >> 16) & 0xFF), (uint8_t)((flashChecksum>>8) & 0xFF), (uint8_t)(flashChecksum & 0xFF) );

			if ( flashChecksum == userChecksum )
			{
				data[0] = 0xA8;
				if ( g_addressStart == APPDATA_MEM1_ADDRESS )
					data[1] = 1;	
				else if ( g_addressStart == APPDATA_MEM2_ADDRESS )
					data[1] = 2;

				conflash_write_enable();
				conflash_page_write( APPDATA_LOC_ADDRESS, data, 2 );

				while( conflash_wip_check() )
					; // Do nothing
				
				// Flash successful... now point to the right firmware blob
				printf( "SUCCESS\n" );
			}
			else
				printf( "FAIL: BAD CHECKSUM\n" );
 
			g_serialLineMode = SERIAL_LINE_MODE_WAIT_PROG;
		}		
		break;

	default:
		g_serialLineMode = SERIAL_LINE_MODE_WAIT_PROG;
	}	
}
*/
 
//--------------------------------------------------//
//        UART RX interrupt service routine         //
//         (receive a byte from the UART)           //
//--------------------------------------------------//
#define MAX_SERIAL_LINE_LEN 	32
unsigned char g_serialLine[MAX_SERIAL_LINE_LEN];
unsigned char g_serialLineLen;
 
wakeup interrupt (PROGRAM_UART_RX_VECTOR) INT_uart_rx(void) 
{
	char rxdata;
  
	// Read the received data
	rxdata = UART_RXD;
	
	// Clear the receive pending flag
	UART_STAT = UART_RX_PND;

	if ( rxdata == '\n' || rxdata == '\r' )
	{
		g_serialLine[g_serialLineLen] = '\0';

		process_serial_loader_line( g_serialLine );	
		g_serialLineLen = 0;
	}
	else if ( g_serialLineLen < MAX_SERIAL_LINE_LEN-1 )
	{
		g_serialLine[g_serialLineLen++] = rxdata;
	}
	// ELSE: Ignore	
 
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
    WDTCTL = WDTPW | WDTHOLD;           // Init watchdog timer
 
    UART_BAUD = BAUD;                   // Init UART
    UART_CTL  = UART_EN | UART_IEN_RX;

    delay( 0xffff );
    delay( 0xffff );
    delay( 0xffff );
    delay( 0xffff );
 
    printf("\r\n====== Testing Boot Loader ======\r\n");   //say hello
    printf("\r\n====== Testing Boot Loader ======\r\n");   //say hello
    printf("\r\n====== Testing Boot Loader ======\r\n");   //say hello
    printf("\r\n====== Testing Boot Loader ======\r\n");   //say hello

    printf( "\r\n======= Writing app_loader =======\r\n" );

	init_serial_loader();
	printf( "Setup complete.\n" );

	eint();

/*
	// Test conflash
	conflash_init();

	// Display

	printf( "Status is: %X\n", conflash_read_status() );

	// Subsector erase
	printf( "Subsector erase at: 393216\n" );
	conflash_ss_erase( 0x60000 );

	// Now wait for ready...
	printf( "Waiting for ready...\n" );
	
	while ( conflash_wip_check() )
		printf( "." );

	printf( "Enabling write.\n" );
	conflash_write_enable();
	
	printf( "Write in progress.\n" );

	// Now write a page
	conflash_page_program( 0x60000, data, 8 );
	
	printf( "Waiting for ready...\n" );
	
	while ( conflash_wip_check() )
		printf( "." );

	printf( "Write done....\n" );

	// Now read back...
	data[0] = 'A';
	data[2] = 'A';
	data[4] = 'a';
	data[6] = 'a';

	conflash_read_bytes( 0x0000, data, 8 );

	printf( "Data is: %s\n", data );
	printf( "Data: %X %X %X %X %X %X %X %X\n", data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7] );

	id = conflash_readid();
	printf( "ID is: %d\n", (uint16_t)id );

	printf( "....\n" );
*/	

    while (1)
    {

    } 
/*
    while (1) {                         //main loop, never ends...
 
        printf("> ");                   //show prompt
        reading = 1;
        while (reading) {               //loop and read characters
 
            LPM0;                       //sync, wakeup by irq
 
	    led++;                      // Some lighting...
	    if (led==4) {
	      led = 0;
	    }
	    P2OUT = (0x01 << led);
 
            switch (rxdata) {
                //process RETURN key
                case '\r':
                //case '\n':
                    printf("\r\n");     //finish line
                    buf[pos++] = 0;     //to use printf...
                    printf(":%s\r\n", buf);
                    reading = 0;        //exit read loop
                    pos = 0;            //reset buffer
                    break;
                //backspace
                case '\b':
                    if (pos > 0) {      //is there a char to delete?
                        pos--;          //remove it in buffer
                        putchar('\b');  //go back
                        putchar(' ');   //erase on screen
                        putchar('\b');  //go back
                    }
                    break;
                //other characters
                default:
                    //only store characters if buffer has space
                    if (pos < sizeof(buf)) {
                        putchar(rxdata);     //echo
                        buf[pos++] = rxdata; //store
                    }
            }
        }
    }
*/
}
