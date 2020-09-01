#include "xtea_encrypt.h"

void xtea_encrypt(const uint32_t v[2], uint32_t w[2], const uint32_t k[4])
{
        volatile uint32_t y=v[0],z=v[1];
	uint32_t sum=0,delta=0x9E3779B9;
        uint16_t n=16;

        while(n-->0)
        {
                y += (z << 4 ^ z >> 5) + z ^ sum + k[sum&3];
                sum += delta;
                z += (y << 4 ^ y >> 5) + y ^ sum + k[sum>>11 & 3];
        }

        w[0]=y; w[1]=z;
}

void xtea_decrypt(const uint32_t v[2], uint32_t w[2], const uint32_t k[4])
{
        volatile uint32_t y=v[0],z=v[1];
	uint32_t sum=0xE3779B90, delta=0x9E3779B9;         
        uint16_t n=16;

        /* sum = delta<<5, in general sum = delta * n */
        while(n-->0)
        {
                z -= (y << 4 ^ y >> 5) + y ^ sum + k[sum>>11 & 3];
                sum -= delta;
                y -= (z << 4 ^ z >> 5) + z ^ sum + k[sum&3];
        }

        w[0]=y; w[1]=z;
}
