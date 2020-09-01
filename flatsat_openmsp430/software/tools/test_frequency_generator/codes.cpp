#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "util.h"
#include "RandomNumberGen.h"
#include "MersenneRNG.h"

/*
bool generate_codes( uint8_t code_team_list[], uint8_t code_difference_per_team[][20] )
{
	uint16_t i, j;	
	MersenneRNG oRNG;

	oRNG.Seed( ReadDevURandomSeed() );

	for ( i = 0; i < 22*20; i++ )
	{
		code_team_list[i] = 255;
	}

	for ( i = 0; i < 22; i++ )
	{
		for ( j = 0; j < 20; j++ )
		{
			code_difference_per_team[i][j] = 255;	
		}
	}

	// Calculate orthogonal spreading codes
	for ( i = 0; i < 22; i++ )
	{
		uint16_t position = 0;
		uint16_t check_position = 0;

		for ( j = 0; j < 20; j++ )
		{
			uint32_t loop_count;
			printf( "i: %d, j: %d\n", i, j );

			for ( loop_count = 0; loop_count < 100000; loop_count++ )	
			{
				uint8_t value;
				oRNG.GetU8( value );

				check_position = (position+value);
				check_position = check_position % (22*20);

				if ( code_team_list[check_position] != 255 )
					continue;

				position = check_position;

				// Make team position occupied
				code_team_list[position] = i;

				// Now record the code difference
				code_difference_per_team[i][j] = value;

				// Break loop
				break;
			}

			if ( loop_count == 100000 )
				return false;
		}
	}

	return true;

}

int main( void )
{
	uint16_t i, j, code_count;	
	
	uint8_t code_team_list[22*20];
	uint8_t code_difference_per_team[22][20];

	for ( i = 0; i < 22; i++ )
	{
		FILE *pFile;
		char szFileName[256];

		sprintf( szFileName, "team_%d_codes.c", i );

		pFile = fopen( szFileName, "w" );

		if ( !pFile )
		{
			printf( "Couldn't write out team code file.\n" );
			exit(0);
		} 

		fprintf( pFile, "#include <stdint.h>\n" );
		fprintf( pFile, "\nuint8_t time_codes[32][20] = {\n" );

		fclose( pFile );
	}
	
	for ( code_count = 0; code_count < 32; code_count++ )
	{

	while ( !generate_codes( code_team_list, code_difference_per_team ) )
		;

	// Print out the codes
	for ( i = 0; i < 22; i++ )
	{
		printf( "Team %d list: ", i );
		for ( j = 0; j < 20; j++ )
		{
			printf( "%d ", code_difference_per_team[i][j] );	
		}
		printf( "\n\n" );
	}

	// Calculate positions and print those out...
	for ( i = 0; i < 22; i++ )
	{	
		uint16_t position = 0;
		
		printf( "Team %2d positions: ", i );
		for ( j = 0; j < 20; j++ )
		{
			position = (position + code_difference_per_team[i][j]);
			position = position % (22*20);

			printf( "%3d ", position );	
		}
		printf( "\n" );
	}

	// Write team code files
	for ( i = 0; i < 22; i++ )
	{
		FILE *pFile;
		char szFileName[256];

		sprintf( szFileName, "team_%d_codes.c", i );

		pFile = fopen( szFileName, "a" );

		if ( !pFile )
		{
			printf( "Couldn't write out team code file.\n" );
			exit(0);
		} 

		fprintf( pFile, "{ " );
		for ( j = 0; j < 20; j++ )
		{
			if ( j < 19 )
				fprintf( pFile, "%d, ", code_difference_per_team[i][j] );	
			else
			{
				if ( code_count < 31 )
					fprintf( pFile, "%d },\n", code_difference_per_team[i][j] );
				else
					fprintf( pFile, "%d }\n };\n", code_difference_per_team[i][j] );
			}
		}

		fclose( pFile );
		
	}
	
	}
	
	return 0;
}
*/

uint16_t lastrand;
uint16_t myrand( void )
{
	uint16_t r;
	
	r = (((((((((((lastrand << 3) - lastrand) << 3)
        + lastrand) << 1) + lastrand) << 4)
	//- lastrand) << 1) + 0xe60));
        - lastrand) << 1) - lastrand) + 0xe60);
        //& 0x7fff;

	lastrand = r - 1; 

	return (r);
}

void generate_team_codes( uint16_t seed, uint32_t *code_count )
{
	uint8_t team_num[21];
	uint8_t i;
	uint32_t iter_count = 0;

	for ( i = 0; i < 21; i++ )
		team_num[i] = 0xFF;

	// Set seed
	lastrand = seed;

	for ( i = 0; i < 21; i++ )
	{
		uint8_t found = 0;
		do
		{
			uint8_t pos = (uint8_t)(myrand() % 21);

			if ( team_num[pos] == 0xFF )
			{
				found = 1;
				team_num[pos] = i;
			}
			
			iter_count++;
		} while ( found == 0 );
	}

	for ( i = 0; i < 21; i++ )
	{
		printf( "%d,", team_num[i] );
	}

	printf( "iter_count=%d\n", iter_count );

	code_count[seed] = iter_count;
}

int main( void )
{
	uint16_t list_pos[9] = { 10, 57, 3, 21, 89, 101, 16, 1, 93 };
	uint16_t new_list[9];	
	uint8_t i, j, idx;
	uint16_t new_item;
	uint32_t seed_cur;

	uint32_t high_val = 0;
	uint32_t code_count[65536];

	for ( seed_cur = 0; seed_cur < 65536; seed_cur++ )
		generate_team_codes( seed_cur, code_count );

	for ( seed_cur = 0; seed_cur < 65536; seed_cur++ )
	{
		if ( code_count[seed_cur] > high_val )
		{
			high_val = code_count[seed_cur];
		}
	}

	printf( "High value: %d\n", high_val );
	

	return 0;
	for ( i = 0; i < 9; i++ )
	{
		new_item = list_pos[i];

		// INSERT
		for ( j = i; j > 0; j-- )
		{
			idx = j-1;
			if ( new_item < new_list[idx] )
				new_list[j] = new_list[idx];
			else
			{
				new_list[j] = new_item;
				break;
			}
		}

		if ( j == 0 )
			new_list[0] = new_item;
	}

	printf( "Sorted list = [" );
	for ( i = 0; i < 9; i++ )
	{
		printf( "%d ", new_list[i] );
	}	
	printf( "]\n" );

	return 0;
}

/*
int main( void )
{
	uint16_t start;
	uint32_t i = 0;
	uint16_t num = 0;
	uint32_t count_bucket[65536];

	for ( i = 0; i < 65536; i++ )
		count_bucket[i] = 0;

	for ( num = 0; num < 2000; num++ )
	{
		printf( "Seed: %d [", num );
		lastrand = num;
		// PSEUDO sequence
		for ( i = 0; i < 21; i++ )
		{
			uint16_t randnum= myrand();
			count_bucket[randnum]++;	

			printf( "%d ", randnum%102 );
			
		}
		printf( "]\n" );
	}

	
}
*/
