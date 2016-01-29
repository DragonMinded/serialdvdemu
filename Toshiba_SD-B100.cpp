#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <memory.h>
#include "Toshiba_SD-B100.h"
#include "DVDEmu.h"
#include "Video.h"

#define PACKET_DEBUG 0

#define PACKET_LEN 256

/* Emulator state */
struct toshiba_state
{
    char packet[PACKET_LEN];
    char response[PACKET_LEN];
} toshiba_state;


void ToshibaInit()
{
    memset( toshiba_state.packet, 0, sizeof(toshiba_state.packet) );
    memset( toshiba_state.response, 0, sizeof(toshiba_state.response) );

    /* Videos seem to be slightly ahead, probably because of built-in delay in original player */
    VideoSetPauseDelay( 250 );
}

void ProcessPacket( const char *in, char *out )
{
    /* Assume valid packet */
    strcpy( out, "K" );

    if( strncmp( in, "WG", 2 ) == 0 )
    {
        /* Ping request I think, no action needed.
         * Characters 3+ are ascii digits for some parameter.
         */
    }
    else if( strcmp( in, "RE" ) == 0 )
    {
        /* Reset? */
        verbose_printf( "Reset request received.\n" );
        PowerOn();
    }
    else if( strcmp( in, "CL" ) == 0 )
    {
        /* Clear? */
        verbose_printf( "Clear request received.\n" );
        PowerOn();
    }
    else if( strncmp( in, "KL", 2 ) == 0 )
    {
        /* Unknown packet, Characters 3+ are ascii digits. */
    }
    else if( strncmp( in, "VM", 2 ) == 0 )
    {
        /* Unknown packet, Characters 3+ are ascii digits.
         * Observed values are '0' before loading chapter,
         * '1' after loading chapter when playback should
         * start.
         */
    }
    else if( strncmp( in, "RP", 2 ) == 0 )
    {
        /* Repeat count */
        int rpCnt = atoi( &in[2] );
        verbose_printf( "Repeat request for %d times received.\n", rpCnt );

        /* TODO: Implement multiple repeats */
    }
    else if( strcmp( in, "PL" ) == 0 )
    {
        /* Play */
        verbose_printf( "Play request received.\n" );
        Play();
    }
    else if( strcmp( in, "PU" ) == 0 )
    {
        /* Play */
        verbose_printf( "Pause request received.\n" );
        Pause();
    }
    else if( strncmp( in, "CS", 2 ) == 0 )
    {
        /* Chapter select */
        int title = 0;
        int chapter = 0;
        sscanf( &in[2], "%d,%d", &title, &chapter );
        verbose_printf( "Chapter select request for title %d, chapter %d.\n", title, chapter );

        /* TODO: Something with titles? */
        SeekToChapter( chapter );
    }
    else
    {
        /* Unknown packet? */
        verbose_printf( "Unknown packet '%s' received!", in );
        strcpy( out, "E" );
    }
}

int ToshibaReceiveByte( unsigned char byte )
{
    if( strlen(toshiba_state.response) > 0 )
    {
        memset( toshiba_state.response, 0, sizeof(toshiba_state.response) );
    }

    if( byte == 0x0d )
    {
        /* Finished packet */
        if( PACKET_DEBUG ) { PrintHex( "= Incoming: ", (const unsigned char *)toshiba_state.packet, strlen(toshiba_state.packet) ); }
        ProcessPacket( toshiba_state.packet, toshiba_state.response );
        memset( toshiba_state.packet, 0, sizeof(toshiba_state.packet) );
        return 1;
    }
    else
    {
        /* Add to received data */
        toshiba_state.packet[strlen(toshiba_state.packet)] = byte;
    }

    return 0;
}

unsigned char *ToshibaGetResponse( int *length )
{
    if( strlen(toshiba_state.response) > 0 )
    {
        /* Add a 0x0d to the end and return */
        toshiba_state.response[strlen(toshiba_state.response)] = 0x0d;
        *length = strlen(toshiba_state.response);
        if( PACKET_DEBUG ) { PrintHex( "= Outgoing: ", (const unsigned char *)toshiba_state.response, strlen(toshiba_state.response) ); }
        return (unsigned char *)toshiba_state.response;
    }

    /* Nothing to respond with */
    *length = 0;
    return NULL;
}
