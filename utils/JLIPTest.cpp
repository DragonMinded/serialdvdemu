#if defined( _WIN32 )
#include "stdafx.h"
#include <windows.h>
#else
#include <stdio.h>
#include <wchar.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <sys/time.h>
#endif

#ifdef _WIN32
#define WAIT()                Sleep( 100 )
#define LONGWAIT()            Sleep( 1000 )
#else
typedef unsigned char BYTE;
typedef const char * LPCTSTR;
typedef wchar_t _TCHAR;
typedef int HANDLE;
typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef int32_t LONG;

#define PURGE_RXABORT       0
#define PURGE_RXCLEAR       0
#define PURGE_TXABORT       0
#define PURGE_TXCLEAR       0

/* Blocking wait time */
#define WAIT_TIME       1000

/* Wait for serial */
#define WAIT()                usleep( 100000 )
#define LONGWAIT()            usleep( 1000000 )

int _tstoi( _TCHAR *text )
{
    int len = wcslen( text );
    char *newtext = (char *)malloc( len * 4 );
    wcstombs( newtext, text , len * 4 );

    int out = atoi( newtext );
    free( newtext );

    return out;
}
#endif

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

/* Status stuff */
// This is invalid response packet data or otherwise unrecognized
#define STATUS_BAD_PACKET	  -1
// This response packet was recognized but failed the CRC check
#define STATUS_BAD_CRC		  -2
// The command sent was unknown
#define STATUS_UNKNOWN_CMD    0x01
// The command sent was executed properly
#define STATUS_OK             0x03
// The command sent was unable to be executed
#define STATUS_ERROR          0x05

/* Device status stuff */
// Could not read device status for some reason
#define DEVICE_STATUS_UNKNOWN 0
// The device is currently off
#define DEVICE_STATUS_OFF     1
// The device is currently on
#define DEVICE_STATUS_ON      2

typedef unsigned char * packet_t;

/* Drive status */
typedef enum {
	DRIVE_IDLE,
	DRIVE_EJECTED,
	DRIVE_INSERTING,
	DRIVE_READING,
	DRIVE_PLAYING,
	DRIVE_PAUSED,
	DRIVE_OFF,
	DRIVE_UNKNOWN_STATUS
} drive_status_t;

/* Disk type */
typedef enum {
	DISK_NONE,
	DISK_VIDEO_CD,
	DISK_AUDIO_CD,
	DISK_VIDEO_DVD,
	DISK_UNRECOGNIZED_CD,
	DISK_UNRECOGNIZED_DVD,
	DISK_UNKNOWN_STATUS
} disk_type_t;

/* Disk status */
typedef struct
{
	drive_status_t drive_status;
	disk_type_t disk_type;
} disk_status_t;

unsigned char NO_PARAMS[4] = { 0, 0, 0, 0 };

void PrintHex(const unsigned char * const data, int length )
{
    printf( "Length: %d bytes\n", length );

    for( int x = 0; x < length; x++ )
    {
        printf( "%02X ", data[x] );
    }

    printf( "\n" );
}

unsigned int GetHex(const _TCHAR *str)
{
	int i = 0;
	unsigned int value = 0;
	while( str[i] != 0 )
	{
		/* Make room for this character */
		value <<= 4;

		switch( str[i] )
		{
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				value |= str[i] - '0';
				break;
			case 'A':
			case 'B':
			case 'C':
			case 'D':
			case 'E':
			case 'F':
				value |= (str[i] - 'A') + 10;
				break;
			case 'a':
			case 'b':
			case 'c':
			case 'd':
			case 'e':
			case 'f':
				value |= (str[i] - 'a') + 10;
				break;
		}

		i++;
	}

	return value;
}

void PrintStatus( int status )
{
	switch( status )
	{
		case STATUS_BAD_PACKET:
			printf( "The device is not communicating.\n" );
			return;
		case STATUS_BAD_CRC:
			printf( "The device returned a bad CRC.\n" );
			return;
		case STATUS_UNKNOWN_CMD:
			printf( "The device did not recognize the command.\n" );
			return;
		case STATUS_OK:
			printf( "The device successfully executed the command.\n" );
			return;
		case STATUS_ERROR:
			printf( "The device could not execute the command.\n" );
			return;
		default:
			printf( "Unknown status!\n" );
			return;
	}
}

void PrintDriveStatus( drive_status_t status )
{
	switch( status )
	{
		case DRIVE_IDLE:
			printf( "The drive is idle.\n" );
			return;
		case DRIVE_EJECTED:
			printf( "The drive is ejected.\n" );
			return;
		case DRIVE_INSERTING:
			printf( "The drive is inserting a disk.\n" );
			return;
		case DRIVE_READING:
			printf( "The drive is reading a disk.\n" );
			return;
		case DRIVE_PLAYING:
			printf( "The drive is playing a disk.\n" );
			return;
		case DRIVE_PAUSED:
			printf( "The drive is paused.\n" );
			return;
		case DRIVE_OFF:
			printf( "The drive is off.\n" );
			return;
		case DRIVE_UNKNOWN_STATUS:
			printf( "The drive status is unknown.\n" );
			return;
	}
}

void PrintDiskType( disk_type_t type )
{
	switch( type )
	{
		case DISK_NONE:
			printf( "There is no disk in the drive.\n" );
			return;
		case DISK_VIDEO_CD:
			printf( "The disk is a video CD.\n" );
			return;
		case DISK_AUDIO_CD:
			printf( "The disk is an audio CD.\n" );
			return;
		case DISK_VIDEO_DVD:
			printf( "The disk is a video DVD.\n" );
			return;
		case DISK_UNRECOGNIZED_CD:
			printf( "The disk is an unrecognized CD.\n" );
			return;
		case DISK_UNRECOGNIZED_DVD:
			printf( "The disk is an unrecognized DVD.\n" );
			return;
		case DISK_UNKNOWN_STATUS:
			printf( "The disk type is unknown.\n" );
			return;
	}
}

