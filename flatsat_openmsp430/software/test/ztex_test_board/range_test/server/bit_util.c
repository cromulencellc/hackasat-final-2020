#include "bit_util.h"

// NOT INTERRUPT SAFE (uses static local variables)
void text_writebit( uint16_t bit_data, int16_t n, uint8_t *data, uint16_t *pos )
{
        int16_t i;
        static uint8_t bit_buffer = 0, bit_count = 0;

        if ( n == 0 && bit_count > 0 )
        {
                while ( bit_count < 8 )
                {
                        bit_count++;
                        bit_buffer <<=1;
                }

                // FLUSH
                data[*pos++] = bit_buffer;
        }
        else
        {
                for ( i = 0; i < n; i++ )
                {
                        if ( bit_data & (1<<((n-1)-i)) )
                                bit_buffer++;

                        bit_count++;
                        if ( bit_count == 8 )
                        {
                                data[(*pos)++] = bit_buffer;

                                bit_buffer = 0;
                                bit_count = 0;
                        }

                        bit_buffer <<= 1;
                }
        }
}

// NOT INTERRUPT SAFE (uses static local variables)
int16_t text_getbit( int16_t n, uint8_t *data, uint16_t *pos, uint16_t maxLen )
{
        int16_t i, x;
        static uint8_t buf, mask = 0;

        x = 0;
        for (i = 0; i < n; i++)
        {
                if (mask == 0)
                {
                        if ( (*pos) >= maxLen )
                                return (-1);

                        buf = data[(*pos)++];

                        mask = 128;
                }
                x <<= 1;

                if (buf & mask)
                        x++;

                mask >>= 1;
        }
        return x;
}


