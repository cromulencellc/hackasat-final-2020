#include "hardware.h"
#include "lcd/lcd_hal.h"
#include "lcd/lcd_gfx.h"
#include "vip.h"

// variables for the position of the ball and paddle
uint8_t leftPaddleY = 0;
uint8_t rightPaddleY = 0;
uint8_t oldLeftPaddleY = 0;
uint8_t oldRightPaddleY = 0;
uint8_t ball2DirectionX = 1;
uint8_t ball2DirectionY = 1;
uint16_t background2_color, ball2_color, paddle2_color;
uint8_t screen_width, screen_height;
uint8_t ball2X,ball2Y, oldball2X, oldball2Y = 0;
uint8_t pong_score2 = 0;

// this function determines the ball's position on screen
static void moveBall2() {
    // if the ball goes offscreen, reverse the direction

    if ((ball2Y <= 0) || (ball2Y + 4 >= screen_height)){
        ball2DirectionY = -ball2DirectionY;
    }

    if (ball2X <= 0) {
        // You lost. reset game
        ball2Y = 60;
        ball2X = 160/2;   
        ball2DirectionY = 1;
        ball2DirectionX = 1;
        leftPaddleY = 60;
        rightPaddleY = 60;

    }  

    if (ball2X > screen_width) {
        // You lost. reset game
        ball2Y = 60;
        ball2X = 160/2;   
        ball2DirectionY = 1;
        ball2DirectionX = -1;
        leftPaddleY = 60;
        rightPaddleY = 60;

    }  
    // update the ball's position
    ball2X += ball2DirectionX;
    ball2Y += ball2DirectionY;

    // check for left hit
    if ((ball2X <= 5) &&
        ((ball2Y + 4 >= leftPaddleY) && (ball2Y <= leftPaddleY + 20))) {
        ball2DirectionX = -ball2DirectionX;
        ball2Y += ball2DirectionY;
    }
    // check for right hit
    if ((ball2Y + 4 >= rightPaddleY) && (ball2Y <= rightPaddleY + 20) &&
        (ball2X + 4 >= 160-5) ){
        ball2DirectionX = -ball2DirectionX;
        ball2X += ball2DirectionX;
    }
    
    // erase the ball's previous position
   
    if (oldball2X != ball2X || oldball2Y != ball2Y) {
        lcd_fillRect(oldball2X, oldball2Y, 5,5, background2_color);
        lcd_fillRect(ball2X, ball2Y, 5, 5, ball2_color);
    }


    oldball2X = ball2X;
    oldball2Y = ball2Y;

}


static void loop2() {
  

  // set the fill color to black and erase the previous 
  // position of the paddle if different from present
    if (oldLeftPaddleY != leftPaddleY) {
        lcd_fillRect(0, oldLeftPaddleY, 5,20, background2_color);
        lcd_fillRect(0, leftPaddleY, 5,20, paddle2_color);
    }
    if (oldRightPaddleY != rightPaddleY) {
        lcd_fillRect(160-5, oldRightPaddleY, 5,20, background2_color);
        lcd_fillRect(160-5, rightPaddleY, 5,20, paddle2_color);
    }

    oldLeftPaddleY = leftPaddleY;
    oldRightPaddleY = rightPaddleY;

    moveBall2();

}


uint8_t pong2(uint8_t refresh)
{

    if (refresh)
    {    
        button_up = button_down = button_back = button_next = 0;
        ball2Y = 60;
        ball2X = 160/2;
        ball2DirectionY = ball2DirectionX = 1;
        screen_width = lcd_gfx_getWidth();
        screen_height = lcd_gfx_getHeight();
        background2_color = lcd_gfx_getBackgroundColor();
        ball2_color = lcd_gfx_getForegroundColor();
        paddle2_color =lcd_gfx_getForegroundColor();
        lcd_fillScreen( background2_color);
        leftPaddleY = 60;
        rightPaddleY = 60;

        lcd_fillRect(0, leftPaddleY, 5,20, paddle2_color);
        lcd_fillRect(160-5, rightPaddleY, 5,20, paddle2_color);
    }

    loop2();

    if ((button_up == 1) || (BUTTON_PORT & BUTTON_UP_BIT)) {
        button_up = 0;
        if(leftPaddleY + 20 < screen_height) leftPaddleY+=2;
    }
    if ((button_down == 1) || (BUTTON_PORT & BUTTON_DOWN_BIT)){
        button_down = 0;
        if (rightPaddleY < screen_height - 20) rightPaddleY +=2;
    }
    if ((button_back == 1) || (BUTTON_PORT & BUTTON_BACK_BIT)){
        button_back = 0;
        if(rightPaddleY > 1) rightPaddleY-=2;
    }
    if ((button_next == 1) || (BUTTON_PORT & BUTTON_NEXT_BIT)){
        button_next = 0;
        if(leftPaddleY > 1) leftPaddleY-=2;
    }   

    return 1;    

}
