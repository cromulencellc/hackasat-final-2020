/* 
   A C-program for MT19937, with initialization improved 2002/1/26.
   Coded by Takuji Nishimura and Makoto Matsumoto.

   Before using, initialize the state by using init_genrand(seed)  
   or init_by_array(init_key, key_length).

   Copyright (C) 1997 - 2002, Makoto Matsumoto and Takuji Nishimura,
   All rights reserved.                          

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

     1. Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.

     2. Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.

     3. The names of its contributors may not be used to endorse or promote 
        products derived from this software without specific prior written 
        permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


   Any feedback is very welcome.
   http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/emt.html
   email: m-mat @ math.sci.hiroshima-u.ac.jp (remove space)
*/

#include "MersenneRNG.h"

#include <stdio.h>

MersenneRNG::MersenneRNG() : RandomNumberGen()
{
	mti=N+1;
}

MersenneRNG::~MersenneRNG()
{

}

CRNGCommon::RNG_STATUS MersenneRNG::GetState( void* pData, uint32_t dataLen )
{
	return (CRNGCommon::RNG_OK);
}

CRNGCommon::RNG_STATUS MersenneRNG::SaveState( void** pData, uint32_t &dataLen )
{
	return (CRNGCommon::RNG_OK);
}

/* initializes mt[N] with a seed */
CRNGCommon::RNG_STATUS MersenneRNG::Seed( uint32_t s )
{
	// Store seed
	seed = s;

    mt[0]= s & 0xffffffffUL;
    for (mti=1; mti<N; mti++) 
	{
        mt[mti] = 
	    (1812433253UL * (mt[mti-1] ^ (mt[mti-1] >> 30)) + mti); 
        /* See Knuth TAOCP Vol2. 3rd Ed. P.106 for multiplier. */
        /* In the previous versions, MSBs of the seed affect   */
        /* only MSBs of the array mt[].                        */
        /* 2002/01/09 modified by Makoto Matsumoto             */
        mt[mti] &= 0xffffffffUL;
        /* for >32 bit machines */
    }

	return (CRNGCommon::RNG_OK);
}

/* initialize by an array with array-length */
/* init_key is the array for initializing keys */
/* key_length is its length */
/* slight change for C++, 2004/2/26 */
CRNGCommon::RNG_STATUS MersenneRNG::InitByArray( uint32_t init_key[], uint32_t key_length )
{
    uint32_t i, j, k;
    Seed(19650218UL);
    i=1; j=0;
    k = (N>key_length ? N : key_length);
    for (; k; k--) 
	{
        mt[i] = (mt[i] ^ ((mt[i-1] ^ (mt[i-1] >> 30)) * 1664525UL))
          + init_key[j] + j; /* non linear */
        mt[i] &= 0xffffffffUL; /* for WORDSIZE > 32 machines */
        i++; j++;
        if (i>=N) { mt[0] = mt[N-1]; i=1; }
        if (j>=key_length) j=0;
    }
    for (k=N-1; k; k--) 
	{
        mt[i] = (mt[i] ^ ((mt[i-1] ^ (mt[i-1] >> 30)) * 1566083941UL))
          - i; /* non linear */
        mt[i] &= 0xffffffffUL; /* for WORDSIZE > 32 machines */
        i++;
        if (i>=N) { mt[0] = mt[N-1]; i=1; }
    }

    mt[0] = 0x80000000UL; /* MSB is 1; assuring non-zero initial array */ 

	return (CRNGCommon::RNG_OK);
}

CRNGCommon::RNG_STATUS MersenneRNG::GetU32( uint32_t &value )
{
	if ( mti == N+1 )
		return (CRNGCommon::RNG_NEED_SEED);

	value = GenRandInt32();

	return (CRNGCommon::RNG_OK);
}

