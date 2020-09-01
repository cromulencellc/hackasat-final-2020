#include "delay.h"

// Dumb delay code (doesn't use timers)
/**
Delay function.
*/
void nop(void)
{
}

void delay(unsigned int c, unsigned int d)
{
  volatile int i, j;
  for (i = 0; i<c; i++) {
    for (j = 0; j<d; j++) {
      nop();
      nop();
    }
  }
}

void delay_ms( unsigned int ms )
{
	delay( ms, 0x7ff );
}


extern volatile unsigned int seconds;

void delay_s (unsigned int s)
{
	unsigned int end_seconds = seconds + s;
	while (seconds != end_seconds);
}

