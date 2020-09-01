#ifndef __RADIO_PACKET_H__
#define __RADIO_PACKET_H__

// Programmed preamble length
#define RADIO_PROGRAMMED_PREAMBLE_LEN	3

// Timeout of search for preamble (preamble window counter)
#define RADIO_PREAMBLE_RX_TIMEOUT	40	

// Radio channel select (high 16-bits)
#define RADIO_BASE_CHANNEL_OFFSETHI	0xE1B0	// 902.75MHz
#define RADIO_MIDDLE_CHANNEL_OFFSETHI	0xE4C0	// 915MHz
#define RADIO_CHANNEL_MAX		0xE7F0	// 927.75Mhz

#define RADIO_CHANNEL_INCREMENT	0x00C0	// 1MHz
#define RADIO_CHANNEL_COUNT	((RADIO_CHANNEL_MAX-RADIO_BASE_CHANNEL_OFFSETHI)/RADIO_CHANNEL_INCREMENT)	// (MAX-MIN)/INC = 25 channels



#endif // __RADIO_PACKET_H__
