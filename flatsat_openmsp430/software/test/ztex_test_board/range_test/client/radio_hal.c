#include "omsp_radio.h"
#include "hardware.h"
#include "radio_hal.h"

#define RADIO_CSN_BIT           0x1
#define RADIO_CSN_PORT          (P2OUT)

#define RADIO_RESET_PORT        (P2OUT)
#define RADIO_RESET_BIT         (0x4)

//--------------------------------------------------//
//                   Delay function                 //
//--------------------------------------------------//
void delay(unsigned int d) 
{
   while(d--) {
      nop();
      nop();
   }
}

void radio_reset( void )
{
        unsigned int i;

	// Turn off CSN
	RADIO_CSN_PORT |= RADIO_CSN_BIT;

	// Start reset sequence
        RADIO_RESET_PORT &= ~RADIO_RESET_BIT;

        for ( i = 0; i < 200; i++ )
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

void radio_spi_writebyte( uint8_t data )
{
        RADIOSPI_DATA = data;
        RADIOSPI_CTRL |= RADIOSPI_CTRL_EN;

        while ( RADIOSPI_CTRL & RADIOSPI_CTRL_EN )
                ;

}

uint8_t radio_spi_readbyte( void )
{
	uint8_t data;

        RADIOSPI_DATA = 0x0;
        RADIOSPI_CTRL |= RADIOSPI_CTRL_EN;

        while ( RADIOSPI_CTRL & RADIOSPI_CTRL_EN )
                ;

        data = RADIOSPI_DATA;

        return (data);
}

void radio_writereg( uint8_t regNum, uint8_t value )
{
        RADIO_CSN_PORT &= ~RADIO_CSN_BIT;

        regNum |= 0x80;

        radio_spi_writebyte( regNum );
        radio_spi_writebyte( value );

        RADIO_CSN_PORT |= RADIO_CSN_BIT;
}

void radio_writefifo( uint8_t *data, unsigned int count )
{
        unsigned int i;

        // The FIFO is only 256 bytes in size   
        if ( count > 256 )
                count = 256;

        RADIO_CSN_PORT &= ~RADIO_CSN_BIT;

        // Write in burst mode to FIFO register
        RADIOSPI_DATA = 0x80;
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

void radio_readfifo( uint8_t *data, unsigned int count )
{
	unsigned int i;

	if ( count > 256 )
		count = 256;

	RADIO_CSN_PORT &= ~RADIO_CSN_BIT;
	
	// Read in burst mode to FIFO register
	RADIOSPI_DATA = 0x00;
	RADIOSPI_CTRL |= RADIOSPI_CTRL_EN;

	while ( RADIOSPI_CTRL & RADIOSPI_CTRL_EN )
		;

	for ( i = 0; i < count; i++ )
	{
		RADIOSPI_CTRL |= RADIOSPI_CTRL_EN;

		while ( RADIOSPI_CTRL & RADIOSPI_CTRL_EN )
			;

		data[i] = RADIOSPI_DATA;
	}

	RADIO_CSN_PORT |= RADIO_CSN_BIT;
}

uint8_t radio_readreg( uint8_t regNum )
{
        uint8_t data;

        RADIO_CSN_PORT &= ~RADIO_CSN_BIT;

        radio_spi_writebyte( regNum );
        data = radio_spi_readbyte();

        RADIO_CSN_PORT |= RADIO_CSN_BIT;

        return (data);
}
