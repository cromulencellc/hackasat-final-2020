#ifndef __RADIO_HAL_H__
#define __RADIO_HAL_H__

#include <stdio.h>

void radio_reset( void );
void radio_spi_init( void );
void radio_spi_writebyte( uint8_t data );
uint8_t radio_spi_readbyte( void );
void radio_writereg( uint8_t regNum, uint8_t value );
void radio_writefifo( uint8_t *data, unsigned int count );
void radio_readfifo( uint8_t *data, unsigned int count );
uint8_t radio_readreg( uint8_t regNum );

#endif // __RADIO_HAL_H__
