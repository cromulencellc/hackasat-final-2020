#ifndef __RAND_H__
#define __RAND_H__

#include <stdint.h>

void seed_quick_rand( uint16_t new_seed );
uint16_t get_quick_rand( void );

#endif // __RAND_H__
