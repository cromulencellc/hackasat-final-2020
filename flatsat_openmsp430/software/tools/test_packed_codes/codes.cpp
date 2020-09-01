#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "util.h"

#define TEAM_COUNT		20
#define CODE_COUNT		21

#define SLOT_TOTAL_COUNT	420

#define SPREAD_OFFSET_MAX	420
#define SPREAD_CODES_MAX	16

extern uint8_t time_codes[16][21];
extern uint8_t server_time_codes[16][420];

typedef struct RADIO_TIME_SLOT
{
	uint8_t		from_tid;
	uint8_t		to_tid;
} tTimeSlot;

void test_server_code( void )
{
	uint16_t i;
        uint16_t spread_sequence;
        uint16_t spread_offset;
        uint8_t team_count[20];
	tTimeSlot timeSlots[SLOT_TOTAL_COUNT];

	// Extract information
        spread_offset = 420-399;
        spread_sequence = 10;

        spread_offset = spread_offset % SPREAD_OFFSET_MAX;
        spread_sequence = spread_sequence % SPREAD_CODES_MAX;

        // Counters for the team slots
        for ( i = 0; i < 20; i++ )
                team_count[i] = 0;

        for ( i = 0; i < SPREAD_OFFSET_MAX; i++ )
        {
                /*if ( mode == TIME_SLOT_RX )
                {
                        // RX
                        uint8_t from_tid = time_codes[spread_sequence][spread_offset];
                        g_timeSlots[i].from_tid = from_tid;

                        if ( team_count[from_tid] == from_tid )
                                team_count[from_tid]++;

                        g_timeSlots[i].to_tid = team_count[from_tid];

                        team_count[from_tid]++;
                }
                else
		*/
                {
			// TX
                        uint8_t to_tid = server_time_codes[spread_sequence][spread_offset];

                        timeSlots[i].to_tid = to_tid;

                        if ( team_count[to_tid] == to_tid )
                                team_count[to_tid]++;

                        timeSlots[i].from_tid = team_count[to_tid];

                        team_count[to_tid]++;
		}

		spread_offset++;
                if ( spread_offset >= SPREAD_OFFSET_MAX )
                        spread_offset = 0;
	}

	for ( i = 0; i < 420; i++ )
	{
		if ( timeSlots[i].to_tid == 0 )
		{
			printf( "slot [%d] tid[%d] fid[%d]\n", i, timeSlots[i].to_tid, timeSlots[i].from_tid );
		}
	}
	
}

void test_client_code( void )
{
	uint8_t i, j;
	uint16_t spread_offset;
	uint8_t spread_sequence;
	uint8_t team_slots[21];
	uint16_t time_slots[21];

	spread_offset = 399;
	spread_sequence = 10;

	for ( i = 0; i < 21; i++ )
        {
                spread_offset += (uint16_t)time_codes[spread_sequence][i];
                spread_offset = spread_offset % SPREAD_OFFSET_MAX;

                // Insert into the array of time_slots... make sure it is ordered
                for ( j = i; j > 0; j-- )
                {
                        uint8_t idx = j-1;

                        // Insertion sort
                        if ( spread_offset < time_slots[idx] )
                        {
                                time_slots[j] = time_slots[idx];
                                team_slots[j] = team_slots[idx];
                        }
                        else
                        {
                                time_slots[j] = spread_offset;
                                team_slots[j] = i;

                                // Exit sort (we are ordered so we don't have to go any further)
                                break;
                        }
                }

                // We are the head
                if ( j == 0 )
                {
                        time_slots[0] = spread_offset;
                        team_slots[0] = i;
                }
        }

	for ( i = 0; i < 21; i++ )
	{
		printf( "Time [%d] team [%d]\n", time_slots[i], team_slots[i] );
	}
}

