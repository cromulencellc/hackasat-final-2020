#ifndef __XTEA_ENCRYPT_H__
#define __XTEA_ENCRYPT_H__

#include <stdio.h>

void xtea_encrypt(const uint32_t v[2], uint32_t w[2], const uint32_t k[4]);
void xtea_decrypt(const uint32_t v[2], uint32_t w[2], const uint32_t k[4]);

#endif // __XTEA_ENCRYPT_H__
