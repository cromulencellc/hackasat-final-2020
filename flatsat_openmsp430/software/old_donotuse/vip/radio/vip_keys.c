#ifndef __RADIO_KEYS_H__
#define __RADIO_KEYS_H__

#include <stdint.h>
const uint32_t radio_sync_key[4] = { 0xEB054CBA, 0xFBBB71D6, 0x35777523, 0x4DE1E070 };
const uint16_t radio_sync_spread_key = 0xCAE0;

const uint32_t radio_vip_key[4] = { 0x32387065, 0xFF624DF0, 0x63214B03, 0x8AB4008B };

const uint16_t radio_vip_spread_key = { 0xF53E };

#endif // __RADIO_KEYS_H__
