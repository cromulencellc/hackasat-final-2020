#include "hardware.h"
#include "lcd/lcd_hal.h"
#include "lcd/lcd_gfx.h"
#include "delay.h"
#include "computer.h"
#include <string.h>
#include <stdio.h>

extern uint8_t button_poll;
const uint16_t up_arrow = 0x001e;
const uint16_t down_arrow = 0x001f;

static char *teams[]={
    "Plaid Parliament of Pwning\n",
    "9447\n",
    "Reckless Abandon\n",
    "Routards\n",
    "raon_ASRT\n",
    "KAIST GoN\n",
    "shellphish\n",
    "CodeRed\n",
    "HITCON\n",
    "blue-lotus\n",
    "HackingForChiMac\n",
    "(Mostly) Men in Black Hats\n",
    "w3stormz\n",
    "More Smoked Leet Chicken\n",
    "Dragon Sector\n",
    "[SEWorks]penthackon\n",
    "Stratum Auhuur\n",
    "Gallopsled\n",
    "BalalaikaCr3w\n",
    "binja\n"
};

static char *leadteams[]={
    "\n   PPP \n",
    "\n   9447 \n",
    " Reckless\n Abandon \n",
    "\n Routards\n",
    "\n raon_ASRT\n",
    "  KAIST\n   GoN\n",
    "\nshellphish\n",
    "\n CodeRed\n",
    "\n  HITCON\n",
    "\nblue-lotus\n",
    "HackingFor\n  ChiMac \n",
    "\n (M) MIBH\n",
    "\n w3stormz\n",
    "More Smokd\nLeet Chckn\n",
    "  Dragon\n  Sector\n",
    "[SEWorks]\npenthackon\n",
    " Stratum\n  Auhuur\n",
    "\nGallopsled\n",
    " Balalaika\n   Cr3w\n",
    "\n  binja\n"
};

enum {
    SPLASH_TITLE,
    SCOREBOARD_TITLE,
    SCOREBOARD_UNAVAILABLE,
    SCOREBOARD_OUTDATED,
    SCOREBOARD_HIDDEN,
    LEADERBOARD_TITLE,
    LEADERBOARD_UNAVAILABLE,
    NEWS_TITLE,
    NUM_MESSAGES
};

#define MSG_LENGTH  64

char messages[NUM_MESSAGES][MSG_LENGTH]={
    "DEF CON 22 CTF",
    "DEF CON 22 CTF",
    "\nScoreboard Not Available\nProceed to the CTF Room\n",
    "\nScoreboard Stale\nProceed to the CTF Room\n",
    "\nScoreboard Hidden Until Closing Ceremony\n",
    "IN THE LEAD\n\n",
    "\n  Leaderboard\n Not Available\n",
    "   CTF NEWS"
};

char testmsg[10][16]={
    "ONE",
    "TWO",
    "THREE",
    "FOUR",
    "FIVE",
    "SIX",
    "SEVEN",
    "EIGHT",
    "NINE",
    "TEN"
};

#define MAX_NEWS  128
#define NEWS_LENGTH  27

struct news_s{
    uint8_t valid;
    uint16_t id;
    char msg[NEWS_LENGTH];
};

struct news_s news[MAX_NEWS];
int16_t oldest_news, latest_news;

static uint8_t contra[] = {4,3,4,3,2,2,1,1};
uint8_t ctfscore[]={0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19};
const uint8_t TEAMCOUNT = 20;
uint8_t score_valid = 0;
unsigned int score_timeout = 0;

extern volatile uint8_t sequence[8];
extern volatile uint8_t button_next, button_up, button_down, button_back;
extern volatile int seconds;
uint16_t score_textcolor, score_bgcolor;



void printnames(uint8_t index)
{ 
   
    uint8_t loop;

    lcd_gfx_setTextWrap(0);
    lcd_gfx_setCursor(0,21);
    if (score_valid)
    {
        if (index > 0)
        {
            lcd_gfx_setCursor(0,21);
            lcd_gfx_print((char *)&up_arrow);
            lcd_gfx_print("\n");
        }
        else
        {
            lcd_gfx_print("\n");
        }
        lcd_gfx_setWidth(160);
        for(loop = 0; loop < 4; loop++)
        {
            lcd_gfx_print(teams[ctfscore[index+loop]]);
        }
        lcd_gfx_setWidth(96);
        for(loop = 4; loop < 10; loop++)
        {
            lcd_gfx_print(teams[ctfscore[index+loop]]);
        }
        if (index < 10)
        {
            lcd_gfx_print((char *)&down_arrow);
        }
        else
        {
            lcd_gfx_print("\n");
        } 
    }
    else
    {
        lcd_gfx_print(messages[SCOREBOARD_UNAVAILABLE]);
    }
    lcd_gfx_setWidth(160);
    lcd_gfx_setTextWrap(1);
}

