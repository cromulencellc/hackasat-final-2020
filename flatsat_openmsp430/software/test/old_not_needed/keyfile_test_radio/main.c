#include "omsp_system.h"
#include "hardware.h"
#include <stdlib.h>
#include <stdio.h>

 
//--------------------------------------------------//
//                   Delay function                 //
//--------------------------------------------------//
void delay(unsigned int d) {
   while(d--) {
      nop();
      nop();
   }
}
 
//--------------------------------------------------//
//                 putChar function                 //
//            (Send a byte to the UART)             //
//--------------------------------------------------//
int putchar (int txdata) {
 
  // Wait until the TX buffer is not full
  while (UART_STAT & UART_TX_FULL);
 
  // Write the output character
  UART_TXD = txdata;
 
  return 0;
}
 
//--------------------------------------------------//
//        UART RX interrupt service routine         //
//         (receive a byte from the UART)           //
//--------------------------------------------------//
volatile char rxdata;
 
wakeup interrupt (UART_RX_VECTOR) INT_uart_rx(void) {
 
  // Read the received data
  rxdata = UART_RXD;
 
  // Clear the receive pending flag
  UART_STAT = UART_RX_PND;
 
  // Exit the low power mode
  LPM0_EXIT;
}
 
 
//--------------------------------------------------//
// Main function with init an an endless loop that  //
// is synced with the interrupts trough the         //
// lowpower mode.                                   //
//--------------------------------------------------//
int main(void) {
    int reading = 0;
    int pos = 0;
    char buf[40];
    int led = 0;
    int count = 0;
 
    WDTCTL = WDTPW | WDTHOLD;           // Init watchdog timer
 
    UART_BAUD = BAUD;                   // Init UART
    UART_CTL  = UART_EN | UART_IEN_RX;

    P2OUT = 0x01;
    P2DIR = 0xFF; 

    delay( 0xffff );
 
    printf("\r\n====== openMSP430 in action ======\r\n");   //say hello
    printf("\r\nSimple Line Editor Ready\r\n");
 
    eint();                             // Enable interrupts

    while (1)
    {
    delay( 0xffff );
    printf("\r\nSetting key:\r\n" );
    (KEY_0_ADDR) = 0xAA;
    (KEY_1_ADDR) = 0xAA;
    (KEY_2_ADDR) = 0xAA;
    (KEY_3_ADDR) = 0xAAAA + count;

    delay( 0xffff );

    printf( "Key is: %X %X %X %X\r\n", (KEY_0_ADDR), (KEY_1_ADDR), (KEY_2_ADDR), (KEY_3_ADDR) ); 
    count++; 
    } 
/*
    while (1) {                         //main loop, never ends...
 
        printf("> ");                   //show prompt
        reading = 1;
        while (reading) {               //loop and read characters
 
            LPM0;                       //sync, wakeup by irq
 
	    led++;                      // Some lighting...
	    if (led==4) {
	      led = 0;
	    }
	    P2OUT = (0x01 << led);
 
            switch (rxdata) {
                //process RETURN key
                case '\r':
                //case '\n':
                    printf("\r\n");     //finish line
                    buf[pos++] = 0;     //to use printf...
                    printf(":%s\r\n", buf);
                    reading = 0;        //exit read loop
                    pos = 0;            //reset buffer
                    break;
                //backspace
                case '\b':
                    if (pos > 0) {      //is there a char to delete?
                        pos--;          //remove it in buffer
                        putchar('\b');  //go back
                        putchar(' ');   //erase on screen
                        putchar('\b');  //go back
                    }
                    break;
                //other characters
                default:
                    //only store characters if buffer has space
                    if (pos < sizeof(buf)) {
                        putchar(rxdata);     //echo
                        buf[pos++] = rxdata; //store
                    }
            }
        }
    }
*/
}
