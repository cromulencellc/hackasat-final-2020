// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
#ifndef __UTIL_H__
// ----------------------------------------------------------------------------
#define __UTIL_H__
// ----------------------------------------------------------------------------
#include <stdint.h>
#include <stdio.h>
// ----------------------------------------------------------------------------
void seed_quick_rand( uint16_t new_seed );
uint16_t get_quick_rand( void );
// ----------------------------------------------------------------------------
#define CIRC_BUF_LEN   (2052*2)   // we have so much ram 

typedef struct {
  uint8_t   buf[CIRC_BUF_LEN];
  uint16_t  in;
  uint16_t  out;
  uint16_t  count;
} t_circbuf;

void bufClear(t_circbuf * buf); // empty the buffer
int bufRem(t_circbuf * buf);  // returns buf free space

int bufAddByte(t_circbuf * buf, uint8_t b); // ret -1 if fail else pass
int bufGetByte(t_circbuf * buf);            // ret -1 if fail else byte

int bufAdd(t_circbuf * buf, uint8_t * src, uint16_t len);   // returns ct bytes added or -1
int bufGet(t_circbuf * buf, uint8_t * dest, uint16_t len);  // removes/returns bytes from queue
int bufPeek(t_circbuf * buf, uint8_t * dest, uint16_t len);  // returns bytes from queue
int bufPeekByte(t_circbuf * buf, int idx);    // returns byte at index
// ----------------------------------------------------------------------------
uint8_t read_hex_uint8( char *string );
uint8_t int_to_hex_char( uint8_t nibble );
uint8_t hex_to_int( uint8_t dataIn );
uint8_t int_to_hex( uint8_t dataIn );
void    strCatOctet(char * dest,uint8_t b);

// ----------------------------------------------------------------------------
void xtea_encrypt(volatile uint32_t w[2], const uint32_t k[4]);
void xtea_decrypt(volatile uint32_t w[2], const uint32_t k[4]);
// ----------------------------------------------------------------------------
#endif // __UTIL_H__
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
