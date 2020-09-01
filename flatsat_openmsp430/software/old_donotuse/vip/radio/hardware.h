#ifndef MAIN_H
#define MAIN_H
 
#include "omsp_radio.h"
 
//--------------------------------------------------
// Hardware UART register address mapping
//--------------------------------------------------
 
#define UART_CTL          (*(volatile unsigned char *) 0x0080)  // UART Control register (8bit)
#define UART_STAT         (*(volatile unsigned char *) 0x0081)  // UART Status register (8bit)
#define UART_BAUD         (*(volatile unsigned int  *) 0x0082)  // UART Baud rate configuration (16bit)
#define UART_TXD          (*(volatile unsigned char *) 0x0084)  // UART Transmit data register (8bit)
#define UART_RXD          (*(volatile unsigned char *) 0x0085)  // UART Receive data register (8bit)

//--------------------------------------------------
// Hardware UART register address mapping
//--------------------------------------------------
 
#define APP_UART_CTL          (*(volatile unsigned char *) 0x0088)  // UART Control register (8bit)
#define APP_UART_STAT         (*(volatile unsigned char *) 0x0089)  // UART Status register (8bit)
#define APP_UART_BAUD         (*(volatile unsigned int  *) 0x008A)  // UART Baud rate configuration (16bit)
#define APP_UART_TXD          (*(volatile unsigned char *) 0x008C)  // UART Transmit data register (8bit)
#define APP_UART_RXD          (*(volatile unsigned char *) 0x008D)  // UART Receive data register (8bit)

//---------------------------------------------------
// TOKEN address for reading by the app processor (and scoring on them)
//---------------------------------------------------
#define TOKEN_0_ADDR        (*(volatile unsigned int *) 0x00A0)  // KEY higher 16-bits
#define TOKEN_1_ADDR        (*(volatile unsigned int *) 0x00A2)  // KEY middle higher 16-bits
#define TOKEN_2_ADDR        (*(volatile unsigned int *) 0x00A4)  // KEY middle lower 16-bits
#define TOKEN_3_ADDR        (*(volatile unsigned int *) 0x00A6)  // KEY Lower 16-bits
 
 
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


#define MODE_SELECT_RUN_PIN		(0x4)
#define MODE_SELECT_PROG_PIN		(0x2)
#define MODE_SELECT_DBG_PIN		(0x1)

#define MODE_SELECT_PIN			(P1IN)
 
 
//--------------------------------------------------
// Diverse
//--------------------------------------------------
 
// BAUD = (mclk_freq/baudrate)-1
 
//#define BAUD           2083            //   9600  @20.0MHz
//#define BAUD           1042            //  19200  @20.0MHz
//#define BAUD            521            //  38400  @20.0MHz
//#define BAUD            347            //  57600  @20.0MHz
//#define BAUD            174            // 115200  @20.0MHz
//#define BAUD             87            // 230400  @20.0MHz
#define BAUD		138		// 115200 @ 16.0MHz 

#define APP_BAUD	34		// 460800 @ 16.0MHz
 
#endif // MAIN_H
