#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "util.h"
#include "RandomNumberGen.h"
#include "MersenneRNG.h"

#define TEAM_COUNT		21
#define CODE_COUNT		20
#define NUMBER_OF_CODES		16

bool generate_codes( uint8_t code_team_list[], uint8_t code_difference_per_team[][CODE_COUNT] )
{
	uint16_t i, j;	
	MersenneRNG oRNG;

	oRNG.Seed( ReadDevURandomSeed() );

	for ( i = 0; i < TEAM_COUNT*CODE_COUNT; i++ )
	{
		code_team_list[i] = 255;
	}

	for ( i = 0; i < TEAM_COUNT; i++ )
	{
		for ( j = 0; j < CODE_COUNT; j++ )
		{
			code_difference_per_team[i][j] = 255;	
		}
	}

	// Calculate orthogonal spreading codes
	for ( i = 0; i < TEAM_COUNT; i++ )
	{
		uint16_t position = 0;
		uint16_t check_position = 0;

		for ( j = 0; j < CODE_COUNT; j++ )
		{
			uint32_t loop_count;

			for ( loop_count = 0; loop_count < 100000; loop_count++ )	
			{
				uint8_t value;
				oRNG.GetU8( value );

				check_position = (position+value);
				check_position = check_position % (TEAM_COUNT*CODE_COUNT);

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
	
	uint8_t code_team_list[NUMBER_OF_CODES][TEAM_COUNT*CODE_COUNT];
	uint8_t code_difference_per_team[NUMBER_OF_CODES][TEAM_COUNT][CODE_COUNT];

	// Write team codes
	{
		FILE *pFile;
		char szFileName[256];

		sprintf( szFileName, "team_codes.c", i );

		pFile = fopen( szFileName, "w" );

		if ( !pFile )
		{
			printf( "Couldn't write out team code file.\n" );
			exit(0);
		} 

		fprintf( pFile, "#include <stdint.h>\n" );

		fclose( pFile );
	}

	// Write server codes
	{
		FILE *pFile;
		char szFileName[256];

		sprintf( szFileName, "server_codes.c" );

		pFile = fopen( szFileName, "w" );
		
		if ( !pFile )
		{
			printf( "Couldn't write out server code file.\n" );
			exit(0);
		}

		fprintf( pFile, "#include <stdint.h>\n" );
		fprintf( pFile, "\nuint8_t time_codes[%d][%d] = {\n", NUMBER_OF_CODES, (TEAM_COUNT*CODE_COUNT) );
	
		fclose( pFile );
	}
	
	for ( code_count = 0; code_count < NUMBER_OF_CODES; code_count++ )
	{

		while ( !generate_codes( code_team_list[code_count], code_difference_per_team[code_count] ) )
			;

		// Print out the codes
		for ( i = 0; i < TEAM_COUNT; i++ )
		{
			printf( "Team %d list: ", i );
			for ( j = 0; j < CODE_COUNT; j++ )
			{
				printf( "%d ", code_difference_per_team[code_count][i][j] );	
			}
			printf( "\n\n" );
		}

		// Calculate positions and print those out...
		for ( i = 0; i < TEAM_COUNT; i++ )
		{	
			uint16_t position = 0;
		
			printf( "Team %2d positions: ", i );
			for ( j = 0; j < CODE_COUNT; j++ )
			{
				position = (position + code_difference_per_team[code_count][i][j]);
				position = position % (TEAM_COUNT*CODE_COUNT);

				printf( "%3d ", position );	
			}
			printf( "\n" );
		}
	}

	// Write team code files
	{
		FILE *pFile;
		char szFileName[256];

		sprintf( szFileName, "team_codes.c", i );

		pFile = fopen( szFileName, "a" );
		
		if ( !pFile )
		{
			printf( "Couldn't write out team code file.\n" );
			exit(0);
		} 


		for ( i = 0; i < TEAM_COUNT; i++ )
		{
			fprintf( pFile, "#ifdef TEAM_%d_CODES\n", i );
			fprintf( pFile, "\nuint8_t time_codes[%d][%d] = {\n", NUMBER_OF_CODES, CODE_COUNT );

			for ( code_count = 0; code_count < NUMBER_OF_CODES; code_count++ )
			{
				fprintf( pFile, "{ " );
			for ( j = 0; j < CODE_COUNT; j++ )
			{
				if ( j < CODE_COUNT-1 )
					fprintf( pFile, "%d, ", code_difference_per_team[code_count][i][j] );	
				else
				{
					if ( code_count < NUMBER_OF_CODES-1 )
						fprintf( pFile, "%d },\n", code_difference_per_team[code_count][i][j] );
					else
						fprintf( pFile, "%d }\n };\n", code_difference_per_team[code_count][i][j] );
				}
			}
			}
			fprintf( pFile, "#endif\n" );
		}
	

		fclose( pFile );
	}

	// Write server file
	{
		FILE *pFile;
		char szFileName[256];
		
		sprintf( szFileName, "server_codes.c" );

		pFile = fopen( szFileName, "a" );

		if ( !pFile )
		{
			printf( "Couldn't write out server code file.\n" );
			exit(0);
		}

		for ( code_count = 0; code_count < NUMBER_OF_CODES; code_count++ )
		{
			fprintf( pFile, "{ " );
		for ( j = 0; j < (TEAM_COUNT*CODE_COUNT); j++ )
		{
			if ( j < ((TEAM_COUNT*CODE_COUNT)-1) )
				fprintf( pFile, "%d, ", code_team_list[code_count][j] );
			else
			{
				if ( code_count < NUMBER_OF_CODES-1 )
					fprintf( pFile, "%d },\n", code_team_list[code_count][j] );
				else
					fprintf( pFile, "%d }\n };\n", code_team_list[code_count][j] );
			}
		}
		}

		fclose( pFile );
	}	
	
	return 0;
}
