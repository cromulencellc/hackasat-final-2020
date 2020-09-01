#ifndef __CONFLASH_H__
#define __CONFLASH_H__
#include <stdio.h>

//============================================================================
// CONFLASH SPI Controller
//============================================================================
#define CONFLASHSPI_CTRL            (*(volatile unsigned int *)0x0098)      // CTRL register address 
#define CONFLASHSPI_DATA            (*(volatile unsigned int *)0x009A)      // Data register (8-bit or 16-bits)

#define CONFLASHSPI_CTRL_SMCLK      (0x0200)        // SMCLK select
#define CONFLASHSPI_CTRL_SENDCNT    (0x0100)        // Send count (1 = 16-bits, 0 = 8 bits)
#define CONFLASHSPI_CTRL_CLOCKDIV   (0x00E0)        // Clock divider (power of 2)
#define CONFLASHSPI_CTRL_IEN        (0x0010)        // Interrupt enable
#define CONFLASHSPI_CTRL_IFLG       (0x0008)        // Interrupt flag
#define CONFLASHSPI_CTRL_CKPH       (0x0004)        // Clock Phase
#define CONFLASHSPI_CTRL_CKPOL      (0x0002)        // Clock Polarity
#define CONFLASHSPI_CTRL_EN         (0x0001)        // Enable

#define CONFLASH_PORT           P2OUT
#define CONFLASH_CSN_BIT        (0x02)

void conflash_init( void );
void conflash_writebyte( unsigned char byte );
unsigned char conflash_readbyte( void );
void conflash_ss_erase( uint32_t address );
void conflash_page_program( uint32_t address, uint8_t *pageData, uint16_t dataSize );
void conflash_page_write( uint32_t address, uint8_t *pageData, uint16_t dataSize );
unsigned char conflash_wip_check( void );
void conflash_write_enable( void );
void conflash_write_disable( void );
void conflash_read_bytes( uint32_t address, unsigned char *dest, uint16_t destSize );
uint32_t conflash_readid( void );
uint8_t conflash_read_status( void );

#endif // __CONFLASH_H__
