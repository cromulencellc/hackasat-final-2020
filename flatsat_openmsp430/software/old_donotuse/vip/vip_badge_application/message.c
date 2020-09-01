#include <stdio.h>
#include "vip.h"
#include "hardware.h"


static uint8_t htoi(char c)
{
	if (c >= '0' && c <= '9')
    	return (c - '0');
    if (c >= 'A' && c <= 'F')
        return (c - 'A' + 10);
    if (c >= 'a' && c <= 'f')
        return (c - 'a' + 10);
    return 0;
}

int8_t process_incoming_message(char *msg, uint16_t length)
{
	uint8_t msg_pos = 3;
	uint8_t data_pos = 0;
	uint8_t checksum = 0;
	char data[86];
	uint8_t byte;

    // Verify message
	if ((length < 6) || (length > RADIO_MSG_LEN))
	{
		return -1;
	}

	if ((msg[0]!='V') || (msg[1]!='P') || (msg[2]!=':') || (msg[length-3] != ','))
	{
		return -1;
	}

	while (msg_pos < (length - 4))
	{
		byte = htoi(msg[msg_pos]) << 4 | htoi(msg[msg_pos+1]);
		checksum += byte;
		data[data_pos++] = byte; 
		msg_pos += 2;
	}

	if (checksum != ((htoi(msg[RADIO_MSG_LEN - 2]) << 4) | htoi(msg[RADIO_MSG_LEN - 1])))
	{
		return -1;
	}

	if (data_pos < 6)
	{
		return -1;
	}

    // Interpret message
	if ( data[0]=='N' && data[1] == 'M')
	{
		int16_t news_id = htoi(data[2])<<12|htoi(data[3])<<8|htoi(data[4])<<4|htoi(data[5]);
		update_news(news_id, &data[6], data_pos - 6);
		return VIP_NEWS_MSG;
	}    
	else if (data[0]=='S' && data[1] == 'U')
	{
		uint16_t score_id = htoi(data[2])<<12|htoi(data[3])<<8|htoi(data[4])<<4|htoi(data[5]);
		update_score(score_id, (uint8_t *)&data[6], data_pos - 6);
		return VIP_SCORE_MSG;
	}
	else if (data[0]=='C' && data[1] == 'T')
	{
		uint16_t string_id = htoi(data[2])<<12|htoi(data[3])<<8|htoi(data[4])<<4|htoi(data[5]);
		update_string(string_id, &data[6], data_pos - 6);
		return VIP_STRING_MSG;
	}
	else if (data[0] == 'A' && data[1] == 'N')
	{
		uint16_t msg_id = htoi(data[2])<<12|htoi(data[3])<<8|htoi(data[4])<<4|htoi(data[5]);
		return new_announcement(msg_id, &data[6], data_pos - 6);
	
	}
	return -1;
}
