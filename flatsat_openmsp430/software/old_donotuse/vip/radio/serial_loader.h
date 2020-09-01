#ifndef __SERIAL_LOADER_H__
#define __SERIAL_LOADER_H__

#include <stdio.h>

void init_serial_loader( void );
void process_serial_loader_line( unsigned char *line );

#endif // __SERIAL_LOADER_H__
