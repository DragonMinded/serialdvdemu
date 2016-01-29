#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "DVDEmu.h"
#include "Serial.h"
#include "Video.h"
#include "Victor_XV-D701.h"
#include "Toshiba_SD-B100.h"

#define VERBOSE_DEBUG         1

int getDirectoryCount( const char * const directory )
{
    int file_count = 0;
    DIR * dirp;
    struct dirent * entry;

    dirp = opendir(directory);

    while ((entry = readdir(dirp)) != NULL)
    {
        if (entry->d_type == DT_REG)
        {
            file_count++;
        }
    }

    closedir(dirp);

    return file_count;
}

int getDvdType( const char * const directory )
{
    char file[256];
    FILE *fp;
    int type = DVD_TYPE_UNKNOWN;

    sprintf(file, "%sdisc.cfg", directory);

    fp = fopen( file, "r" );

    if( fp )
    {
        char value[128];
        fscanf(fp, "%s", value);

        if( strcmp( value, "XV-D701-VCD" ) == 0 )
        {
            type = DVD_TYPE_VICTOR_XV_D701_VCD;
        }
        else if( strcmp( value, "XV-D701-DVD" ) == 0 )
        {
            type = DVD_TYPE_VICTOR_XV_D701_DVD;
        }
        else if( strcmp( value, "SD-B100-DVD" ) == 0 )
        {
            type = DVD_TYPE_TOSHIBA_SD_B100_DVD;
        }

        fclose( fp );
    }

    return type;
}

void verbose_printf( const char * const fmt, ... )
{
    if( VERBOSE_DEBUG )
    {
        va_list args;
        va_start(args, fmt);
        vprintf(fmt, args);
        va_end(args);
    }
}

void exec_shell( const char * shellcmd )
{
    if( fork() == 0 )
    {
        execl("/bin/bash","/bin/bash","-c",shellcmd,NULL);
    }
    else
    {
        int status;
        wait(&status);
    }
}

void PrintHex( const char * prepend, const unsigned char * const data, int length )
{
    if( prepend )
    {
        printf( "%s", prepend );
    }

    for( int x = 0; x < length; x++ )
    {
        printf( "%02X ", data[x] );
    }

    printf( " (Length: %d bytes)\n", length );
}

void PrintInstructions( char * name )
{
    fprintf( stderr, "\n" );
    fprintf( stderr, "%s port folder\n", name );
    fprintf( stderr, "\tport - a com port, such as COM1.\n" );
    fprintf( stderr, "\tfolder - a folder of M4Vs, such as /home/root/dvd/. Must end in \"/\".\n" );
}

int main( int argc, char *argv[] )
{
    /* Ensure command is good */
    if( argc < 2 )
    {
        fprintf( stderr, "Missing serial port argument!\n" );
        PrintInstructions( argv[0] );
        return 1;
    }

    if( argc < 3 )
    {
        fprintf( stderr, "Missing m4v folder!\n" );
        PrintInstructions( argv[0] );
        return 1;
    }

    /* Attempt to ascertain the type */
    int dvd_type = getDvdType( argv[2] );

    if( dvd_type == DVD_TYPE_UNKNOWN )
    {
        fprintf( stderr, "Couldn't get DVD information. Be sure your DVD directory has a disc.cfg specifying the type!\n" );
        return 1;
    }

    /* Set up DVD player serial */
    int serial;
    if( dvd_type == DVD_TYPE_VICTOR_XV_D701_VCD || dvd_type == DVD_TYPE_VICTOR_XV_D701_DVD )
    {
        serial = OpenSerial( argv[1], 9600, PARITY_EVEN );
    }
    else if( dvd_type == DVD_TYPE_TOSHIBA_SD_B100_DVD )
    {
        serial = OpenSerial( argv[1], 9600, PARITY_NONE );
    }
    else
    {
        fprintf( stderr, "Unknown DVD type, can't init!\n" );
        return 1;
    }

    if( serial < 0 )
    {
        fprintf( stderr, "Failed to open serial port '%s'!\n", argv[1] );
        return 1;
    }

    /* Ensure we start fresh */
    PurgeComm( serial );
    
    /* Set up defaults, making sure not to include the type file */
    VideoInit( argv[2], getDirectoryCount(argv[2]) - 1 );

    /* Set up DVD emulator */
    if( dvd_type == DVD_TYPE_VICTOR_XV_D701_VCD )
    {
        VictorInit( VICTOR_VCD );
    }
    else if( dvd_type == DVD_TYPE_VICTOR_XV_D701_DVD )
    {
        VictorInit( VICTOR_DVD );
    }
    else if( dvd_type == DVD_TYPE_TOSHIBA_SD_B100_DVD )
    {
        ToshibaInit();
    }

    while( 1 )
    {
        /* Since JLIP has a start of message and a checksum, as well as the
         * start of message can't be a mistake given that the top bit is set
         * and no command or argument uses the top bit, we can simply feed
         * every byte we get into a small buffer and attempt to handle it as
         * a packet.
         */
        unsigned char byte;

        if( ReadSerial( serial, &byte, 1 ) == 1 )
        {
            /* Got one */
            int response;
            if( dvd_type == DVD_TYPE_VICTOR_XV_D701_VCD || dvd_type == DVD_TYPE_VICTOR_XV_D701_DVD )
            {
                response = VictorReceiveByte( byte );
            }
            else if( dvd_type == DVD_TYPE_TOSHIBA_SD_B100_DVD )
            {
                response = ToshibaReceiveByte( byte );
            }

            if( response )
            {
                int length = 0;
                unsigned char *packet = NULL;

                if( dvd_type == DVD_TYPE_VICTOR_XV_D701_VCD || dvd_type == DVD_TYPE_VICTOR_XV_D701_DVD )
                {
                    packet = VictorGetResponse( &length );
                }
                else if( dvd_type == DVD_TYPE_TOSHIBA_SD_B100_DVD )
                {
                    packet = ToshibaGetResponse( &length );
                }

                if( packet && length > 0 )
                {
                    WriteSerial( serial, packet, length );
                }
            }
        }
    }
    
    /* Close the connection */
    CloseSerial( serial );

    return 0;
}
