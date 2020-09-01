#ifndef __MERSENNE_RNG_H__
#define __MERSENNE_RNG_H__

#include "RandomNumberGen.h"

class MersenneRNG : public RandomNumberGen
{
public:
	MersenneRNG();
	~MersenneRNG();

	CRNGCommon::RNG_STATUS GetState( void* pData, uint32_t dataLen );
	CRNGCommon::RNG_STATUS SaveState( void** pData, uint32_t &dataLen );

	CRNGCommon::RNG_STATUS Seed( uint32_t seed );
	CRNGCommon::RNG_STATUS InitByArray( uint32_t init_key[], uint32_t key_length );

	CRNGCommon::RNG_STATUS GetU32( uint32_t &value );
	CRNGCommon::RNG_STATUS GetRange( uint32_t start, uint32_t end, uint32_t &value );
	CRNGCommon::RNG_STATUS GetU8( uint8_t &value );
	CRNGCommon::RNG_STATUS GetU16( uint16_t &value );
	CRNGCommon::RNG_STATUS GetReal( double &value );

	int32_t GenRandInt31(void);
	double GenRandReal1(void);
	double GenRandReal2(void);
	double GenRandReal3(void);
	double GenRandRes53(void);

protected:
	uint32_t GenRandInt32(void);

private:
	uint32_t seed;
	
	static const uint32_t N = 624;
	static const uint32_t M = 397;
	static const uint32_t MATRIX_A = 0x9908b0dfUL;   /* constant vector a */
	static const uint32_t UPPER_MASK = 0x80000000UL; /* most significant w-r bits */
	static const uint32_t LOWER_MASK = 0x7fffffffUL; /* least significant r bits */

	uint32_t mt[N]; /* the array for the state vector  */
	uint32_t mti;	/* mti==N+1 means mt[N] is not initialized */
};

#endif // __MERSENNE_RNG_H__