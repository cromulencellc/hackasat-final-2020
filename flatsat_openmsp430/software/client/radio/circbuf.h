// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
#ifndef __CIRCBUF_H__
// ----------------------------------------------------------------------------
#define __CIRCBUF_H__
// ----------------------------------------------------------------------------
#include <stdint.h>
#include <stdio.h>
// ----------------------------------------------------------------------------
#define CIRC_BUF_LEN   (1024+1024)

typedef struct {
  uint8_t   buf[CIRC_BUF_LEN];
  uint16_t  in;
  uint16_t  out;
  uint16_t  count;
} t_circbuf;

void bufClear(t_circbuf * buf); // empty the buffer
uint16_t bufRem(t_circbuf * buf);  // returns buf free space

int bufAddByte(t_circbuf * buf, uint8_t b); // ret -1 if fail else pass
int bufGetByte(t_circbuf * buf);            // ret -1 if fail else byte

int bufAdd(t_circbuf * buf, uint8_t * src, uint16_t len);   // returns ct bytes added or -1
uint16_t bufGet(t_circbuf * buf, uint8_t * dest, uint16_t len);  // removes/returns bytes from queue
uint16_t bufPeek(t_circbuf * buf, uint8_t * dest, uint16_t len);  // returns bytes from queue

uint8_t bufPeekByte(t_circbuf * buf, uint16_t idx);    // returns byte at index

// ----------------------------------------------------------------------------
#endif // __CIRCBUF_H__
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
