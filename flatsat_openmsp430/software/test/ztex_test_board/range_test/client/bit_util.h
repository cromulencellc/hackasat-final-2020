#ifndef __BIT_UTIL_H__
#define __BIT_UTIL_H__
#include <stdint.h>

void text_writebit( uint16_t bit_data, int16_t n, uint8_t *data, uint16_t *pos );
int16_t text_getbit( int16_t n, uint8_t *data, uint16_t *pos, uint16_t maxLen );

#endif // __BIT_UTIL_H__
