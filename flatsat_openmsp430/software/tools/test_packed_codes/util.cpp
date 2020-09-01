#include "util.h"

#include <string>
#include <stdio.h>
#include <stdlib.h>

#define KEYFILE_NAME "key"
#define DEFAULT_KEY "The key is: magically delicious"

uint32_t ReadDevURandomSeed( void )
{
        FILE *pFile;
        uint32_t value;

        pFile = fopen( "/dev/urandom", "rb" );

        if ( !pFile )
        {
                printf( "Couldn't open /dev/urandom reverting to time(NULL) seed.\n" );
                return (time(NULL));
        }

        if ( fread( &value, sizeof(value), 1, pFile ) != 1 )
        {
                printf( "Couldn't get enough entropy.  Reverting to time(NULL) seed.\n" );
                return (time(NULL));
        }

        fclose( pFile );

        return (value);
}

std::string ReadKeyFile( void )
{
	char data[255];
	FILE *pFile;

	pFile = fopen( KEYFILE_NAME, "r" );
	
	if ( !pFile )
	{
		printf( "Error: No key file: %s.  Using default key.\n", KEYFILE_NAME );
		return std::string(DEFAULT_KEY);
	}

	size_t startPos = ftell( pFile );

	fseek( pFile, 0, SEEK_END );

	size_t endPos = ftell( pFile );

	fseek( pFile, 0, SEEK_SET );

	size_t fileSize = (endPos-startPos);

	if ( fileSize > 100 )
	{
		printf( "Error key file too large.  Using default key.\n" );
		return std::string(DEFAULT_KEY);
	}	

	if ( fread( data, 1, fileSize, pFile ) != fileSize )
	{
		printf( "Error reading key file.  Using default key.\n" );
		return std::string(DEFAULT_KEY);
	}

	return std::string(data);
}

unsigned int random_in_range (unsigned int min, unsigned int max)
{
  unsigned int base_random = rand(); /* in [0, RAND_MAX] */
  if (RAND_MAX == base_random) return random_in_range(min, max);
  /* now guaranteed to be in [0, RAND_MAX) */
  unsigned int range       = max - min,
      remainder   = RAND_MAX % range,
      bucket      = RAND_MAX / range;
  /* There are range buckets, plus one smaller interval
     within remainder of RAND_MAX */
  if (base_random < RAND_MAX - remainder) {
    return min + base_random/bucket;
  } else {
    return random_in_range (min, max);
  }
}

