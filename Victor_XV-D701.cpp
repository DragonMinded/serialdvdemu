#include <stdio.h>
#include <stdint.h>
#include <malloc.h>
#include <memory.h>
#include "Victor_XV-D701.h"
#include "Video.h"
#include "DVDEmu.h"

#define PACKET_DEBUG          0

/* Wait for serial */
#define WAIT()                usleep( 100000 )
#define LONGWAIT()            usleep( 1000000 )

/* Requests */
#define PREAMBLE_LOC          0
#define JLIP_ID_LOC           2
#define COMMAND_LOC           3
#define PARAMS_LOC            6

/* Responses */
#define PREAMBLE_LOC          0
#define JLIP_ID_LOC           2
#define STATUS_LOC            3
#define DATA_LOC              4

/* Generic */
#define PACKET_LEN            11
#define CRC_LOC               10

unsigned char NO_RESPONSE[6] = { 0, 0, 0, 0, 0, 0 };

// The command sent was unknown
#define STATUS_UNKNOWN_CMD    0x01
// The command sent was executed properly
#define STATUS_OK             0x03
// The command sent was unable to be executed
#define STATUS_ERROR          0x05

typedef unsigned char * packet_t;

/* Emulator state */
struct victor_state
{
    unsigned int jlip_id;
    unsigned int dvd_type;
    unsigned char packet[PACKET_LEN];
    packet_t response;
} victor_state;

unsigned char CalcChecksum( const packet_t packet )
{
	unsigned char sum = 0x80;

	for( uint32_t i = 0; i < (PACKET_LEN - 1); i++ )
	{
		sum = sum - (packet[i] & 0x7F);
	}

	return sum & 0x7F;
}

packet_t CreatePacket( unsigned int id, unsigned int status, unsigned char response[6] )
{
	packet_t data = (packet_t)malloc(PACKET_LEN);

	/* Create preamble */
	data[PREAMBLE_LOC + 0] = 0xFC;
	data[PREAMBLE_LOC + 1] = 0xFF;

	/* Stuff in ID */
	data[JLIP_ID_LOC] = id & 0xFF;
    
    /* Stuff in response status */
    data[STATUS_LOC] = status & 0xFF;

	/* Stuff in data */
	memcpy(&data[DATA_LOC], response, 6);

	/* Calculate CRC */
	data[CRC_LOC] = CalcChecksum(data);

	/* Send it */
	return data;
}

int IsPacketGood( packet_t packet )
{
	if( packet[PREAMBLE_LOC + 0] != 0xFF ||
		packet[PREAMBLE_LOC + 1] != 0xFF )
	{
		/* Not a response packet */
		return 0;
	}

	if( CalcChecksum( packet ) != packet[CRC_LOC] )
	{
		/* Invalid CRC */
		return 0;
	}

	return 1;
}

void DestroyPacket( packet_t packet )
{
	free( packet );
}

