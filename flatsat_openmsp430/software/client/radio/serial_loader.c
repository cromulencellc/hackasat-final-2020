#include "serial_loader.h"
#include "conflash.h"
#include "util.h"

#define APPLOADER_CTRL ((volatile unsigned int *)0x00A8)
#define APPLOADER_DATA ((volatile unsigned int *)0x00AA)

#define SERIAL_LINE_MODE_WAIT_PROG      0
#define SERIAL_LINE_MODE_PROG           1
#define SERIAL_LINE_MODE_CRC            2

#define MAX_ADDRESS_COUNT (16384)

#define APPDATA_LOC_ADDRESS             0x60000L
#define APPDATA_MEM1_ADDRESS            0x62000L
#define APPDATA_MEM2_ADDRESS            0x66000L
#define APPDATA_MEM_SIZE                0x4000  // 16KB

#define SERIAL_LOADER_DEBUG	1

unsigned char g_serialLineMode;
uint32_t g_addressStart;
uint32_t g_curAddress;

void init_serial_loader( void )
{
        unsigned char load_app;
        unsigned char loc[2];
        g_serialLineMode = SERIAL_LINE_MODE_WAIT_PROG;
        g_addressStart = 0;

        conflash_init();

        // READ LOC
        conflash_read_bytes( APPDATA_LOC_ADDRESS, loc, 2 );

#ifdef SERIAL_LOADER_DEBUG
        printf( "Read data: %X %X\n", loc[0], loc[1] );
#endif

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

#ifdef SERIAL_LOADER_DEBUG
                printf( "Loading app processor from flash %d.\n", load_app );
#endif

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

#ifdef SERIAL_LOADER_DEBUG
                puts( "Loading app processor complete" );
#endif
        }
        else
	{
#ifdef SERIAL_LOADER_DEBUG
                puts( "No app processor code found in flash. Please program device" );
#endif
	}

#ifdef SERIAL_LOADER_DEBUG
        printf( "Start address is: %X %X %X\n", (uint8_t)((g_addressStart >> 16) & 0xFF), (uint8_t)((g_addressStart >> 8) & 0xFF), (uint8_t)(g_addressStart & 0xFF) );
#endif
}

void process_serial_loader_line( unsigned char *serialLine )
{
        switch ( g_serialLineMode )
        {
        case SERIAL_LINE_MODE_WAIT_PROG:
                if ( serialLine[0] == 'P' && serialLine[1] == 'R' && serialLine[2] == 'O' && serialLine[3] == 'G' )
                {
                        unsigned char i;

                        puts( "WAIT" );

                        for ( i = 0; i < 4; i++ )
                        {
                                conflash_write_enable();
                                conflash_ss_erase( (g_addressStart + ((uint32_t)i<<12)) );

                                while( conflash_wip_check() )
                                        ; // Do nothing
                        }


                        g_serialLineMode = SERIAL_LINE_MODE_PROG;

                        g_curAddress = g_addressStart;

                        puts( "READY" );
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
                               
			while( conflash_wip_check() )
				; // Do nothing

                        g_curAddress+=2;

			if ( (uint16_t)(g_curAddress - g_addressStart) >= MAX_ADDRESS_COUNT )
                                g_serialLineMode = SERIAL_LINE_MODE_CRC;

			putchar('\n');
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
                                puts( "SUCCESS" );
                        }
                        else
			{
                                puts( "FAIL: BAD CHECKSUM" );
			}

                        g_serialLineMode = SERIAL_LINE_MODE_WAIT_PROG;
                }
                break;

        default:
                g_serialLineMode = SERIAL_LINE_MODE_WAIT_PROG;
        }
}
