#ifndef __TOSHIBA_SD_B100
#define __TOSHIBA_SD_B100

void ToshibaInit();
int ToshibaReceiveByte( unsigned char byte );
unsigned char *ToshibaGetResponse( int *length );

#endif