packet_t HandleMediaPacket( packet_t packet )
{
    switch( packet[COMMAND_LOC + 1] )
    {
        case 0x41:
        {
            /* TODO: Drive commands */
            break;
        }
        case 0x43:
        {
            /* Play commands */
            if( packet[COMMAND_LOC + 2] == 0x6D )
            {
                /* Pause */
                verbose_printf( "Device pause request.\n" );

                Pause();

                return CreatePacket( victor_state.jlip_id, STATUS_OK, NO_RESPONSE );
            }
            else if( packet[COMMAND_LOC + 2] == 0x75 )
            {
                /* Play */
                verbose_printf( "Device play request.\n" );

                Play();

                return CreatePacket( victor_state.jlip_id, STATUS_OK, NO_RESPONSE );
            }

            break;
        }
        case 0x44:
        {
            /* Stop commands */
            if( packet[COMMAND_LOC + 2] == 0x60 )
            {
                /* Stop */
                verbose_printf( "Device stop request.\n" );

                Stop();

                return CreatePacket( victor_state.jlip_id, STATUS_OK, NO_RESPONSE );
            }

            break;
        }
        case 0x4C:
        {
            /* TODO; Disk parameter commands */
            break;
        }
        case 0x4E:
        {
            /* TODO: Disk status commands */
            break;
        }
        case 0x50:
        {
            /* Seek commands */
            if( packet[COMMAND_LOC + 2] == 0x20 )
            {
                /* Seek to chapter */
                unsigned int chapter = ((packet[PARAMS_LOC + 0] % 10) * 100) + ((packet[PARAMS_LOC + 1] % 10) * 10) + (packet[PARAMS_LOC + 2] % 10);

                if( victor_state.dvd_type == VICTOR_VCD )
                {
                    /* VCD can only go to 99, so it sicks the data in the first two spots */
                    chapter /= 10;
                }

                verbose_printf( "Device seek to chapter %d request.\n", chapter );

                if( SeekToChapter( chapter ) )
                {
                    return CreatePacket( victor_state.jlip_id, STATUS_OK, NO_RESPONSE );
                }
                else
                {
                    return CreatePacket( victor_state.jlip_id, STATUS_ERROR, NO_RESPONSE );
                }
            }
            else if( packet[COMMAND_LOC + 2] == 0x61 )
            {
                /* Previous chapter */
                verbose_printf( "Device seek to previous chapter request.\n" );

                unsigned int chapter = GetChapter() - 1;

                /* Special case where if we're idle and seek backwards, we start at chapter 1 */
                if( !IsPlaying() && chapter == 0 )
                {
                    chapter = 1;
                }

                if( SeekToChapter( chapter ) )
                {
                    return CreatePacket( victor_state.jlip_id, STATUS_OK, NO_RESPONSE );
                }
                else
                {
                    return CreatePacket( victor_state.jlip_id, STATUS_ERROR, NO_RESPONSE );
                }
            }
            else if( packet[COMMAND_LOC + 2] == 0x73 )
            {
                /* Next chapter */
                verbose_printf( "Device seek to next chapter request.\n" );

                unsigned int chapter = GetChapter() + 1;
                
                if( SeekToChapter( chapter ) )
                {
                    return CreatePacket( victor_state.jlip_id, STATUS_OK, NO_RESPONSE );
                }
                else
                {
                    return CreatePacket( victor_state.jlip_id, STATUS_ERROR, NO_RESPONSE );
                }
            }

            break;
        }
    }

    return NULL;
}

packet_t HandlePowerPacket( packet_t packet )
{
    switch( packet[COMMAND_LOC + 1] )
    {
        case 0x40:
        {
            /* Power commands */
            if( packet[COMMAND_LOC + 2] == 0x60 )
            {
                /* Turn off device */
                verbose_printf( "Device turn off request.\n" );

                int status = IsPowered() ? STATUS_OK : STATUS_ERROR;
                if( status == STATUS_OK )
                {
                    PowerOff();
                }
                return CreatePacket( victor_state.jlip_id, status, NO_RESPONSE );
            }
            else if( packet[COMMAND_LOC + 2] == 0x70 )
            {
                /* Turn on device */
                verbose_printf( "Device turn on request.\n" );

                int status = IsPowered() ? STATUS_ERROR : STATUS_OK;
                if( status == STATUS_OK )
                {
                    PowerOn();
                }
                return CreatePacket( victor_state.jlip_id, status, NO_RESPONSE );
            }
            break;
        }
        case 0x4E:
        {
            /* Power status commands */
            if( packet[COMMAND_LOC + 2] == 0x20 )
            {
                /* Status */
                verbose_printf( "Device power status request.\n" );

                unsigned char response[6] = { 0, 0x20, 0, 0, 0, 0 };
                response[0] = IsPowered() ? 0x01 : 0x0;
                return CreatePacket( victor_state.jlip_id, STATUS_OK, response );
            }
            break;
        }
    }

    /* Unknown, let the master handler send back a failure */
    return NULL;
}