void printnews(int16_t index)
{
    int16_t current_news;
    uint8_t line_count;
    char num[10];

    lcd_gfx_setCursor(0,21);

    // If newer news exists
    if (index != latest_news)
    {
        lcd_gfx_setCursor(0,21);
        lcd_gfx_print((char *)&up_arrow);
        lcd_gfx_print("\n");
    }
    else
    {
        lcd_gfx_print("\n");
    }

    current_news = index;
    line_count = 0;
    while(current_news != oldest_news && line_count < 10)
    {
       
        if (news[current_news].valid)
        {
            sprintf(num, "%d:%d", current_news, news[current_news].id);
            lcd_gfx_print(num);
            lcd_gfx_print(news[current_news].msg);
            lcd_gfx_print("\n");
            line_count ++;
        }
        current_news--;
        if(current_news < 0) current_news = MAX_NEWS - 1;
    }
    if ((current_news == oldest_news) && (line_count < 10))
    {
        lcd_gfx_print(news[current_news].msg);
        while(line_count < 11)
        {
            lcd_gfx_print("\n");
            line_count++;
        }
    }
    else
    {
        lcd_gfx_print((char *)&down_arrow);
    }
 
}

void newsscreen(void)
{
    // Display news header and printnews
    lcd_gfx_setTextSize(2);
    lcd_fillScreen(lcd_gfx_getBackgroundColor());
    lcd_gfx_setCursor(3, 5);
    lcd_gfx_print(messages[NEWS_TITLE]);
    lcd_gfx_setTextSize(1);
    lcd_gfx_print("\n"); 
    printnews(latest_news);
}

void splashscreen(void)
{

    lcd_fillScreen(lcd_gfx_getBackgroundColor());
    lcd_gfx_drawBitmap16(16,  15,  computer_bitmap, computer_palette, 64, 64, 2) ;
    lcd_gfx_setCursor(3,5);
    lcd_gfx_setTextSize(2);
    lcd_gfx_print(messages[SPLASH_TITLE]);
}

void leaderboard(uint8_t index)
{
    lcd_gfx_setTextSize(2);
    lcd_fillScreen(lcd_gfx_getBackgroundColor());
    lcd_gfx_setCursor(20, 5);
    lcd_gfx_print(messages[LEADERBOARD_TITLE]);

    if (score_valid)
    {
        lcd_gfx_setTextSize(3);
        lcd_gfx_setTextColor(COLOR_LEGIT_WHITE, lcd_gfx_getBackgroundColor());
        lcd_gfx_print(leadteams[ctfscore[index]]); 
    }
    else
    {
        lcd_gfx_setTextSize(2);
        lcd_gfx_print(messages[LEADERBOARD_UNAVAILABLE]);
    }

    lcd_gfx_setTextColor(lcd_gfx_getForegroundColor(), lcd_gfx_getBackgroundColor());
    lcd_gfx_setTextSize(2);
}

void scorescreen()
{

    lcd_gfx_setTextSize(2);
    lcd_fillScreen(lcd_gfx_getBackgroundColor());
    lcd_gfx_setCursor(3, 5);
    lcd_gfx_print(messages[SCOREBOARD_TITLE]);
    lcd_gfx_setTextSize(1);
    lcd_gfx_print("\n"); 
    lcd_gfx_drawBitmap16(96,  64,  computer_bitmap, computer_palette, 64, 64, 1) ;
    printnames(0);
}

void update_score(uint8_t newscore[])
{
    uint8_t i;
    for(i=0; i<TEAMCOUNT; i++)
    {
        ctfscore[i] = newscore[i];
        score_valid = 1;
        score_timeout = seconds + 60*60; 
    }
}

void update_message(uint8_t msg_id, char *new_message, uint8_t new_len)
{
    uint8_t length = new_len;
    uint8_t i;
    if (msg_id >= NUM_MESSAGES)
    {
        return;
    }
    if (new_len > MSG_LENGTH - 1)
        length = MSG_LENGTH - 1;
    for(i=0; i<=length; i++)
    {
        messages[msg_id][i] = new_message[i];
    }
    messages[msg_id][i] = 0;
}

