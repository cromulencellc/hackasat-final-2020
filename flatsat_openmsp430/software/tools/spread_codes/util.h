#ifndef __UTIL_H__
#define __UTIL_H__

#include <sstream>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static const int NO_SOCKET = -1;

#define MAKE_STRING(msg) \
	(((std::ostringstream&) (std::ostringstream() << std::boolalpha << msg)).str())

uint32_t ReadDevURandomSeed( void );
std::string ReadKeyFile( void );

unsigned int random_in_range( unsigned int min, unsigned int max );

#endif __UTIL_H__
