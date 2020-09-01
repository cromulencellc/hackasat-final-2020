#include "hardware.h"
#include "lcd/lcd_hal.h"
#include "lcd/lcd_gfx.h"

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


void pong(void)
{

    button_up = button_down = button_back = button_next = 0;
    ballY = ballX = 0;
    ballDirectionY = ballDirectionX = 1;
    screen_width = lcd_gfx_getWidth();
    screen_height = lcd_gfx_getHeight();
    background_color = lcd_gfx_getBackgroundColor();
    ball_color = lcd_gfx_getForegroundColor();
    paddle_color =lcd_gfx_getForegroundColor();
    lcd_fillScreen( background_color);
    paddleX = 70;
    paddleY = 115;

    lcd_fillRect(paddleX, paddleY, 20,5, paddle_color);
    while(1){

        loop();

        // All buttons pressed exits pong
        if ((BUTTON_PORT & 0x0F) == 0x0F){
            return;
        }

        if ((button_up == 1) || (BUTTON_PORT & BUTTON_UP_BIT)) {
            button_up = 0;
            if(paddleX > 0) paddleX-=2;
        }
        if ((button_down == 1) || (BUTTON_PORT & BUTTON_DOWN_BIT)){
            button_down = 0;
            if (paddleX < screen_width - 20) paddleX +=2;
        }
        if ((button_back == 1) || (BUTTON_PORT & BUTTON_BACK_BIT)){
            button_back = 0;
            if(paddleY > 1) paddleY-=2;
        }
        if ((button_next == 1) || (BUTTON_PORT & BUTTON_NEXT_BIT)){
            button_next = 0;
            if(paddleY + 5 < screen_height - 1) paddleY+=2;
        }    

        LPM0;
        
    }

}
