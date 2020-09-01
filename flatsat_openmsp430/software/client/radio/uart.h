//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#ifndef __UART_H__
//-----------------------------------------------------------------------------
#define __UART_H__
//-----------------------------------------------------------------------------
#include <stdint.h>
#include "circbuf.h"
#include "radio_process.h"
#include "cdh_common.h"
#include "util.h"
#include "common.h"   					// oobm command codes and structs
//-----------------------------------------------------------------------------
// UART max line length
#define MAX_DATA_LENGTH   (2048)

extern volatile uint8_t g_leon_uart_buf_rx[MAX_DATA_LENGTH];
extern volatile uint16_t g_leon_uart_buf_rxCount;
extern volatile uint16_t g_leon_uart_buf_rxReadPos;
extern volatile uint16_t g_leon_uart_buf_rxWritePos;

extern volatile uint8_t g_leon_uart_buf_tx[MAX_DATA_LENGTH];
extern volatile uint16_t g_leon_uart_buf_txCount;
extern volatile uint16_t g_leon_uart_buf_txReadPos;
extern volatile uint16_t g_leon_uart_buf_txWritePos;

extern t_circbuf g_oobm_buf_rx;
extern t_circbuf g_oobm_buf_tx;
extern uint8_t g_temp_buf[2052];

void uartInit( void );
void uartRun();
void leonGotRadioData( volatile uint8_t *pData, uint8_t dataLen );
void leon3_uart_run( void );
//-----------------------------------------------------------------------------
#endif
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
