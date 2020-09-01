#ifndef VIP_VIP_H_
#define VIP_VIP_H_


// Variables needed by other modules
extern volatile uint8_t button_next, button_up, button_down, button_back;
extern volatile uint32_t seconds;
extern volatile uint8_t sequence[8];
extern volatile uint32_t activity_timer;
extern volatile uint8_t wake_flag;


// Functions in other files, declared here for convenience
void scoreboard(uint8_t incoming_refresh);
void rain(uint8_t refresh);
uint8_t pong2(uint8_t refresh);
uint8_t pong(uint8_t refresh);
int8_t  process_incoming_message(char *msg, uint16_t length);
void update_news(int16_t news_id, char *new_news, uint8_t new_len);
void update_string(uint16_t msg_id, char *new_message, uint8_t new_len);
void update_score(int16_t score_id, uint8_t *new_score, uint8_t score_len);
void color_picker(uint8_t refresh);
uint16_t get_current_color(void);
uint16_t get_color(uint16_t x, uint16_t y);
int8_t new_announcement(uint16_t announcement_id, char *msg, uint8_t msg_len);
void pre_announcement(uint8_t refresh, uint32_t time);
void announcement(uint8_t refresh, uint32_t time);

// Reason to wake from LPM0
#define WAKE_UART   		0x01
#define WAKE_BUTTON 		0x02
#define WAKE_TIMER  		0x04
#define WAKE_BUTTON_POLL   	0x08

// VIP Control messages
#define RADIO_MSG_LEN 178
#define MAX_NEWS  64
#define NEWS_LENGTH  27
#define VIP_STRING_LENGTH  64

#define TEAM_COUNT 20

typedef struct news_s{
    uint8_t valid;
    int16_t id;
    char msg[NEWS_LENGTH];
}news_t;


// Message Types
#define VIP_ANNOUNCEMENT_MSG 0x01
#define VIP_STRING_MSG 0x02
#define VIP_NEWS_MSG 0x03
#define VIP_SCORE_MSG 0x04

#define VIP_PRE_ANNOUNCEMENT_TIME 20
#define VIP_ANNOUNCEMENT_TIME 20
#define SCOREBOARD_TIMEOUT 8
#define SCOREBOARD_MANUAL_TIMEOUT 16
#endif //VIP_VIP_H_