#ifndef __APP_UART_H__
#define __APP_UART_H__

#include <stdint.h>

void init_app_uart( void );
void app_uart_write( uint8_t *data, uint16_t dataLen );
void app_uart_process_line( volatile uint8_t *pLine );
void app_uart_run( void );

#endif // __APP_UART_H__
