#ifndef __CDH_COMMON_H__
#define __CDH_COMMON_H__

#include "omsp_cdh.h"
 
//--------------------------------------------------
// (CONSOLE) Hardware UART register address mapping
//--------------------------------------------------
 
#define CONSOLE_UART_CTL          (*(volatile unsigned char *) 0x0080)  // UART Control register (8bit)
#define CONSOLE_UART_STAT         (*(volatile unsigned char *) 0x0081)  // UART Status register (8bit)
#define CONSOLE_UART_BAUD         (*(volatile unsigned int  *) 0x0082)  // UART Baud rate configuration (16bit)
#define CONSOLE_UART_TXD          (*(volatile unsigned char *) 0x0084)  // UART Transmit data register (8bit)
#define CONSOLE_UART_RXD          (*(volatile unsigned char *) 0x0085)  // UART Receive data register (8bit)

//--------------------------------------------------
// (APP UART aka LEON-3) UART register address mapping
//--------------------------------------------------
 
#define APP_UART_CTL          (*(volatile unsigned char *) 0x0088)  // UART Control register (8bit)
#define APP_UART_STAT         (*(volatile unsigned char *) 0x0089)  // UART Status register (8bit)
#define APP_UART_BAUD         (*(volatile unsigned int  *) 0x008A)  // UART Baud rate configuration (16bit)
#define APP_UART_TXD          (*(volatile unsigned char *) 0x008C)  // UART Transmit data register (8bit)
#define APP_UART_RXD          (*(volatile unsigned char *) 0x008D)  // UART Receive data register (8bit)

//--------------------------------------------------
// Hardware UART register field mapping
//--------------------------------------------------
 
// UART Control register fields
#define  UART_IEN_TX_EMPTY  0x80
#define  UART_IEN_TX        0x40
#define  UART_IEN_RX_OVFLW  0x20
#define  UART_IEN_RX        0x10
#define  UART_SMCLK_SEL     0x02
#define  UART_EN            0x01
 
// UART Status register fields
#define  UART_TX_EMPTY_PND  0x80
#define  UART_TX_PND        0x40
#define  UART_RX_OVFLW_PND  0x20
#define  UART_RX_PND        0x10
#define  UART_TX_FULL       0x08
#define  UART_TX_BUSY       0x04
#define  UART_RX_BUSY       0x01


//-----------------------------------------
// TODO: Add pins here
//-----------------------------------------
//#define MODE_SELECT_RUN_PIN		(0x4)
//#define MODE_SELECT_PROG_PIN		(0x2)
//#define MODE_SELECT_DBG_PIN		(0x1)

//#define MODE_SELECT_PIN			(P1IN)
 
 
//--------------------------------------------------
// Diverse
//--------------------------------------------------
 
// BAUD = (mclk_freq/baudrate)-1
 
// #define CONSOLE_UART_BAUD_VALUE		173		// 115200 @ 20.0MHz 
#define CONSOLE_UART_BAUD_VALUE		207		// 115200 @ 24.0MHz 

// #define APP_UART_BAUD_VALUE		1041		// 19200 @ 20.0MHz
#define APP_UART_BAUD_VALUE		1249		// 19200 @ 24.0MHz
 
#endif // __CDH_COMMON_H__