void unpack_codes( uint8_t *packedCodes, tTimeSlot *pSlots )
{
	uint16_t i;
	uint16_t bytePos;
	uint16_t bitPos;
	uint16_t temp;
	uint16_t slot_pos;

	bytePos = 0;
	bitPos = 0;
	temp = 0;
	slot_pos=0;

	for ( i = 0; i < ((SLOT_TOTAL_COUNT*10)/8); i++ )
	{
		uint8_t from_tid, to_tid;

		if ( bitPos < 5 )
		{
			temp = (temp << 8) | packedCodes[bytePos++];
			bitPos+= 8;
		}

		from_tid = ((temp >> (bitPos-5)) & 0x1F);
		temp = (temp & ((1<<(bitPos-5))-1));
		bitPos -= 5;

		if ( bitPos < 5 )
		{
			temp = (temp << 8) | packedCodes[bytePos++];
			bitPos+= 8;
		}				
		
		to_tid = ((temp >> (bitPos-5)) & 0x1F);
		temp = (temp & ((1<<(bitPos-5))-1));
		bitPos -= 5;

		pSlots[slot_pos].from_tid = from_tid;
		pSlots[slot_pos].to_tid = to_tid;
		slot_pos++;
	}
}

void pack_codes( uint8_t *packedCodes, tTimeSlot *pSlots )
{
	uint16_t i;
	uint16_t bytePos;
	uint16_t bitPos;
	uint16_t temp;

	bytePos = 0;
	bitPos = 0;
	temp = 0;

	for ( i = 0; i < SLOT_TOTAL_COUNT; i++ )
	{
		uint8_t from_tid, to_tid;
		
		from_tid = pSlots[i].from_tid;
		to_tid = pSlots[i].to_tid;

		temp = (temp << 5) | (from_tid & 0x1F);
		bitPos += 5;
		
		if ( bitPos >= 8 )
		{
			// Write out a bit
			packedCodes[bytePos++] = (temp >> (bitPos-8));
			temp = (temp & ((1<<(bitPos-8))-1));

			bitPos -= 8;
		}

		temp = (temp << 5) | (to_tid & 0x1F);
		bitPos += 5;
		
		if ( bitPos >= 8 )
		{
			// Write out a bit
			packedCodes[bytePos++] = (temp >> (bitPos-8));
			temp = (temp & ((1<<(bitPos-8))-1));

			bitPos -= 8;
		}
	}

	if ( bitPos != 0 )
		printf( "Uneven amount!\n" );

	printf( "Packed %d bytes.\r\n", bytePos );
}


int main( void )
{
	uint16_t i = 0;
	uint8_t from_tid = 0, to_tid = 0;
	uint8_t packedCodes[525];
	tTimeSlot oTimeSlots[SLOT_TOTAL_COUNT];

	for ( i = 0; i < SLOT_TOTAL_COUNT; i++ )
	{
		oTimeSlots[i].from_tid = from_tid;
		oTimeSlots[i].to_tid = to_tid;
	
		from_tid++;
		if ( from_tid >= 22 )
			from_tid = 0;

		to_tid++;
		if ( to_tid >= 20 )
			to_tid = 0;	
	}

	printf( "Codes:\r\n" );
	for ( i = 0; i < SLOT_TOTAL_COUNT; i++ )
	{
		printf( "[%2d %2d]", oTimeSlots[i].from_tid, oTimeSlots[i].to_tid );
	}

	pack_codes( packedCodes, oTimeSlots );

	unpack_codes( packedCodes, oTimeSlots );
	
	printf( "Codes:\r\n" );
	for ( i = 0; i < SLOT_TOTAL_COUNT; i++ )
	{
		printf( "[%2d %2d]", oTimeSlots[i].from_tid, oTimeSlots[i].to_tid );
	}

	printf( "Client codes:\n" );
	test_client_code();
	printf( "Server codes:\n" );
	test_server_code();

}

#if 0
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
		fprintf( pFile, "\nuint8_t time_codes[16][420] = {\n" );
	
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
			fprintf( pFile, "\nuint8_t time_codes[16][21] = {\n" );

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

#endif 
