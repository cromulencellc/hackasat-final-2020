#include "rand.h"

uint16_t g_lastrand = 0x18F0;

void seed_quick_rand( uint16_t new_seed )
{
	g_lastrand = new_seed;
}

uint16_t get_quick_rand( void )
{
        uint16_t r;

        r = (((((((((((g_lastrand << 3) - g_lastrand) << 3)
        + g_lastrand) << 1) + g_lastrand) << 4)
        - g_lastrand) << 1) - g_lastrand) + 0xe60);

        g_lastrand = r - 1;

	return (r);
}