void update_news(int16_t news_id, char *new_news, uint8_t new_len)
{
    uint8_t length = new_len;
    uint16_t i, diff;
    int16_t current_news;

    printf("Update News: %d - %s (latest: %d, oldest:%d)\n\r", news_id, new_news, latest_news, oldest_news);
    // Do not store really old news
    if (news_id < news[oldest_news].id)
    {
        return;
    }
    // Advance queue to latest 
    if (news_id > news[latest_news].id)
    {
        diff = news_id - news[latest_news].id;
        for (i = 0; i< diff; i++)
        {
            // Advance news
            latest_news++;
            if (latest_news == MAX_NEWS) latest_news = 0;
            if(oldest_news==latest_news)
            {
                oldest_news++;
                if (oldest_news == MAX_NEWS) oldest_news = 0;
            }
            news[latest_news].valid = 0;

        }
        current_news = latest_news;
    }
    else
    // Find correct location within queue for this message
    {
        diff = news[latest_news].id - news_id;
        current_news = latest_news;
        while (diff > 0)
        {
            current_news--;
            if (current_news < 0) current_news = MAX_NEWS - 1;
            diff--;
        }   

    }
    // Copy current message
    if (new_len > NEWS_LENGTH - 1)
    {
        length = NEWS_LENGTH - 1;
    }
    news[current_news].id = news_id;
    for (i = 0; i < length; i++)
    {
        news[current_news].msg[i] = new_news[i];
    }
    news[current_news].msg[i] = 0;
    news[current_news].valid = 1;
    
}

void scoreboard(void){
    uint8_t index = 0;
    int16_t news_index = 0;
    enum screens { SPLASH, SCORE, LEADER, NEWS, NUM_SCREENS };
    enum screens screen_state = NEWS;
    int timeout;
    uint8_t refresh = 1;
    button_up = button_down = button_back = button_next = 0;
    lcd_gfx_setTextColor(lcd_gfx_getForegroundColor(), lcd_gfx_getBackgroundColor());
    timeout = seconds + 5; 
    uint8_t i;
    while(1) 
    {
       
        if (score_timeout == seconds)
        {
            score_valid = 0;
        }

        if (memcmp((void *)sequence, contra, 8)==0)
            break;
        
        if (button_back == 1){
            button_back = 0;
            screen_state = (screen_state - 1) % NUM_SCREENS;
            refresh = 1;
            timeout = seconds + 10;
        }
        
        if (button_next == 1){
            button_next = 0;
            screen_state = (screen_state + 1) % NUM_SCREENS;
            refresh = 1;
            timeout = seconds + 10;
        }
        
        if (timeout == seconds)
        {
            screen_state = (screen_state + 1) % NUM_SCREENS;
            refresh = 1;
            timeout = seconds + 5;
        }

        switch(screen_state)
        {
            case SPLASH:
                if (refresh)
                    splashscreen();
                break;
            case SCORE:
                if (refresh)
                    scorescreen();
                if (button_down == 1)
                {

                    timeout = seconds + 10;
                    button_down = 0;
                    if (index < 10)
                    {
                        index+=10;
                        printnames(index);
                    }
                }

                if (button_up == 1)
                {
                    timeout = seconds + 10;
                    button_up = 0;
                    if (index > 0)
                    {
                        index-=10;
                        printnames(index);
                    }
                }
                break;
            case LEADER:    
                if (refresh)
                    leaderboard(0);
                /*
                if (button_down == 1)
                {

                    timeout = seconds + 10;
                    button_down = 0;
                    if (index < 19)
                    {
                        index+=1;
                        leaderboard(index);
                    }
                }

                if (button_up == 1)
                {
                    timeout = seconds + 10;
                    button_up = 0;
                    if (index > 0)
                    {
                        index-=1;
                        leaderboard(index);
                    }
                }
                */
                break;
            case NEWS:
                if (refresh)
                {
                    newsscreen();
                    news_index = latest_news;
                }
                if ((button_down == 1) || (BUTTON_PORT & BUTTON_DOWN_BIT))
                {

                    timeout = seconds + 10;
                    button_down = 0;
                    if (news_index != oldest_news)
                    {
                        printf("news_index (%d -> %d)\r\n", news_index, news_index -1);
                        news_index-=1;
                        if (news_index<0) news_index = MAX_NEWS - 1;
                        printnews(news_index);
                    }
                }

                if ((button_up == 1) || (BUTTON_PORT & BUTTON_UP_BIT))
                {
                    timeout = seconds + 10;
                    button_up = 0;
                    if (news_index != latest_news)
                    {
                        printf("news_index (%d -> %d)\r\n", news_index, news_index +1);
                        news_index+=1;
                        if (news_index == MAX_NEWS) news_index = 0;
                        printnews(news_index);
                    }
                }
                
                break;
            default:
                break;
        }
        refresh = 0;

        button_back = button_down = button_next = button_up = 0;
        LPM0;
    } 
    lcd_gfx_setWidth(160);
    lcd_gfx_setTextWrap(1);
}
