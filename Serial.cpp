#include <stdio.h>
#include <stdarg.h>
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
#include <sys/wait.h>
#include <dirent.h>
#include "Serial.h"

/* Blocking wait time */
#define WAIT_TIME       1000

int OpenSerial( const char *port, int baud, int parity )
{
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
            /* Invalid baud rate */
            return -1;
    }

    struct termios tio;
    int hSerial = 0;

    memset(&tio,0,sizeof(tio));
    tio.c_iflag=0;
    tio.c_oflag=0;
    tio.c_cflag=CS8|CREAD|PARENB|CLOCAL;
    tio.c_lflag=0;
    tio.c_cc[VMIN]=1;
    tio.c_cc[VTIME]=0;

    hSerial=open(port, O_RDWR | O_NONBLOCK);
    if( hSerial < 0 ) { return -1; }
    cfsetospeed(&tio,rate);
    cfsetispeed(&tio,rate);

    tcsetattr(hSerial,TCSANOW,&tio);

    return hSerial;
}

uint64_t getMS()
{
    struct timeval tv;
    
    gettimeofday(&tv, NULL );

    return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

bool ReadFile ( int file, void *buf, uint32_t length, uint32_t *readAmt, void *reserved )
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

bool WriteFile( int file, const void * const buf, uint32_t length, uint32_t *writeAmt, void *reserved )
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

void SetFilePointer ( int file, int off, void *reserved, int origin )
{
    lseek( file, off, origin );
}

void CloseHandle( int file )
{
    close( file );
}

bool PurgeComm( int serial )
{
    /* Clear out old data */
    do
    {
        unsigned char data[1024];
        uint32_t length;
        ReadFile( serial, data, sizeof( data ), &length, 0 );

        if( length <= 0 ) { break; }
    } while( 1 );

    return true;
}

uint32_t ReadSerial( int serial, unsigned char *data, unsigned int length )
{
    uint32_t readLength = 0;
    ReadFile( serial, data, length, &readLength, 0 );
	return readLength;
}

uint32_t WriteSerial( int serial, const unsigned char *data, unsigned int length )
{
    uint32_t wroteLength = 0;
    WriteFile( serial, data, length, &wroteLength, 0 );
	return wroteLength;
}

void CloseSerial( int serial )
{
	CloseHandle( serial );
}
