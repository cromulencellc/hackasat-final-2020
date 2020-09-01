#include "conflash.h"
#include "omsp_radio.h"
#include "cdh_common.h"
#include <stdio.h>
#include <stdlib.h>

void conflash_init( void )
{
	P2OUT |= CONFLASH_CSN_BIT;
	P2DIR |= CONFLASH_CSN_BIT;
	CONFLASHSPI_CTRL = (0x0000);
}

uint32_t conflash_readid( void )
{
	uint32_t id = 0;

	CONFLASH_PORT &= ~CONFLASH_CSN_BIT;

	// Send address
	conflash_writebyte( 0x9f );
	id |= (uint32_t)conflash_readbyte() << 16;
	id |= (uint32_t)conflash_readbyte() << 8;
	id |= (uint32_t)conflash_readbyte();

	CONFLASH_PORT |= CONFLASH_CSN_BIT;

	return id;
}

void conflash_writebyte( unsigned char byte )
{
	CONFLASHSPI_DATA = byte;
	CONFLASHSPI_CTRL |= CONFLASHSPI_CTRL_EN;

	while ( CONFLASHSPI_CTRL & CONFLASHSPI_CTRL_EN )
		;
}

unsigned char conflash_readbyte( void )
{
	unsigned char data;

	CONFLASHSPI_DATA = 0xFF;
	CONFLASHSPI_CTRL |= CONFLASHSPI_CTRL_EN;

	while ( CONFLASHSPI_CTRL & CONFLASHSPI_CTRL_EN )
		;

	data = CONFLASHSPI_DATA;

	return data;
}

uint8_t conflash_read_status( void )
{
	uint8_t status;

	CONFLASH_PORT &= ~CONFLASH_CSN_BIT;

	conflash_writebyte( 0x05 );
	status = conflash_readbyte();

	CONFLASH_PORT |= CONFLASH_CSN_BIT;

	return status;	
}

void conflash_ss_erase( uint32_t address )
{
	CONFLASH_PORT &= ~CONFLASH_CSN_BIT;

	// Send address
	conflash_writebyte( 0x20 );
	conflash_writebyte( (uint8_t)((address >> 16) & 0xFF) );
	conflash_writebyte( (uint8_t)((address >> 8) & 0xFF) );
	conflash_writebyte( (uint8_t)((address & 0xFF)) );

	CONFLASH_PORT |= CONFLASH_CSN_BIT;	
}

void conflash_page_program( uint32_t address, uint8_t *pageData, uint16_t dataSize )
{
	uint16_t i = 0;

	if ( dataSize > 256 )
		dataSize = 256;
	
	if ( dataSize == 0 )
		return;

	CONFLASH_PORT &= ~CONFLASH_CSN_BIT;

	conflash_writebyte( 0x02 );
	conflash_writebyte( (uint8_t)((address >> 16) & 0xFF) );
	conflash_writebyte( (uint8_t)((address >> 8) & 0xFF) );
	conflash_writebyte( (uint8_t)((address & 0xFF)) );

	for ( i = 0; i < dataSize; i++ )
		conflash_writebyte( pageData[i] );

	CONFLASH_PORT |= CONFLASH_CSN_BIT; 
}

void conflash_page_write( uint32_t address, uint8_t *pageData, uint16_t dataSize )
{
	uint16_t i = 0;

	if ( dataSize > 256 )
		dataSize = 256;
	
	if ( dataSize == 0 )
		return;

	CONFLASH_PORT &= ~CONFLASH_CSN_BIT;

	conflash_writebyte( 0x0A );
	conflash_writebyte( (uint8_t)((address >> 16) & 0xFF) );
	conflash_writebyte( (uint8_t)((address >> 8) & 0xFF) );
	conflash_writebyte( (uint8_t)((address & 0xFF)) );

	for ( i = 0; i < dataSize; i++ )
		conflash_writebyte( pageData[i] );

	CONFLASH_PORT |= CONFLASH_CSN_BIT; 
}

unsigned char conflash_wip_check( void )
{
	unsigned char status_reg;
	CONFLASH_PORT &= ~CONFLASH_CSN_BIT;

	conflash_writebyte( 0x05 );
	status_reg = conflash_readbyte();

	CONFLASH_PORT |= CONFLASH_CSN_BIT;

	return (status_reg & 0x1);	
}

void conflash_write_enable( void )
{
	CONFLASH_PORT &= ~CONFLASH_CSN_BIT;
	conflash_writebyte( 0x06 );
	CONFLASH_PORT |= CONFLASH_CSN_BIT;
}

void conflash_write_disable( void )
{
	CONFLASH_PORT &= ~CONFLASH_CSN_BIT;
	conflash_writebyte( 0x04 );
	CONFLASH_PORT |= CONFLASH_CSN_BIT;
}

void conflash_read_bytes( uint32_t address, unsigned char *dest, uint16_t dataSize )
{
	uint16_t i = 0;

	if ( dataSize == 0 )
		return;

	CONFLASH_PORT &= ~CONFLASH_CSN_BIT;

	conflash_writebyte( 0x03 );
	conflash_writebyte( (uint8_t)((address >> 16) & 0xFF) );
	conflash_writebyte( (uint8_t)((address >> 8) & 0xFF) );
	conflash_writebyte( (uint8_t)((address & 0xFF)) );

	for ( i = 0; i < dataSize; i++ )
		dest[i] = conflash_readbyte();

	CONFLASH_PORT |= CONFLASH_CSN_BIT;
}
