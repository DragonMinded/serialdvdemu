#ifndef __DVDEMU_H
#define __DVDEMU_H

// The disc type is unknown
#define DVD_TYPE_UNKNOWN                0
// The DVD player is emulating a Victor XV-D701 VCD
#define DVD_TYPE_VICTOR_XV_D701_VCD     1
// The DVD player is emulating a Victor XV-D701 DVD
#define DVD_TYPE_VICTOR_XV_D701_DVD     2
// The DVD player is emulating a Toshiba SD-B100
#define DVD_TYPE_TOSHIBA_SD_B100_DVD    3

void verbose_printf( const char * const fmt, ... );
int exec_shell( const char * shellcmd );
void PrintHex( const char * prepend, const unsigned char * const data, int length );

#endif
