#include <stdio.h>
#include "hardware.h"
#include "delay.h"
#include "lcd/lcd_hal.h"
#include "lcd/lcd_gfx.h"
/**
This one is executed onece a second. it counts seconds, minues, hours - hey
it shoule be a clock ;-)
it does not count days, but i think you'll get the idea.
*/
volatile int irq_counter, offset;
volatile uint8_t button_next, button_up, button_down, button_back;

// variables for the position of the ball and paddle
uint8_t paddleX = 0;
uint8_t paddleY = 0;
uint8_t oldPaddleX,oldPaddleY = 0;
uint8_t ballDirectionX = 1;
uint8_t ballDirectionY = 1;
uint16_t background_color, ball_color, paddle_color;
uint8_t screen_width, screen_height;
uint8_t ballX,ballY, oldBallX, oldBallY = 0;
uint8_t score = 0;

wakeup interrupt (WDT_VECTOR) INT_Watchdog(void) {
 
  irq_counter++;
  if (irq_counter == 300) {
    irq_counter = 0;
    offset = (offset+1) % 20;
  }
}
 

int putchar (int txdata) {
    while (UART_STAT & UART_TX_FULL);
    UART_TXD = txdata;
    return 0;
}

volatile uint8_t timer_count;
volatile int seconds;

interrupt (TIMERA0_VECTOR) TimerA_Interrupt( void )
{
    timer_count++;
    
    if ((timer_count % 2) ==  0){
        LPM0_EXIT;
    }

    if (timer_count == 100){
        seconds+=1;
        timer_count = 0;
    }

}

//----------------------------------------------------//
// PORT1 Interrupt -- Button Depress
//----------------------------------------------------//
interrupt (PORT1_VECTOR) INT_button(void)
{
    if ( BUTTON_FLAG & BUTTON_NEXT_BIT )
    {
        button_next =1;
        BUTTON_FLAG ^= BUTTON_NEXT_BIT;
    }
    if (BUTTON_FLAG & BUTTON_DOWN_BIT){
        button_down = 1;
        BUTTON_FLAG ^= BUTTON_DOWN_BIT;
    }
    if ( BUTTON_FLAG & BUTTON_UP_BIT ){
        button_up = 1;
        BUTTON_FLAG ^= BUTTON_UP_BIT;
    }
    if (BUTTON_FLAG & BUTTON_BACK_BIT){
        button_back = 1;
        BUTTON_FLAG ^= BUTTON_BACK_BIT;
    }

    LPM0_EXIT;
}



void init(void)
{ 
    WDTCTL = WDTPW | WDTHOLD;          // Disable watchdog timer
 
    UART_BAUD = BAUD;                   // Init UART
    UART_CTL = UART_EN;                 //Enable UART Output
    
    P1OUT  = 0x00;                     // Port data output
    P2OUT  = 0x00;
 
    P1DIR  = 0x00;                     // Port direction register
    P2DIR  = 0xff;
 
    P1IES  = 0x00;                     // Port interrupt enable (0=dis 1=enabled)
    P2IES  = 0x00;
    P1IE   = 0x0F;                     // Port interrupt Edge Select (0=pos 1=neg)
    P2IE   = 0x00;
 
    //WDTCTL = WDTPW | WDTTMSEL | WDTCNTCL;// | WDTIS1  | WDTIS0 ;          // Configure watchdog interrupt
    
    timer_count = seconds = 0;

    CCR0 = 20000; 
    CCTL0 |= CCIE;
    TACTL = ID_3 | TASSEL_2 | MC_1; // ACLK, upmode 
    //IE1 |= 0x01;

    eint();                            //enable interrupts
    
    lcd_init_screen( INITR_BLACKTAB );
    lcd_init_gfx( 160, 128 );

    lcd_fillScreen( ST7735_BLACK );
}

void testprint(char *s)
{
	printf("%s\r", s);
	lcd_gfx_print(s);
}


// this function determines the ball's position on screen
void moveBall() {
    // if the ball goes offscreen, reverse the direction:
    if (ballX + 6 > screen_width){
        ballDirectionX = -1;
    }
    else if (ballX <= 0) {
        ballDirectionX = 1;
    }

    if (ballY <= 0){
        ballDirectionY = 1;
    }

    if (ballY > screen_height) {
        // You lost. reset game
        lcd_gfx_setCursor(10, 70);
        ballY = ballX = 0;    
        ballDirectionY = ballDirectionX = 1;
        paddleX = 70;
        paddleY = 115;

    }  

    // update the ball's position
    ballX += ballDirectionX;
    ballY += ballDirectionY;

    // check for veritical hit
    if ((ballX + 4 > paddleX) && (ballX < paddleX + 20) &&
        ((ballY + 4 >= paddleY) && (ballY <= paddleY + 4))) {
        ballDirectionY = -ballDirectionY;
        ballY += ballDirectionY;
    }
    // check for horizontal hit
    if ((ballY + 4 >= paddleY) && (ballY <= paddleY + 4) &&
        ((ballX + 4 == paddleX) || (ballX == paddleX +20))){
        ballDirectionX = -ballDirectionX;
        ballX += ballDirectionX;
    }
    
    // erase the ball's previous position
   
    if (oldBallX != ballX || oldBallY != ballY) {
        lcd_fillRect(oldBallX, oldBallY, 5,5, background_color);
        lcd_fillRect(ballX, ballY, 5,5, ball_color);
    }


    oldBallX = ballX;
    oldBallY = ballY;

}


void loop() {
  

  // set the fill color to black and erase the previous 
  // position of the paddle if different from present
    if (oldPaddleX != paddleX || oldPaddleY != paddleY) {
        lcd_fillRect(oldPaddleX, oldPaddleY, 20,5, background_color);
        lcd_fillRect(paddleX, paddleY, 20,5, paddle_color);
    }

    oldPaddleX = paddleX;
    oldPaddleY = paddleY;

    moveBall();

}


int main(void)
{

    init();
    irq_counter = 0;
    offset      = 0;
    button_up = button_down = button_back = button_next = 0;
    ballY = ballX = 0;
    ballDirectionY = ballDirectionX = 1;
    screen_width = lcd_gfx_getWidth();
    screen_height = lcd_gfx_getHeight();
    background_color = COLOR_LEGIT_PURPLE;
    ball_color = COLOR_LEGIT_ORANGE;
    paddle_color = COLOR_LEGIT_ORANGE;
    lcd_fillScreen( background_color);
    paddleX = 70;
    paddleY = 115;

    lcd_fillRect(paddleX, paddleY, 20,5, paddle_color);
    while(1){

        loop();

        if ((button_up == 1) || (BUTTON_PORT & BUTTON_UP_BIT)) {
            button_up = 0;
            if(paddleY > 1) paddleY-=2;
        }
        if ((button_down == 1) || (BUTTON_PORT & BUTTON_DOWN_BIT)){
            button_down = 0;
            if(paddleY + 5 < screen_height - 1) paddleY+=2;
        }
        if ((button_back == 1) || (BUTTON_PORT & BUTTON_BACK_BIT)){
            button_back = 0;
            if (paddleX < screen_width - 20) paddleX +=2;
        }
        if ((button_next == 1) || (BUTTON_PORT & BUTTON_NEXT_BIT)){
            button_next = 0;
            if(paddleX > 0) paddleX-=2;
        }    

        LPM0;
        
    }

}
