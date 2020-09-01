#include "util.h"

uint8_t hex_char_to_int( char value )
{
        if ( value >= '0' && value <= '9' )
                return (value - '0');
        else if ( value >= 'A' && value <= 'F' )
                return (10 + (value - 'A'));
        else if ( value >= 'a' && value <= 'f' )
                return (10 + (value - 'a'));
        else
                return 0;
}

uint8_t read_hex_uint8( char *hexString )
{
        int i = 0;
        uint8_t value = 0;

        for ( i = 0; i < 2; i++ )
                value = value | (hex_char_to_int( hexString[i] ) << ((1-i)*4));

        return value;
}

