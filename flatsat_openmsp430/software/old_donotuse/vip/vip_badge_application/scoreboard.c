#include "hardware.h"
#include "lcd/lcd_hal.h"
#include "lcd/lcd_gfx.h"
#include "delay.h"
#include "vip.h"
#include "computer.h"
#include <stdio.h>


uint8_t sb_index = 0;
int16_t news_index = 0;
enum sb_screens { SPLASH, NAMETAG, SCORE, LEADER, NEWS, NUM_SB_SCREENS };
enum sb_screens sb_screen_state = SPLASH;
int sb_timeout;
uint8_t sb_refresh = 1;
uint8_t i;
news_t news[MAX_NEWS];
int16_t oldest_news, latest_news;
uint8_t ctfscore[]={0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19};
uint8_t score_valid = 0;
uint16_t latest_score_id = 0;
uint32_t score_timeout = 0;

static char *teams[]={
    "Plaid Parliament of Pwning\n",
    "[CBA]9447\n",
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
    "PPP\n",
    "[CBA]9447\n",
    "Reckless\nAbandon\n",
    "Routards\n",
    "raon_ASRT\n",
    "KAIST\nGoN\n",
    "shellphish\n",
    "CodeRed\n",
    "HITCON\n",
    "blue-lotus\n",
    "HackingFor\n ChiMac\n",
    "(M) MIBH\n",
    "w3stormz\n",
    "More Smokd\nLeet Chckn\n",
    "Dragon\nSector\n",
    "[SEWorks]\npenthackon\n",
    "Stratum\nAuhuur\n",
    "Gallopsled\n",
    "Balalaika\nCr3w\n",
    "binja\n"
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
    NUM_STRINGS
};

char strings[NUM_STRINGS][VIP_STRING_LENGTH]={
    "DEF CON 22 CTF",
    "DEF CON 22 CTF",
    "\nScoreboard Not Available\nProceed to the CTF Room\n",
    "\nScoreboard Stale\nProceed to the CTF Room\n",
    "\nScoreboard Hidden Until Closing Ceremony\n",
    "IN THE LEAD\n",
    "Leaderboard\nNot Available\n",
    "CTF NEWS"
};


static void printnames(uint8_t index)
{ 
   
    uint8_t loop;

    lcd_gfx_setTextWrap(0);
    lcd_gfx_setCursor(0,21);
    if (score_valid)
    {
        if (index > 0)
        {
            lcd_gfx_setCursor(0,21);
            // Up Arrow
            lcd_gfx_print("\x001e");
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
            // Down arrow
            lcd_gfx_print("\x001f");
        }
        else
        {
            lcd_gfx_print("\n");
        } 
    }
    else
    {
        lcd_gfx_print(strings[SCOREBOARD_UNAVAILABLE]);
    }
    lcd_gfx_setWidth(160);
    lcd_gfx_setTextWrap(1);
}

static void printnews(int16_t index)
{
    int16_t current_news;
    uint8_t line_count;

    lcd_gfx_setCursor(0,21);

    // If newer news exists
    if (index != latest_news)
    {
        lcd_gfx_setCursor(0,21);
        // Up Arrow
        lcd_gfx_print("\x001e");
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
            lcd_gfx_print(news[current_news].msg);
            lcd_gfx_print("\n");
            line_count ++;
        }
        current_news--;
        if(current_news < 0) current_news = MAX_NEWS - 1;
    }
    if ((current_news == oldest_news) && (line_count < 10))
    {
        if (news[current_news].valid)
        {
            lcd_gfx_print(news[current_news].msg);
        }
        while(line_count < 11)
        {
            lcd_gfx_print("\n");
            line_count++;
        }
    }
    else
    {
        // Down Arrow
        lcd_gfx_print("\x001f");
    }
 
}

static void newsscreen(void)
{
    // Display news header and printnews
    lcd_gfx_setTextSize(2);
    lcd_gfx_setTextColor(lcd_gfx_getForegroundColor(), lcd_gfx_getBackgroundColor());
    lcd_fillScreen(lcd_gfx_getBackgroundColor());
    lcd_gfx_setCursor(3, 5);
    lcd_gfx_printCenter(strings[NEWS_TITLE]);
    lcd_gfx_setTextSize(1);
    lcd_gfx_print("\n"); 
    printnews(latest_news);
}

static void splashscreen(void)
{

    lcd_fillScreen(lcd_gfx_getBackgroundColor());
    lcd_gfx_setTextColor(lcd_gfx_getForegroundColor(), lcd_gfx_getBackgroundColor());
    lcd_gfx_drawBitmap16(16,  15,  computer_bitmap, computer_palette, 64, 64, 2) ;
    lcd_gfx_setCursor(3,5);
    lcd_gfx_setTextSize(2);
    lcd_gfx_printCenter(strings[SPLASH_TITLE]);
}

static void leaderboard(uint8_t index)
{
    lcd_gfx_setTextSize(2);
    lcd_gfx_setTextColor(lcd_gfx_getForegroundColor(), lcd_gfx_getBackgroundColor());
    lcd_fillScreen(lcd_gfx_getBackgroundColor());
    lcd_gfx_setCursor(0, 5);
    lcd_gfx_printCenter(strings[LEADERBOARD_TITLE]);
    lcd_gfx_print("\n\n");
    if (score_valid)
    {
        lcd_gfx_setTextSize(3);
        lcd_gfx_setTextColor(lcd_gfx_getHighlightColor(), lcd_gfx_getBackgroundColor());
        lcd_gfx_printCenter(leadteams[ctfscore[index]]); 
    }
    else
    {
        lcd_gfx_setTextSize(2);
        lcd_gfx_printCenter(strings[LEADERBOARD_UNAVAILABLE]);
    }

    lcd_gfx_setTextColor(lcd_gfx_getForegroundColor(), lcd_gfx_getBackgroundColor());
    lcd_gfx_setTextSize(2);
}