packet_t HandleDevicePacket( packet_t packet )
{
    if( packet[COMMAND_LOC + 1] == 0x41 )
    {
        /* Change JLIP ID packet */
        unsigned int new_id = packet[COMMAND_LOC + 2];

        if( new_id >= 1 && new_id <= 63 )
        {
            /* Switch IDs */
            verbose_printf( "Change JLIP ID to %d\n", new_id );

            packet_t response = CreatePacket( victor_state.jlip_id, STATUS_OK, NO_RESPONSE );
            victor_state.jlip_id = new_id; /* TODO: Save this to file */
            return response;
        }
        else
        {
            /* Out of range */
            return CreatePacket( victor_state.jlip_id, STATUS_ERROR, NO_RESPONSE );
        }
    }
    else if( packet[COMMAND_LOC + 1] == 0x45 &&
             packet[COMMAND_LOC + 2] == 0x00 )
    {
        /* Get machine code */
        verbose_printf( "Machine code request.\n");

        unsigned char response[6] = { 0x00, 0x01, 0x03, 0x00, 0x03, 0x01 };
        return CreatePacket( victor_state.jlip_id, STATUS_OK, response );
    }
    else if( packet[COMMAND_LOC + 1] == 0x48 &&
             packet[COMMAND_LOC + 2] == 0x20 )
    {
        /* Get max baud rate */
        verbose_printf( "Baud rate request.\n");

        /* Hardcode to 9600 baud for now */
        unsigned char response[6] = { 0x20, 0x00, 0x00, 0x00, 0x00, 0x00 };
        return CreatePacket( victor_state.jlip_id, STATUS_OK, response );
    }
    else if( packet[COMMAND_LOC + 1] == 0x49 &&
             packet[COMMAND_LOC + 2] == 0x00 )
    {
        /* Get machine code */
        verbose_printf( "Device code request.\n");

        unsigned char response[6] = { 0x03, 0x0C, 0x7F, 0x7F, 0x7F, 0x7F };
        return CreatePacket( victor_state.jlip_id, STATUS_OK, response );
    }
    else if( packet[COMMAND_LOC + 1] == 0x4C &&
             packet[COMMAND_LOC + 2] == 0x00 )
    {
        /* Get first 6 bytes of name */
        verbose_printf( "Device name first half request.\n" );

        unsigned char response[6] = { 'D', 'V', 'D', ' ', 'P', 'L' };
        return CreatePacket( victor_state.jlip_id, STATUS_OK, response );
    }
    else if( packet[COMMAND_LOC + 1] == 0x4D &&
             packet[COMMAND_LOC + 2] == 0x00 )
    {
        /* Get first 6 bytes of name */
        verbose_printf( "Device name last half request.\n" );

        unsigned char response[6] = { 'A', 'Y', 'E', 'R', 0x7F, 0x7F };
        return CreatePacket( victor_state.jlip_id, STATUS_OK, response );
    }
    else if( packet[COMMAND_LOC + 1] == 0x4E &&
             packet[COMMAND_LOC + 2] == 0x20 )
    {
        /* Get machine code */
        verbose_printf( "NOP request.\n");

        unsigned char response[6] = { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00 };
        return CreatePacket( victor_state.jlip_id, STATUS_OK, response );
    }

    return NULL;
}

packet_t HandlePacket( packet_t packet )
{
    /* Check that the checksum is good first */
    if( !IsPacketGood( packet ) )
    {
        /* In the case its invalid or unrecognized, we do nothing */
        return NULL;
    }

    /* Check to see that it's the right ID */
    if( packet[JLIP_ID_LOC] != victor_state.jlip_id )
    {
        /* If it isn't destined for us, we do nothing */
        return NULL;
    }

    if( PACKET_DEBUG ) { PrintHex( "= Incoming: ", packet, PACKET_LEN ); }

    /* Parse the packet out */
    packet_t response = NULL;
    switch( packet[COMMAND_LOC] )
    {
        case 0x0C:
            response = HandleMediaPacket( packet );
            break;
        case 0x3E:
            response = HandlePowerPacket( packet );
            break;
        case 0x7C:
            response = HandleDevicePacket( packet );
            break;
    }

    if( !response )
    {
        /* Unknown */
        PrintHex( "Unknown packet ", packet, PACKET_LEN );
        response = CreatePacket( victor_state.jlip_id, STATUS_UNKNOWN_CMD, NO_RESPONSE );
    }

    return response;
}

void VictorInit( int type )
{
    victor_state.jlip_id = 33; /* TODO: Read this from file */
    victor_state.dvd_type = type;
    memset( victor_state.packet, 0, PACKET_LEN );
    victor_state.response = NULL;

    verbose_printf( "Emulating Victor XV-D701 %s.\n", victor_state.dvd_type == VICTOR_VCD ? "VCD" : "DVD" );
}

int VictorReceiveByte( unsigned char byte )
{
    memmove( &victor_state.packet[0], &victor_state.packet[1], PACKET_LEN - 1 );
    victor_state.packet[PACKET_LEN - 1] = byte;

    if( victor_state.response )
    {
        /* Destroy previous packet */
        DestroyPacket( victor_state.response );
    }

    victor_state.response = HandlePacket( victor_state.packet );
    if( victor_state.response )
    {
        if( PACKET_DEBUG ) { PrintHex( "= Outgoing: ", victor_state.response, PACKET_LEN ); }
        return 1;
    }

    return 0;
}

unsigned char *VictorGetResponse( int *length )
{
    if( victor_state.response )
    {
        *length = PACKET_LEN;
        return (unsigned char *)victor_state.response;
    }

    *length = 0;
    return NULL;
}
