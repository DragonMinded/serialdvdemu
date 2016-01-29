#ifndef __Victor_XV_D701_H
#define __Victor_XV_D701_H

#define VICTOR_DVD 0
#define VICTOR_VCD 1

void VictorInit( int type );
int VictorReceiveByte( unsigned char byte );
unsigned char *VictorGetResponse( int *length );

#endif