static void scorescreen()
{

    lcd_gfx_setTextSize(2);
    lcd_gfx_setTextColor(lcd_gfx_getForegroundColor(), lcd_gfx_getBackgroundColor());
    lcd_fillScreen(lcd_gfx_getBackgroundColor());
    lcd_gfx_setCursor(0, 5);
    lcd_gfx_printCenter(strings[SCOREBOARD_TITLE]);
    lcd_gfx_setTextSize(1);
    lcd_gfx_print("\n"); 
    lcd_gfx_drawBitmap16(96,  64,  computer_bitmap, computer_palette, 64, 64, 1) ;
    printnames(0);
}


void update_string(uint16_t msg_id, char *new_message, uint8_t new_len)
{
    uint8_t length = new_len;
    uint8_t i;
    if ((msg_id >= NUM_STRINGS) || (length < 1))
    {
        return;
    }

    if (new_len > VIP_STRING_LENGTH - 1)
        length = VIP_STRING_LENGTH - 1;
    for(i=0; i<=length; i++)
    {
        strings[msg_id][i] = new_message[i];
    }
    strings[msg_id][i] = 0;
    sb_refresh = 1;
}

void update_news(int16_t news_id, char *new_news, uint8_t new_len)
{
    uint8_t length = new_len;
    int16_t i, diff;
    int16_t current_news;
 
   
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
    if (sb_screen_state == NEWS && (current_news == latest_news))
    {
        sb_refresh = 1;
    }
}


void update_score(int16_t score_id, uint8_t *new_score, uint8_t score_len)
{
    uint8_t i;
    if (score_len < (TEAM_COUNT - 1))
    {
        return;
    }

    for(i=0; i<TEAM_COUNT; i++)
    {
        ctfscore[i] = new_score[i];
    }


    score_valid = 1;
    score_timeout = seconds + 60*60*3; // 3 hour timeout 

    if (sb_screen_state == SCORE)
    {
        sb_refresh = 1;
    }
}

void scoreboard(uint8_t incoming_refresh){
      
    if (incoming_refresh == 1)
    {
        sb_index = 0;
        sb_screen_state = SPLASH;
        lcd_gfx_setTextColor(lcd_gfx_getForegroundColor(), lcd_gfx_getBackgroundColor());
        sb_timeout = seconds + SCOREBOARD_TIMEOUT; 
        sb_refresh = 1;
    }

    if (score_timeout == seconds)
    {
        score_valid = 0;
    }
        
    if (button_back == 1){
        button_back = 0;
        if (sb_screen_state == 0)
            sb_screen_state = NUM_SB_SCREENS - 1;
        else
            sb_screen_state--;
        sb_refresh = 1;
        sb_timeout = seconds + SCOREBOARD_MANUAL_TIMEOUT;
    }
        
    if (button_next == 1){
        button_next = 0;
        sb_screen_state = (sb_screen_state + 1) % NUM_SB_SCREENS;
        sb_refresh = 1;
        sb_timeout = seconds + SCOREBOARD_MANUAL_TIMEOUT;
    }
       
    if (sb_timeout == seconds)
    {
        sb_screen_state = (sb_screen_state + 1) % NUM_SB_SCREENS;
        sb_refresh = 1;
        sb_timeout = seconds + SCOREBOARD_TIMEOUT;
    }

    switch(sb_screen_state)
    {
        case SPLASH:
            if (sb_refresh)
                splashscreen();
            break;
        case SCORE:
            if (sb_refresh)
                scorescreen();
            if (button_down == 1)
            {
                sb_timeout = seconds + SCOREBOARD_MANUAL_TIMEOUT;
                button_down = 0;
                if (sb_index < 10)
                {
                    sb_index+=10;
                    printnames(sb_index);
                }
            }

            if (button_up == 1)
            {
                sb_timeout = seconds + SCOREBOARD_MANUAL_TIMEOUT;
                button_up = 0;
                if (sb_index > 0)
                {
                    sb_index-=10;
                    printnames(sb_index);
                }
            }
            break;
        case LEADER:    
            if (sb_refresh)
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
            if (sb_refresh)
            {
                newsscreen();
                news_index = latest_news;
            }
            if ((button_down == 1) || (BUTTON_PORT & BUTTON_DOWN_BIT))
            {

                sb_timeout = seconds + SCOREBOARD_MANUAL_TIMEOUT;
                button_down = 0;
                if (news_index != oldest_news)
                {
                    news_index-=1;
                    if (news_index<0) news_index = MAX_NEWS - 1;
                    printnews(news_index);
                }
            }

            if ((button_up == 1) || (BUTTON_PORT & BUTTON_UP_BIT))
            {
                sb_timeout = seconds + SCOREBOARD_MANUAL_TIMEOUT;
                button_up = 0;
                if (news_index != latest_news)
                {
                    news_index+=1;
                    if (news_index == MAX_NEWS) news_index = 0;
                    printnews(news_index);
                }
            }
            
            break;
        case NAMETAG:
            rain(sb_refresh);
            break;
        default:
            break;
    }
    sb_refresh = 0;
}
