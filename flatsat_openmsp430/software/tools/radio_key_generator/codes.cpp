#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "util.h"

#define TEAM_COUNT	22

void set_random_xtea_key( uint32_t key[4] )
{
	// Use /dev/random to get a random key
	
	FILE *pFile;

	pFile = fopen( "/dev/random", "rb" );

	if ( !pFile )
	{
		printf( "Failed to open /dev/random.\n" );
		exit(0);
	}

	printf( "Getting KEY DATA\n" );
	fread( key, 16, 1, pFile );
	printf( "GOT KEY DATA: %08X %08X %08X %08X\n", key[0], key[1], key[2], key[3] );

	fclose( pFile );

	return;
}

uint32_t get_random_u32( void )
{
	uint32_t temp;
	// Use /dev/random to get a random key
	
	FILE *pFile;

	pFile = fopen( "/dev/random", "rb" );

	if ( !pFile )
	{
		printf( "Failed to open /dev/random.\n" );
		exit(0);
	}

	fread( &temp, 4, 1, pFile );

	fclose( pFile );

	return temp;
}


uint16_t get_random_u16( void )
{
	uint16_t temp;
	// Use /dev/random to get a random key
	
	FILE *pFile;

	pFile = fopen( "/dev/random", "rb" );

	if ( !pFile )
	{
		printf( "Failed to open /dev/random.\n" );
		exit(0);
	}

	fread( &temp, 2, 1, pFile );

	fclose( pFile );

	return temp;
}

int main( void )
{
	uint32_t i;

	// Write team codes
	uint32_t sync_key[4];
	uint16_t sync_spread_key;

	uint32_t team_data_key[TEAM_COUNT][4];
	uint32_t team_token_key[TEAM_COUNT][4];
	uint32_t team_random_key[TEAM_COUNT][4];
	uint32_t team_code_id[TEAM_COUNT];
	uint16_t team_spread_key[TEAM_COUNT];

	printf( "Generating random encryption keys... Give me ENTROPY!!!!\r\n" );	

	{
		FILE *pFile;
		char szFileName[256];

		sprintf( szFileName, "team_keys.c" );

		pFile = fopen( szFileName, "w" );

		if ( !pFile )
		{
			printf( "Couldn't write out team code file.\n" );
			exit(0);
		} 

		fprintf( pFile, "#include <stdint.h>\n" );

		// Create sync keys
		set_random_xtea_key( sync_key );
		
		sync_spread_key = get_random_u16();

		fprintf( pFile, "const uint32_t radio_sync_key[4] = { 0x%08X, 0x%08X, 0x%08X, 0x%08X };\n", sync_key[0], sync_key[1], sync_key[2], sync_key[3] );

		fprintf( pFile, "const uint16_t radio_sync_spread_key = 0x%04X;\n", sync_spread_key ); 

		for ( i = 0; i < TEAM_COUNT; i++ )
		{
			fprintf( pFile, "#ifdef TEAM_%d_CODES\n", i );

			// Generate team keys
			set_random_xtea_key( team_data_key[i] );
			set_random_xtea_key( team_token_key[i] );
			set_random_xtea_key( team_random_key[i] );

			team_code_id[i] = get_random_u32();
			team_spread_key[i] = get_random_u16();
		
			// Write out TEAM ID
			fprintf( pFile, "#define MY_TEAM_ID %d\n", i );	

			fprintf( pFile, "const uint32_t radio_data_key[4] = { 0x%08X, 0x%08X, 0x%08X, 0x%08X };\n", team_data_key[i][0], team_data_key[i][1], team_data_key[i][2], team_data_key[i][3] );
			fprintf( pFile, "const uint32_t radio_random_key[4] = { 0x%08X, 0x%08X, 0x%08X, 0x%08X };\n", team_random_key[i][0], team_random_key[i][1], team_random_key[i][2], team_random_key[i][3] );
			fprintf( pFile, "const uint32_t radio_token_key[4] = { 0x%08X, 0x%08X, 0x%08X, 0x%08X };\n", team_token_key[i][0], team_token_key[i][1], team_token_key[i][2], team_token_key[i][3] );

			fprintf( pFile, "const uint32_t radio_team_code = 0x%08X;\n", team_code_id[i] ); 
			fprintf( pFile, "const uint16_t radio_data_spread_key = 0x%04X;\n", team_spread_key[i] ); 
			
			fprintf( pFile, "#endif\n\n" );
		}
	
		fclose( pFile );
	}

	// Write server codes
	{
		FILE *pFile;
		char szFileName[256];

		sprintf( szFileName, "server_keys.c" );

		pFile = fopen( szFileName, "w" );
		
		if ( !pFile )
		{
			printf( "Couldn't write out server code file.\n" );
			exit(0);
		}

		fprintf( pFile, "#include <stdint.h>\n\n" );
		fprintf( pFile, "const uint32_t radio_sync_key[4] = { 0x%08X, 0x%08X, 0x%08X, 0x%08X };\n", sync_key[0], sync_key[1], sync_key[2], sync_key[3] );

		fprintf( pFile, "const uint16_t radio_sync_spread_key = 0x%04X;\n", sync_spread_key ); 

		fprintf( pFile, "\n// Team specific keys (in an array of course)\n\n" );	

		fprintf( pFile, "const uint32_t radio_data_key[%d][4] = {\n", TEAM_COUNT );
		for ( i = 0; i < TEAM_COUNT; i++ )
		{
			if ( i < (TEAM_COUNT-1) )
				fprintf( pFile, "{ 0x%08X, 0x%08X, 0x%08X, 0x%08X },\n", team_data_key[i][0], team_data_key[i][1], team_data_key[i][2], team_data_key[i][3] );
			else
				fprintf( pFile, "{ 0x%08X, 0x%08X, 0x%08X, 0x%08X }\n};\n\n", team_data_key[i][0], team_data_key[i][1], team_data_key[i][2], team_data_key[i][3] );
				
		}


		fprintf( pFile, "const uint32_t radio_token_key[%d][4] = {\n", TEAM_COUNT );
		for ( i = 0; i < TEAM_COUNT; i++ )
		{
			if ( i < (TEAM_COUNT-1) )
				fprintf( pFile, "{ 0x%08X, 0x%08X, 0x%08X, 0x%08X },\n", team_token_key[i][0], team_token_key[i][1], team_token_key[i][2], team_token_key[i][3] );
			else
				fprintf( pFile, "{ 0x%08X, 0x%08X, 0x%08X, 0x%08X }\n};\n\n", team_token_key[i][0], team_token_key[i][1], team_token_key[i][2], team_token_key[i][3] );
				
		}
	
		fprintf( pFile, "const uint32_t radio_team_code[%d] = { ", TEAM_COUNT );
		for ( i = 0; i < TEAM_COUNT; i++ )
		{
			if ( i < (TEAM_COUNT-1) )
				fprintf( pFile, "0x%08X, ", team_code_id[i] );
			else
				fprintf( pFile, "0x%08X };\n\n", team_code_id[i] );
				
		}
				
		fprintf( pFile, "const uint16_t radio_data_spread_key[%d] = { ", TEAM_COUNT );
		for ( i = 0; i < TEAM_COUNT; i++ )
		{
			if ( i < (TEAM_COUNT-1) )
				fprintf( pFile, "0x%04X, ", team_spread_key[i] );
			else
				fprintf( pFile, "0x%04X };\n\n", team_spread_key[i] );
				
		}

		fclose( pFile );
	}
	
	return 0;
}