CRNGCommon::RNG_STATUS MersenneRNG::GetRange( uint32_t start, uint32_t end, uint32_t &value )
{
	if ( mti == N+1 )
		return (CRNGCommon::RNG_NEED_SEED);

	if ( start > end )
	{
		// Swap em!
		uint32_t temp = end;
		end = start;
		start = temp;
	}
	else if ( start == end )
	{
		// start is equal to end... always return the value.
		value = start;

		return (CRNGCommon::RNG_OK);
	}

	// Calculate the difference... this is the actual range we will calculate over. (note we are inclusive!)
	uint32_t diff = (end - start) + 1;

	// Quick calculation... check for a full range
	if ( diff == 0 )
	{
		value = GenRandInt32();
		return (CRNGCommon::RNG_OK);
	}

	// Find the ceiling of (n * diff) that is less than 2^32... this will be the pick range
	uint64_t top_range = ((uint64_t)1<<32);
	uint64_t subtract_amt = (top_range % (uint64_t)diff);

	top_range = (top_range - subtract_amt);

	if ( top_range >= ((uint64_t)1<<32) )
	{
		value = start + (GenRandInt32() % diff);
		return (CRNGCommon::RNG_OK);
	}

	uint32_t choice_range = (uint32_t)top_range;
	uint32_t choice;

	// Constrain the choice to the proper range... this guarantees better distribution
	do
	{
		choice = GenRandInt32();
	} while ( choice >= choice_range );

	// We have a valid choice, return.
	value = start + (choice % diff);

	return (CRNGCommon::RNG_OK);
}

CRNGCommon::RNG_STATUS MersenneRNG::GetU8( uint8_t &value )
{
	if ( mti == N+1 )
		return (CRNGCommon::RNG_NEED_SEED);

	value = (uint8_t)(GenRandInt32() >> 24);

	return (CRNGCommon::RNG_OK);
}

CRNGCommon::RNG_STATUS MersenneRNG::GetU16( uint16_t &value )
{
	if ( mti == N+1 )
		return (CRNGCommon::RNG_NEED_SEED);

	value = (uint16_t)(GenRandInt32() >> 16);

	return (CRNGCommon::RNG_OK);
}

CRNGCommon::RNG_STATUS MersenneRNG::GetReal( double &value )
{
	if ( mti == N+1 )
		return (CRNGCommon::RNG_NEED_SEED);

	value = GenRandReal2();

	return (CRNGCommon::RNG_OK);
}

/* generates a random number on [0,0xffffffff]-interval */
uint32_t MersenneRNG::GenRandInt32(void)
{
    uint32_t y;
    static uint32_t mag01[2]={0x0UL, MATRIX_A};
    /* mag01[x] = x * MATRIX_A  for x=0,1 */

    if (mti >= N) 
	{ /* generate N words at one time */
        uint32_t kk;

        if (mti == N+1)   /* if init_genrand() has not been called, */
            Seed(5489UL); /* a default initial seed is used */

        for (kk=0;kk<N-M;kk++) 
		{
            y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
            mt[kk] = mt[kk+M] ^ (y >> 1) ^ mag01[y & 0x1UL];
        }

        for (;kk<N-1;kk++) 
		{
            y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
            mt[kk] = mt[kk+(M-N)] ^ (y >> 1) ^ mag01[y & 0x1UL];
        }

        y = (mt[N-1]&UPPER_MASK)|(mt[0]&LOWER_MASK);
        mt[N-1] = mt[M-1] ^ (y >> 1) ^ mag01[y & 0x1UL];

        mti = 0;
    }
  
    y = mt[mti++];

    /* Tempering */
    y ^= (y >> 11);
    y ^= (y << 7) & 0x9d2c5680UL;
    y ^= (y << 15) & 0xefc60000UL;
    y ^= (y >> 18);

    return y;
}


/* generates a random number on [0,0x7fffffff]-interval */
int32_t MersenneRNG::GenRandInt31(void)
{
    return (int32_t)(GenRandInt32()>>1);
}

/* generates a random number on [0,1]-real-interval */
double MersenneRNG::GenRandReal1(void)
{
    return GenRandInt32()*(1.0/4294967295.0); 
    /* divided by 2^32-1 */ 
}

/* generates a random number on [0,1)-real-interval */
double MersenneRNG::GenRandReal2(void)
{
    return GenRandInt32()*(1.0/4294967296.0); 
    /* divided by 2^32 */
}

/* generates a random number on (0,1)-real-interval */
double MersenneRNG::GenRandReal3(void)
{
    return (((double)GenRandInt32()) + 0.5)*(1.0/4294967296.0); 
    /* divided by 2^32 */
}

/* generates a random number on [0,1) with 53-bit resolution*/
double MersenneRNG::GenRandRes53(void) 
{ 
    uint32_t a=GenRandInt32()>>5, b=GenRandInt32()>>6; 
    return(a*67108864.0+b)*(1.0/9007199254740992.0); 
} 
