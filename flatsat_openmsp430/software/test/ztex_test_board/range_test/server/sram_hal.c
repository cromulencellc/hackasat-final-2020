#include "omsp_radio.h"
#include "sram_hal.h"

// We used two types of SRAM on this build
// one 512kbit and the proper one 1Mbit
// If it is 512kbit the address is 2 bytes
// if it is 1Mbit the address is 3 bytes
#define SRAM_1M

void sram_init( void )
{
        // 8MHz SPI
        SRAMSPI_CTRL = (0x0100);

        // Bring CSN high       
        SRAM_CSN_PORT |= SRAM_CSN_BIT;
}

uint8_t sram_read_mode( void )
{
	uint8_t data;

        SRAM_CSN_PORT &= ~SRAM_CSN_BIT;

        SRAMSPI_DATA = 0x0500;
        SRAMSPI_CTRL |= SRAMSPI_CTRL_EN;

        while ( SRAMSPI_CTRL & SRAMSPI_CTRL_EN )
                ;

        data = (uint8_t)SRAMSPI_DATA;

        SRAM_CSN_PORT |= SRAM_CSN_BIT;

        return (data);
}

void sram_write_u16( uint16_t address, uint16_t *dest, uint16_t length )
{
	uint16_t pos;
	uint16_t temp_addr;

	// Control access to SRAM
	dint();

	SRAM_CSN_PORT &= ~SRAM_CSN_BIT;

#ifdef SRAM_512
        SRAMSPI_CTRL &= ~(0x0100);
	
	SRAMSPI_DATA = 0x02;

        SRAMSPI_CTRL |= SRAMSPI_CTRL_EN;
	while ( SRAMSPI_CTRL & SRAMSPI_CTRL_EN )
		;

	SRAMSPI_CTRL |= (0x0100);

#else
	if ( address & 0x8000 )
		SRAMSPI_DATA = 0x0201;
	else
		SRAMSPI_DATA = 0x0200;

        SRAMSPI_CTRL |= SRAMSPI_CTRL_EN;
	while ( SRAMSPI_CTRL & SRAMSPI_CTRL_EN )
		;
#endif

	temp_addr = (address << 1);
	SRAMSPI_DATA = temp_addr;
        
	SRAMSPI_CTRL |= SRAMSPI_CTRL_EN;
	while ( SRAMSPI_CTRL & SRAMSPI_CTRL_EN )
		;

	for ( pos=0; pos<length; pos++ )
	{
		SRAMSPI_DATA = dest[pos];
		SRAMSPI_CTRL |= SRAMSPI_CTRL_EN;

		while ( SRAMSPI_CTRL & SRAMSPI_CTRL_EN )
			;
	}
	
        SRAM_CSN_PORT |= SRAM_CSN_BIT;
	
	// Control access to SRAM
	eint();
}

void sram_read_u16( uint16_t address, uint16_t *dest, uint16_t length )
{
	uint16_t pos;
	uint16_t temp_addr;

	// Control access to SRAM
	dint();

	SRAM_CSN_PORT &= ~SRAM_CSN_BIT;

#ifdef SRAM_512
        SRAMSPI_CTRL &= ~(0x0100);
	
	SRAMSPI_DATA = 0x03;

        SRAMSPI_CTRL |= SRAMSPI_CTRL_EN;
	while ( SRAMSPI_CTRL & SRAMSPI_CTRL_EN )
		;

        SRAMSPI_CTRL |= (0x0100);
#else
	if ( address & 0x8000 )
		SRAMSPI_DATA = 0x0301;
	else
		SRAMSPI_DATA = 0x0300;

        SRAMSPI_CTRL |= SRAMSPI_CTRL_EN;
	while ( SRAMSPI_CTRL & SRAMSPI_CTRL_EN )
		;
#endif

	temp_addr = (address << 1);
	SRAMSPI_DATA = temp_addr;
        
	SRAMSPI_CTRL |= SRAMSPI_CTRL_EN;
	while ( SRAMSPI_CTRL & SRAMSPI_CTRL_EN )
		;

	for ( pos = 0; pos < length; pos++ )
	{
		SRAMSPI_CTRL |= SRAMSPI_CTRL_EN;

		while ( SRAMSPI_CTRL & SRAMSPI_CTRL_EN )
			;

		dest[pos] = SRAMSPI_DATA;
	}
	
        SRAM_CSN_PORT |= SRAM_CSN_BIT;
	
	// Control access to SRAM
	eint();
}