HANDLE OpenSerial( const _TCHAR *port, int baud )
{
#ifdef _WIN32
    HANDLE hSerial = CreateFile(port, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	int rate;

	switch( baud )
	{
		case 9600:
			rate = CBR_9600;
			break;
		case 19200:
			rate = CBR_19200;
			break;
		case 38400:
			rate = CBR_38400;
			break;
		case 57600:
			rate = CBR_57600;
			break;
		case 115200:
			rate = CBR_115200;
			break;
		default:
			rate = CBR_9600;
			break;
	}

    DCB dcbSerialParams = {0};

    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    dcbSerialParams.BaudRate = rate;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = EVENPARITY;

    SetCommState(hSerial, &dcbSerialParams);

    COMMTIMEOUTS timeouts = { 0 };

    timeouts.ReadIntervalTimeout = 100;
    timeouts.ReadTotalTimeoutConstant = 100;
    timeouts.ReadTotalTimeoutMultiplier = 20;

    SetCommTimeouts(hSerial, &timeouts);
#else
    int rate;

    switch( baud )
    {
        case 9600:
            rate = B9600;
            break;
        case 19200:
            rate = B19200;
            break;
        case 38400:
            rate = B38400;
            break;
        case 57600:
            rate = B57600;
            break;
        case 115200:
            rate = B115200;
            break;
        default:
            rate = B9600;
            break;
    }

    struct termios tio;
    HANDLE hSerial = 0;

    memset(&tio,0,sizeof(tio));
    tio.c_iflag=0;
    tio.c_oflag=0;
    tio.c_cflag=CS8|CREAD|PARENB|CLOCAL;           // 8n1, see termios.h for more information
    tio.c_lflag=0;
    tio.c_cc[VMIN]=1;
    tio.c_cc[VTIME]=0;

    /* Convert to char */
    int portlen = wcslen( port );
    char *portname = (char *)malloc( portlen * 4 );
    wcstombs( portname, port, portlen * 4 );

    hSerial=open(portname, O_RDWR | O_NONBLOCK);
    if( hSerial < 0 ) { return -1; }
    cfsetospeed(&tio,rate);
    cfsetispeed(&tio,rate);

    tcsetattr(hSerial,TCSANOW,&tio);

    free( portname );
#endif

    return hSerial;
}

#ifndef _WIN32
uint64_t getMS()
{
    struct timeval tv;
    
    gettimeofday(&tv, NULL );

    return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

bool ReadFile ( HANDLE file, void *buf, DWORD length, DWORD *readAmt, void *reserved )
{
    uint64_t start = getMS();

    *readAmt = 0;

    while( true )
    {
        int amount = read( file, buf, length );

        if( amount < 0 )
        {
            if( errno == EAGAIN || errno == EWOULDBLOCK )
            {
                if( (getMS() - start) > WAIT_TIME )
                {
                    return false;
                }

                /* Try again */
                continue;
            }

            return false;

        }

        *readAmt = amount;

        return true;
    }
}

bool WriteFile( HANDLE file, const void * const buf, DWORD length, DWORD *writeAmt, void *reserved )
{
    uint64_t start = getMS();

    while( true )
    {
        int amount = write( file, buf, length );

        if( amount < 0 )
        {
            if( errno == EAGAIN || errno == EWOULDBLOCK )
            {
                if( (getMS() - start) > 1000 )
                {
                    return false;
                }

                /* Try again */
                continue;
            }

            return false;
        }
        else
        {
            *writeAmt = amount;
            return true;
        }
    }
}

void SetFilePointer ( HANDLE file, int off, void *reserved, int origin )
{
    lseek( file, off, origin );
}

void CloseHandle( HANDLE file )
{
    close( file );
}

bool PurgeComm( HANDLE serial, DWORD dwFlags )
{
    /* Clear out old data */
    do
    {
        unsigned char data[1024];
        DWORD length;
        ReadFile( serial, data, sizeof( data ), &length, 0 );

        if( length <= 0 ) { break; }
    } while( 1 );

    return true;
}
#endif

DWORD ReadSerial( HANDLE serial, unsigned char *data, unsigned int length )
{
    DWORD readLength = 0;
    ReadFile( serial, data, length, &readLength, 0 );
	return readLength;
}

DWORD WriteSerial( HANDLE serial, const unsigned char *data, unsigned int length )
{
    DWORD wroteLength = 0;
    WriteFile( serial, data, length, &wroteLength, 0 );
	return wroteLength;
}

void CloseSerial( HANDLE serial )
{
	CloseHandle( serial );
}

unsigned char CalcChecksum( const packet_t packet )
{
	unsigned char sum = 0x80;

	for( DWORD i = 0; i < (PACKET_LEN - 1); i++ )
	{
		sum = sum - (packet[i] & 0x7F);
	}

	return sum & 0x7F;
}

packet_t CreateEmptyPacket()
{
	return (packet_t)malloc(PACKET_LEN);
}

packet_t CreatePacket( unsigned int id, unsigned int command, unsigned char params[4] )
{
	packet_t data = (packet_t)malloc(PACKET_LEN);

	/* Create preamble */
	data[PREAMBLE_LOC + 0] = 0xFF;
	data[PREAMBLE_LOC + 1] = 0xFF;

	/* Stuff in ID */
	data[JLIP_ID_LOC] = id & 0xFF;

	/* Stuff in command */
	data[COMMAND_LOC + 0] = (command >> 16) & 0xFF;
	data[COMMAND_LOC + 1] = (command >> 8) & 0xFF;
	data[COMMAND_LOC + 2] = command & 0xFF;

	/* Stuff in params */
	memcpy(&data[PARAMS_LOC], params, 4);

	/* Calculate CRC */
	data[CRC_LOC] = CalcChecksum(data);

	/* Send it */
	return data;
}

int GetPacketStatus( packet_t packet )
{
	if( packet[PREAMBLE_LOC + 0] != 0xFC ||
		packet[PREAMBLE_LOC + 1] != 0xFF )
	{
		/* Not a response packet */
		return STATUS_BAD_PACKET;
	}

	if( CalcChecksum( packet ) != packet[CRC_LOC] )
	{
		/* Invalid CRC */
		return STATUS_BAD_CRC;
	}

	return packet[STATUS_LOC];
}

void DestroyPacket( packet_t packet )
{
	free( packet );
}

/**
 * Send a NOP to the device.  Useful for probing for devices.
 * Returns STATUS_OK if the device is present and talking.
 *
 * @retval STATUS_OK The device responded successfully
 * @retval STATUS_UNKNOWN_CMD The device did not recognize the command
 * @retval STATUS_BAD_PACKET The device is not present
 * @retval STATUS_BAD_CRC The device had an error in communicating
 */
int NoOp( HANDLE serial, unsigned int id )
{
	int status;
	packet_t request = CreatePacket( id, 0x7C4E20, NO_PARAMS );
	packet_t response = CreateEmptyPacket();
	WriteSerial( serial, request, PACKET_LEN );
	WAIT();
	ReadSerial( serial, response, PACKET_LEN );

	status = GetPacketStatus( response );

	DestroyPacket( request );
	DestroyPacket( response );

	return status;
}

/**
 * Ask the device for its machine code.  Returns a 6-byte malloc'd byte
 * array.  If there is an error reading the device, returns NULL.
 */
unsigned char * MachineCode( HANDLE serial, unsigned int id )
{
	unsigned char * result = (unsigned char *)malloc(6);
	packet_t request = CreatePacket( id, 0x7C4500, NO_PARAMS );
	packet_t response = CreateEmptyPacket();
	WriteSerial( serial, request, PACKET_LEN );
	WAIT();
	ReadSerial( serial, response, PACKET_LEN );

	if( GetPacketStatus( response ) == STATUS_OK )
	{
		memcpy( result, &response[DATA_LOC], 6 );
	}
	else
	{
		free( result );
		result = 0;
	}

	DestroyPacket( request );
	DestroyPacket( response );

	return result;
}

/**
 * Ask the device for its device code.  Returns a 6-byte malloc'd byte
 * array.  If there is an error reading the device, returns NULL.  Empty
 * bytes in the array will be filled with 0x7F.
 */
unsigned char * DeviceCode( HANDLE serial, unsigned int id )
{
	unsigned char * result = (unsigned char *)malloc(6);
	packet_t request = CreatePacket( id, 0x7C4900, NO_PARAMS );
	packet_t response = CreateEmptyPacket();
	WriteSerial( serial, request, PACKET_LEN );
	WAIT();
	ReadSerial( serial, response, PACKET_LEN );

	if( GetPacketStatus( response ) == STATUS_OK )
	{
		memcpy( result, &response[DATA_LOC], 6 );
	}
	else
	{
		free( result );
		result = 0;
	}

	DestroyPacket( request );
	DestroyPacket( response );

	return result;
}

/** 
 * Queries the device to see the maximum baud rate it can
 * operate.  Assume that it can always operate at 9600 baud.
 *
 * @retval 9600 The device can operate at 9600 baud.
 * @retval 19200 The device can operate at 9600 or 19200 baud.
 * @retval 38400 The device can operate at 9600, 19200 or 38400 baud.
 * @retval 115200 The device can operate at 9600, 19200, 38400 or 115200 baud.
 * @retval 0 The device didn't respond properly or the baud rate was unrecognized.  Assume 9600.
 */
unsigned int MaxBaudRate( HANDLE serial, unsigned int id )
{
	unsigned int rate = 0;
	packet_t request = CreatePacket( id, 0x7C4820, NO_PARAMS );
	packet_t response = CreateEmptyPacket();
	WriteSerial( serial, request, PACKET_LEN );
	WAIT();
	ReadSerial( serial, response, PACKET_LEN );

	if( GetPacketStatus( response ) == STATUS_OK )
	{
		switch( response[DATA_LOC] )
		{
			case 0x20:
				rate = 9600;
				break;
			case 0x21:
				rate = 19200;
				break;
			case 0x22:
				rate = 38400;
				break;
			case 0x23:
				rate = 115200;
				break;
			default:
				/* Unknown */
				rate = 0;
				break;
		}
	}
	else
	{
		/* Unknown */
		rate = 0;
	}

	DestroyPacket( request );
	DestroyPacket( response );

	return rate;
}

/**
 * Ask the device for its internl name.  Returns a malloc'd string
 * containing the device name as read from the device.  If the device
 * could not be read, returns NULL.
 */
char *DeviceName( HANDLE serial, unsigned int id )
{
	/* Maximum characters is 12 I think */
	char *name = (char *)malloc(13);
	name[0] = 0;

	packet_t request1 = CreatePacket( id, 0x7C4C00, NO_PARAMS );
	packet_t response1 = CreateEmptyPacket();
	packet_t request2 = CreatePacket( id, 0x7C4D00, NO_PARAMS );
	packet_t response2 = CreateEmptyPacket();

	WriteSerial( serial, request1, PACKET_LEN );
	WAIT();
	ReadSerial( serial, response1, PACKET_LEN );
	WAIT();
	WriteSerial( serial, request2, PACKET_LEN );
	WAIT();
	ReadSerial( serial, response2, PACKET_LEN );

	if( GetPacketStatus( response1 ) == STATUS_OK &&
		GetPacketStatus( response2 ) == STATUS_OK )
	{
		packet_t responses[2] = { response1, response2 };

		for( int y = 0; y < 2; y++ )
		{
			for( int x = 0; x < 6; x++ )
			{
				if( responses[y][DATA_LOC + x] != 0x7F )
				{
					char str[2] = { responses[y][DATA_LOC + x], 0x00 };
					strcat( name, str );
				}
			}
		}
	}
	else
	{
		/* Unknown */
		free( name );
		name = 0;
	}
	
	DestroyPacket( request1 );
	DestroyPacket( request2 );
	DestroyPacket( response1 );
	DestroyPacket( response2 );

	return name;
}

/**
 * Ask the device for the current length of the disk.  Returns a
 * malloc'd 3-integer array representing the minutes, seconds and fractions
 * of a second of content on the currently inserted disk.  If the details
 * couldn't be fetched, returns NULL.
 */
unsigned int *DiskLengthTime( HANDLE serial, unsigned int id )
{
	unsigned int *length = (unsigned int *)malloc((sizeof(unsigned int)) * 3);
	length[0] = 0;
	length[1] = 0;
	length[2] = 0;

	packet_t request = CreatePacket( id, 0x0C4C22, NO_PARAMS );
	packet_t response = CreateEmptyPacket();

	WriteSerial( serial, request, PACKET_LEN );
	WAIT();
	ReadSerial( serial, response, PACKET_LEN );

	if( GetPacketStatus( response ) == STATUS_OK )
	{
		length[0] = response[DATA_LOC + 0] * 10 + response[DATA_LOC + 1];
		length[1] = response[DATA_LOC + 2] * 10 + response[DATA_LOC + 3];
		length[2] = response[DATA_LOC + 4] * 10 + response[DATA_LOC + 5];
	}
	else
	{
		/* Unknown */
		free( length );
		length = 0;
	}
	
	DestroyPacket( request );
	DestroyPacket( response );

	return length;
}

/**
 * Ask the device for the number of chapters of the disk.  Returns an
 * integer representing the number of tracks.  If the device couldn't be
 * queried, returns 0.
 */
unsigned int DiskLengthChapters( HANDLE serial, unsigned int id )
{
	unsigned int length;
	packet_t request = CreatePacket( id, 0x0C4C23, NO_PARAMS );
	packet_t response = CreateEmptyPacket();

	WriteSerial( serial, request, PACKET_LEN );
	WAIT();
	ReadSerial( serial, response, PACKET_LEN );

	if( GetPacketStatus( response ) == STATUS_OK )
	{
		length = response[DATA_LOC + 2] * 10 + response[DATA_LOC + 3];
	}
	else
	{
		/* Unknown */
		length = 0;
	}
	
	DestroyPacket( request );
	DestroyPacket( response );

	return length;
}

/**
 * Ask the device for current disk status.  Returns a structure
 * representing the status of the disk drive and the type of media
 * detected.
 */
disk_status_t DiskStatus( HANDLE serial, unsigned int id )
{
	disk_status_t status;
	packet_t request = CreatePacket( id, 0x0C4E20, NO_PARAMS );
	packet_t response = CreateEmptyPacket();

	WriteSerial( serial, request, PACKET_LEN );
	WAIT();
	ReadSerial( serial, response, PACKET_LEN );

	if( GetPacketStatus( response ) == STATUS_OK )
	{
		/* Get drive status */
		switch( response[DATA_LOC + 0] )
		{
			case 0x01:
				status.drive_status = DRIVE_EJECTED;
				break;
			case 0x02:
				status.drive_status = DRIVE_INSERTING;
				break;
			case 0x10:
				status.drive_status = DRIVE_READING;
				break;
			case 0x20:
				status.drive_status = DRIVE_IDLE;
				break;
			case 0x40:
				status.drive_status = DRIVE_PLAYING;
				break;
			case 0x50:
				status.drive_status = DRIVE_PAUSED;
				break;
			case 0x6F:
				status.drive_status = DRIVE_OFF;
				break;
			default:
				status.drive_status = DRIVE_UNKNOWN_STATUS;
				break;
		}

		/* Get disk type */
		unsigned int type = (response[DATA_LOC + 2] << 8) | response[DATA_LOC + 3];
		switch( type )
		{
			case 0x4040:
				status.disk_type = DISK_NONE;
				break;
			case 0x4144:
				status.disk_type = DISK_VIDEO_CD;
				break;
			case 0x4440:
				status.disk_type = DISK_AUDIO_CD;
				break;
			case 0x4A40:
				status.disk_type = DISK_VIDEO_DVD;
				break;
			case 0x4840:
				status.disk_type = DISK_UNRECOGNIZED_DVD;
				break;
			case 0x414E:
			case 0x4140:
				status.disk_type = DISK_UNRECOGNIZED_CD;
				break;
			default:
				status.disk_type = DISK_UNKNOWN_STATUS;
				break;
		}
	}
	else
	{
		/* Unknown */
		status.drive_status = DRIVE_UNKNOWN_STATUS;
		status.disk_type = DISK_UNKNOWN_STATUS;
	}
	
	DestroyPacket( request );
	DestroyPacket( response );

	return status;
}

/**
 * Set the device to a new JLIP ID.
 *
 * @retval STATUS_BAD_PACKET The device isn't communicating
 * @retval STATUS_OK The device changed to the new ID
 * @retval STATUS_ERROR The ID was out of range
 */
int SetID( HANDLE serial, unsigned int id, unsigned int newid )
{
	int status;
	packet_t request = CreatePacket( id, 0x7C4100 | (newid & 0x3F), NO_PARAMS );
	packet_t response = CreateEmptyPacket();
	WriteSerial( serial, request, PACKET_LEN );
	WAIT();
	ReadSerial( serial, response, PACKET_LEN );

	status = GetPacketStatus( response );

	DestroyPacket( request );
	DestroyPacket( response );

	return status;
}

/**
 * Turn the device on.
 *
 * @retval STATUS_BAD_PACKET The device isn't communicating
 * @retval STATUS_OK The device was turned on
 * @retval STATUS_ERROR The device was already on
 */
int TurnOn( HANDLE serial, unsigned int id )
{
	int status;
	packet_t request = CreatePacket( id, 0x3E4070, NO_PARAMS );
	packet_t response = CreateEmptyPacket();
	WriteSerial( serial, request, PACKET_LEN );
	WAIT();
	ReadSerial( serial, response, PACKET_LEN );

	status = GetPacketStatus( response );

	DestroyPacket( request );
	DestroyPacket( response );

	return status;
}

/**
 * Turn the device off.
 *
 * @retval STATUS_BAD_PACKET The device isn't communicating
 * @retval STATUS_OK The device was turned off
 * @retval STATUS_ERROR The device was already off
 */
int TurnOff( HANDLE serial, unsigned int id )
{
	int status;
	packet_t request = CreatePacket( id, 0x3E4060, NO_PARAMS );
	packet_t response = CreateEmptyPacket();
	WriteSerial( serial, request, PACKET_LEN );
	WAIT();
	ReadSerial( serial, response, PACKET_LEN );

	status = GetPacketStatus( response );

	DestroyPacket( request );
	DestroyPacket( response );

	return status;
}

/** 
 * Queries the device for status and returns whether the device
 * is on or off.  If the device can't be read, returns unknown.
 *
 * @retval DEVICE_STATUS_UNKNOWN Could not probe the device for status
 * @retval DEVICE_STATUS_OFF The device is off or in standby
 * @retval DEVICE_STATUS_ON The device is on
 */
int PowerStatus( HANDLE serial, unsigned int id )
{
	unsigned int status;
	packet_t request = CreatePacket( id, 0x3E4E20, NO_PARAMS );
	packet_t response = CreateEmptyPacket();
	WriteSerial( serial, request, PACKET_LEN );
	WAIT();
	ReadSerial( serial, response, PACKET_LEN );

	if( GetPacketStatus( response ) != STATUS_OK )
	{
		status = DEVICE_STATUS_UNKNOWN;
	}
	else
	{
		switch( response[DATA_LOC] )
		{
			case 0x00:
				status = DEVICE_STATUS_OFF;
				break;
			case 0x01:
				status = DEVICE_STATUS_ON;
				break;
			default:
				status = DEVICE_STATUS_UNKNOWN;
				break;
		}
	}

	DestroyPacket( request );
	DestroyPacket( response );

	return status;
}

/**
 * Play the currently selected track or chapter on the device.  If the
 * device is off, turns the device on and starts playing at chapter/track
 * 1.  When the device is idle, starts playing at chapter/track 1.  When
 * the device is paused, resumes playing at the current location.  When
 * already playing, has no effect.
 *
 * @retval STATUS_OK The device executed the play command
 * @retval STATUS_BAD_PACKET The device isn't communicating
 */
int Play( HANDLE serial, unsigned int id )
{
	int status;
	packet_t request = CreatePacket( id, 0x0C4375, NO_PARAMS );
	packet_t response = CreateEmptyPacket();
	WriteSerial( serial, request, PACKET_LEN );
	WAIT();
	ReadSerial( serial, response, PACKET_LEN );

	status = GetPacketStatus( response );

	DestroyPacket( request );
	DestroyPacket( response );

	return status;
}

/**
 * Pause the currently playing track/chapter.  If the device is
 * off, has no effect.  If the device is idle, queues up track/chapter
 * 1 and starts paused.  If the device is playing, pauses the
 * track/chapter at the current location.  If the device is already
 * paused, it stays paused.
 *
 * @retval STATUS_OK The device executed the pause command
 * @retval STATUS_BAD_PACKET The device isn't communicating
 */
int Pause( HANDLE serial, unsigned int id )
{
	int status;
	packet_t request = CreatePacket( id, 0x0C436D, NO_PARAMS );
	packet_t response = CreateEmptyPacket();
	WriteSerial( serial, request, PACKET_LEN );
	WAIT();
	ReadSerial( serial, response, PACKET_LEN );

	status = GetPacketStatus( response );

	DestroyPacket( request );
	DestroyPacket( response );

	return status;
}

/**
 * Stop the device if it is currently playing.  If the device is off
 * or idle, this has no effect.  If the device is playing or paused,
 * its state is returned to idle.
 *
 * @retval STATUS_OK The device executed the stop command
 * @retval STATUS_BAD_PACKET The device isn't communicating
 */
int Stop( HANDLE serial, unsigned int id )
{
	int status;
	packet_t request = CreatePacket( id, 0x0C4460, NO_PARAMS );
	packet_t response = CreateEmptyPacket();
	WriteSerial( serial, request, PACKET_LEN );
	WAIT();
	ReadSerial( serial, response, PACKET_LEN );

	status = GetPacketStatus( response );

	DestroyPacket( request );
	DestroyPacket( response );

	return status;
}

/**
 * Seek to the next track/chapter on the device.  If the device is off
 * or the track/chapter is out of bounds, this will return an error.  If the
 * device is idle, this will seek to track/chapter 2 and play.  If the device
 * is playing, this will seek to the next track/chapter and play.  If the device
 * is paused, this will seek to the next track/chapter and remain paused at the 
 * start of the track/chapter.
 *
 * @retval STATUS_OK The device executed the seek command
 * @retval STATUS_ERROR The device was off or the track/chapter was out of bounds
 * @retval STATUS_BAD_PACKET The device isn't communicating
 */
int NextChapter( HANDLE serial, unsigned int id )
{
	int status;
	packet_t request = CreatePacket( id, 0x0C5073, NO_PARAMS );
	packet_t response = CreateEmptyPacket();
	WriteSerial( serial, request, PACKET_LEN );
	WAIT();
	ReadSerial( serial, response, PACKET_LEN );

	status = GetPacketStatus( response );
	DestroyPacket( request );
	DestroyPacket( response );

	return status;
}

/**
 * Seek to the previous track/chapter on the device.  If the device is off
 * or the track/chapter is out of bounds, this will return an error.  If the
 * device is idle, this will seek to track/chapter 1 and play.  If the device
 * is playing, this will seek to the previous track/chapter and play.  If the device
 * is paused, this will seek to the previous track/chapter and remain paused at the 
 * start of the track/chapter.
 *
 * @retval STATUS_OK The device executed the seek command
 * @retval STATUS_ERROR The device was off or the track/chapter was out of bounds
 * @retval STATUS_BAD_PACKET The device isn't communicating
 */
int PrevChapter( HANDLE serial, unsigned int id )
{
	int status;
	packet_t request = CreatePacket( id, 0x0C5061, NO_PARAMS );
	packet_t response = CreateEmptyPacket();
	WriteSerial( serial, request, PACKET_LEN );
	WAIT();
	ReadSerial( serial, response, PACKET_LEN );

	status = GetPacketStatus( response );

	DestroyPacket( request );
	DestroyPacket( response );

	return status;
}

/**
 * Seek to the specified track/chapter on the device.  If the device is off
 * or the track/chapter is out of bounds, this will return an error.  If the
 * device is idle, this will seek to the track/chapter and play.  If the device
 * is playing, this will seek to the new track/chapter and play.  If the device
 * is paused, this will seek to the new track/chapter and remain paused at the 
 * start of the track/chapter.
 *
 * @retval STATUS_OK The device executed the seek command
 * @retval STATUS_ERROR The device was off or the track/chapter was out of bounds
 * @retval STATUS_BAD_PACKET The device isn't communicating
 */
int SeekToChapter( HANDLE serial, unsigned int id, unsigned int chapter )
{
	int status;
	unsigned char params[4] = { 0x00, (unsigned char)((chapter / 10) % 10), (unsigned char)(chapter % 10), 0x00 };
	packet_t request = CreatePacket( id, 0x0C5020, params );
	packet_t response = CreateEmptyPacket();
	WriteSerial( serial, request, PACKET_LEN );
	WAIT();
	ReadSerial( serial, response, PACKET_LEN );

	status = GetPacketStatus( response );

	DestroyPacket( request );
	DestroyPacket( response );

	return status;
}

/**
 * Eject the drive tray on the device.  If the device is off or in the process of
 * identifying a disk, this will return an error.  If the device has already ejected,
 * this will also return an error.
 *
 * @retval STATUS_OK The device ejected the drive tray
 * @retval STATUS_ERROR The device was off or busy, or the device was already ejected
 * @retval STATUS_BAD_PACKET The device isn't communicating
 */
int Eject( HANDLE serial, unsigned int id )
{
	int status;
	packet_t request = CreatePacket( id, 0x0C4161, NO_PARAMS );
	packet_t response = CreateEmptyPacket();
	WriteSerial( serial, request, PACKET_LEN );
	WAIT();
	ReadSerial( serial, response, PACKET_LEN );

	status = GetPacketStatus( response );

	DestroyPacket( request );
	DestroyPacket( response );

	return status;
}

/**
 * Insert the drive tray on the device.  If the device is off or in the process of
 * inserting a disk, this will return an error.  If the device tray is closed, this will
 * instead perform an eject command.  Therefore it is not safe to call this assuming the
 * state will always be inserted after a successful execute.  It is recommneded to probe
 * the device status to determine the drive tray state.
 *
 * @retval STATUS_OK The device inserted the drive tray
 * @retval STATUS_ERROR The device was off or busy
 * @retval STATUS_BAD_PACKET The device isn't communicating
 */
int Insert( HANDLE serial, unsigned int id )
{
	int status;
	packet_t request = CreatePacket( id, 0x0C4171, NO_PARAMS );
	packet_t response = CreateEmptyPacket();
	WriteSerial( serial, request, PACKET_LEN );
	WAIT();
	ReadSerial( serial, response, PACKET_LEN );

	status = GetPacketStatus( response );

	DestroyPacket( request );
	DestroyPacket( response );

	return status;
}

void PrintInstructions( _TCHAR * name )
{
	fwprintf( stderr, L"\n" );
	fwprintf( stderr, L"%ls port id command\n", name );
	fwprintf( stderr, L"\tport - a com port, such as COM1.\n" );
	fwprintf( stderr, L"\tid - a JLIP ID, such as 33.  Use 0 to probe.\n" );
	fwprintf( stderr, L"\tcommand - a command to perform.  Valid commands below.\n" );
	fwprintf( stderr, L"\n" );
	fwprintf( stderr, L"\tturnon\t\tturn the device on.\n");
	fwprintf( stderr, L"\tturnoff\t\tturn the device off.\n");
	fwprintf( stderr, L"\teject\t\teject the device tray.\n");
	fwprintf( stderr, L"\tinsert\t\tinsert the device tray.\n");
	fwprintf( stderr, L"\tstatus\t\tprint device status information.\n");
	fwprintf( stderr, L"\tdiskstatus\t\tprint disk status information.\n");
	fwprintf( stderr, L"\tsetid x\t\tset the device to listen on JLIP ID x.\n");
	fwprintf( stderr, L"\tplay\t\tplay current chapter/track on device.\n");
	fwprintf( stderr, L"\tpause\t\tpause playback on device.\n");
	fwprintf( stderr, L"\tstop\t\tstop playing and return to idle state.\n");
	fwprintf( stderr, L"\tnextchapter\tseek to next chapter.\n");
	fwprintf( stderr, L"\tpreviouschapter\tseek to previous chapter.\n");
	fwprintf( stderr, L"\tseekchapter x\tseek to chapter x (range 1-99).\n");
	fwprintf( stderr, L"\n" );
	fwprintf( stderr, L"\traw xxyyzz aa bb cc dd\tperform raw command xxyyzz with params aa, bb, cc, dd.\n");
	fwprintf( stderr, L"\tsweep xx[yy[zz]]\tdo a raw command sweep with prefix xx[yy[zz]].\n");
}

#ifdef _WIN32
int _tmain(int argc, _TCHAR* argv[])
{
#else
int main(int argc, char *_targv[])
{
    /* Convert to wide */
    _TCHAR ** argv = (_TCHAR **)malloc( sizeof(_TCHAR *) * (argc + 1) );
    argv[argc] = NULL;

    for( int tmp = 0; tmp < argc; tmp++ )
    {
        int len = strlen(_targv[tmp]);
        argv[tmp] = (_TCHAR *)malloc( sizeof(_TCHAR) * (len + 1) );
        mbstowcs( argv[tmp], _targv[tmp],  sizeof(_TCHAR) * (len + 1) );
    }
#endif
    /* Ensure command is good */
    if( argc < 2 )
    {
        fwprintf( stderr, L"Missing serial port argument!\n" );
		PrintInstructions( argv[0] );
        return 1;
    }

	if( argc < 3 )
	{
        fwprintf( stderr, L"Missing JLIP ID argument!\n" );
		PrintInstructions( argv[0] );
        return 1;
	}

	if( argc < 4 )
	{
        fwprintf( stderr, L"Missing command argument!\n" );
		PrintInstructions( argv[0] );
        return 1;
	}

    /* Get serial device */
    HANDLE serial = OpenSerial( argv[1], 9600 );
	unsigned int id = _tstoi( argv[2] );

    if( serial < 0 )
    {
        fwprintf( stderr, L"Failed to open serial port '%ls'!\n", argv[1] );
        return 1;
    }

	/* Ensure we start fresh */
	PurgeComm( serial, PURGE_RXABORT | PURGE_RXCLEAR | PURGE_TXABORT | PURGE_TXCLEAR );
	
	if( id < 1 || id > 63 )
	{
		printf("Invalid JLIP ID, attempting to probe...\n");

		for( unsigned int i = 1; i <= 63; i++ )
		{
			if( NoOp( serial, i ) == STATUS_OK )
			{
				/* This is it! */
				printf("JLIP device found at address %d.\n", i);
				id = i;
				break;
			}
		}
	}

	if( id < 1 || id > 63 )
	{
		fwprintf( stderr, L"Unable to find a device!\n" );
	}
	else
	{
		if( wcscmp( argv[3], L"status" ) == 0 )
		{
			printf("Attempting to read device status...\n\n");

			char * name = DeviceName( serial, id );
			if( name )
			{
				printf("Device name is '%s'\n", name );
				free( name );
			}
			else
			{
				printf("Couldn't read name!\n");
			}

			int mbr = MaxBaudRate( serial, id );
			if( mbr > 0 )
			{
				printf("Device max baud rate is '%d'\n", mbr );
			}
			else
			{
				printf("Device max baud rate is UNKNOWN\n");
			}

			switch( PowerStatus( serial, id ) )
			{
				case DEVICE_STATUS_UNKNOWN:
					printf("Device status is UNKNOWN\n");
					break;
				case DEVICE_STATUS_OFF:
					printf("Device status is OFF\n");
					break;
				case DEVICE_STATUS_ON:
					printf("Device status is ON\n");
					break;
			}

			unsigned char *mc = MachineCode( serial, id );
			if( mc )
			{
				printf("Device machine code is %02X %02X %02X %02X %02X %02X\n",
					   mc[0], mc[1], mc[2], mc[3], mc[4], mc[5] );
				free( mc );
			}
			else
			{
				printf("Couldn't read machine code!\n");
			}

			unsigned char *dc = DeviceCode( serial, id );
			if( dc )
			{
				printf("Device code is");
				for( int i = 0; i < 6; i++ )
				{
					if( dc[i] != 0x7F )
					{
						printf(" %02X", dc[i] );
					}
				}

				printf("\n");
				free( dc );
			}
			else
			{
				printf("Couldn't read device code!\n");
			}
		}
		else if( wcscmp( argv[3], L"diskstatus" ) == 0 )
		{
			printf("Attempting to read disk status...\n\n");

			disk_status_t status = DiskStatus( serial, id );
			PrintDriveStatus( status.drive_status );

			if( status.drive_status != DRIVE_OFF )
			{
				PrintDiskType( status.disk_type );

				if( status.drive_status != DRIVE_EJECTED &&
					status.drive_status != DRIVE_INSERTING &&
					status.drive_status != DRIVE_READING &&
					status.drive_status != DRIVE_OFF &&
					status.disk_type != DISK_NONE )
				{
					unsigned int chapters = DiskLengthChapters( serial, id );
					if( chapters )
					{
						printf("Disk has %d chapers/tracks.\n", chapters );
					}
					else
					{
						printf("Couldn't read disk chapters!\n");
					}

					unsigned int *length = DiskLengthTime( serial, id );
					if( length )
					{
						printf("Disk length is %d:%d.%d\n",
							   length[0], length[1], length[2] );
						free( length );
					}
					else
					{
						printf("Couldn't read disk length!\n");
					}
				}
			}
		}
		else if( wcscmp( argv[3], L"turnon" ) == 0 )
		{
			int status = TurnOn( serial, id );

			if( status == STATUS_ERROR )
			{
				printf( "The device was already on.\n" );
			}
			else
			{
				PrintStatus( status );
			}
		}
		else if( wcscmp( argv[3], L"turnoff" ) == 0 )
		{
			int status = TurnOff( serial, id );

			if( status == STATUS_ERROR )
			{
				printf( "The device was already off.\n" );
			}
			else
			{
				PrintStatus( status );
			}
		}
		else if( wcscmp( argv[3], L"eject" ) == 0 )
		{
			PrintStatus(Eject( serial, id ));
		}
		else if( wcscmp( argv[3], L"insert" ) == 0 )
		{
			PrintStatus(Insert( serial, id ));
		}
		else if( wcscmp( argv[3], L"play" ) == 0 )
		{
			PrintStatus(Play( serial, id ));
		}
		else if( wcscmp( argv[3], L"pause" ) == 0 )
		{
			PrintStatus(Pause( serial, id ));
		}
		else if( wcscmp( argv[3], L"stop" ) == 0 )
		{
			PrintStatus(Stop( serial, id ));
		}
		else if( wcscmp( argv[3], L"nextchapter" ) == 0 )
		{
			PrintStatus(NextChapter( serial, id ));
		}
		else if( wcscmp( argv[3], L"previouschapter" ) == 0 )
		{
			PrintStatus(PrevChapter( serial, id ));
		}
		else if( wcscmp( argv[3], L"setid" ) == 0 )
		{
			unsigned int newid = 33;
			if( argc < 5 )
			{
				fwprintf( stderr, L"Unknown JLIP ID, assumming 33!\n" );
			}
			else
			{
				newid = _tstoi( argv[4] );
			}

			PrintStatus(SetID( serial, id, newid ));
		}
		else if( wcscmp( argv[3], L"seekchapter" ) == 0 )
		{
			unsigned int chapter = 1;
			if( argc < 5 )
			{
				fwprintf( stderr, L"Unknown chapter, assuming chapter 1!\n" );
			}
			else
			{
				chapter = _tstoi( argv[4] );
			}

			PrintStatus(SeekToChapter( serial, id, chapter ));
		}
		else if( wcscmp( argv[3], L"raw" ) == 0 )
		{
			if( argc < 9 )
			{
				fwprintf( stderr, L"Invalid raw command format!\n" );
			}
			else
			{
				unsigned int command = GetHex(argv[4]);
				unsigned char params[4] = { (unsigned char)(GetHex(argv[5]) & 0xFF),
											(unsigned char)(GetHex(argv[6]) & 0xFF),
											(unsigned char)(GetHex(argv[7]) & 0xFF),
											(unsigned char)(GetHex(argv[8]) & 0xFF) };

				packet_t request = CreatePacket( id, command, params );
				packet_t response = CreateEmptyPacket();

				WriteSerial( serial, request, PACKET_LEN );
				WAIT();
				ReadSerial( serial, response, PACKET_LEN );

				printf("Request RAW:\n");
				PrintHex( request, PACKET_LEN );
				printf("Response RAW:\n");
				PrintHex( response, PACKET_LEN );

				DestroyPacket( request );
				DestroyPacket( response );
			}
		}
		else if( wcscmp( argv[3], L"sweep" ) == 0 )
		{
			if( argc < 5 )
			{
				fwprintf( stderr, L"Invalid sweep command format!\n" );
			}
			else
			{
				unsigned int command = GetHex(argv[4]);
				unsigned int commandlen = wcslen(argv[4]);
				unsigned int sweeplen = (6 - commandlen);

				if( sweeplen > 0 )
				{
					int sweepmax = 1 << (sweeplen * 4);
					command <<= (sweeplen * 4);

					for( int i = 0; i < sweepmax; i++ )
					{
						packet_t request = CreatePacket( id, command | i, NO_PARAMS );
						packet_t response = CreateEmptyPacket();

						WriteSerial( serial, request, PACKET_LEN );
						WAIT();
						ReadSerial( serial, response, PACKET_LEN );

						printf("Request RAW:\n");
						PrintHex( request, PACKET_LEN );
						printf("Response RAW:\n");
						PrintHex( response, PACKET_LEN );
						printf("\n");

						DestroyPacket( request );
						DestroyPacket( response );
					}
				}
				else
				{
					printf( "No range to sweep!\n" );
				}
			}
		}
		else
		{
			fwprintf( stderr, L"Unrecognized command!\n" );
			PrintInstructions( argv[0] );
		}
	}
	
	/* Close the connection */
	CloseSerial( serial );

    return 0;
}
