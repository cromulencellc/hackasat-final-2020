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
    int i;
    int reading = 0;
    int pos = 0;
    char buf[40];
    char local_copy;
    int led = 0;
 
    WDTCTL = WDTPW | WDTHOLD;           // Init watchdog timer
 
    UART_BAUD = BAUD;                   // Init UART
    UART_CTL  = UART_EN | UART_IEN_RX;

    P2OUT = 0x01;
    P2DIR = 0xFF; 

    buf[0] = 't';
    buf[1] = 'e';
    buf[2] = 's';
    buf[3] = 't';
    buf[4] = 0;

    putchar( '\r' );
    putchar( '\n' );
    putchar( ':' );
    for ( i = 0; i < 4; i++ )
	    putchar( buf[i] );

    putchar('\r');
    putchar('\n');

    putchar(buf[0]);
    putchar(buf[1]);
    putchar(buf[2]);
    putchar(buf[3]);
    putchar('\r');
    putchar('\n');

    putchar('+');
    putchar('t');
    putchar('e');
    putchar('s');
    putchar('t');
    putchar('\r');
    putchar('\n');
 
    printf("\r\n====== openMSP430 in action ======\r\n");   //say hello
    printf("\r\nSimple Line Editor Ready\r\n");

    printf( "\r\n%s\r\n", buf );
 
    eint();                             // Enable interrupts

    while (1) {                         //main loop, never ends...
 
        printf("> ");                   //show prompt
        reading = 1;
        while (reading) {               //loop and read characters
 
            LPM0;                       //sync, wakeup by irq
	    local_copy = rxdata;
 
	    led++;                      // Some lighting...
	    if (led==4) {
	      led = 0;
	    }
	    P2OUT = (0x01 << led);
 
            switch (local_copy) {
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
                        putchar(local_copy);     //echo
                        buf[pos++] = local_copy; //store
                    }
            }
        }
    }
}
