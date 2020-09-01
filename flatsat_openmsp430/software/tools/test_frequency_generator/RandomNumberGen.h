#ifndef __RANDOM_NUMBER_GEN_H__
#define __RANDOM_NUMBER_GEN_H__

#include <stdint.h>

class CRNGCommon
{
public:

	typedef enum
	{
		RNG_OUT_OF_MEMORY = 0,
		RNG_NO_DATA = 1,
		RNG_NEED_SEED = 2,
		RNG_ERROR = 3,
		RNG_DATA_INVALID = 4,
		RNG_OK = 5
	} RNG_STATUS;

private:
};

class RandomNumberGen
{
public:
	RandomNumberGen() {};
	~RandomNumberGen() {};

	virtual CRNGCommon::RNG_STATUS GetState( void* pData, uint32_t dataLen ) = 0;
	virtual CRNGCommon::RNG_STATUS SaveState( void** pData, uint32_t &dataLen ) = 0;

	virtual CRNGCommon::RNG_STATUS Seed( uint32_t seed ) = 0;

	virtual CRNGCommon::RNG_STATUS GetU32( uint32_t &value ) = 0;
	virtual CRNGCommon::RNG_STATUS GetRange( uint32_t start, uint32_t end, uint32_t &value ) = 0;
	virtual CRNGCommon::RNG_STATUS GetU8( uint8_t &value ) = 0;
	virtual CRNGCommon::RNG_STATUS GetU16( uint16_t &value ) = 0;
	virtual CRNGCommon::RNG_STATUS GetReal( double &value ) = 0;

private:
};

#endif // __RANDOM_NUMBER_GEN_H__