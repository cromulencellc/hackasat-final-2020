#ifndef __XTEA_ENCRYPT_H__
#define __XTEA_ENCRYPT_H__

#include <stdio.h>

void xtea_encrypt(volatile uint32_t v[2], const uint32_t k[4]);
void xtea_decrypt(volatile uint32_t v[2], const uint32_t k[4]);

#if 0
void xtea_encrypt(volatile const uint32_t v[2], volatile uint32_t w[2], const uint32_t k[4]);
void xtea_decrypt(volatile const uint32_t v[2], volatile uint32_t w[2], const uint32_t k[4]);
#endif

#endif // __XTEA_ENCRYPT_H__
