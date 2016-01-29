#ifndef __SERIAL_H
#define __SERIAL_H

#define PARITY_EVEN 1
#define PARITY_ODD 2
#define PARITY_NONE 3

int OpenSerial( const char *port, int baud, int parity );
uint32_t ReadSerial( int serial, unsigned char *data, unsigned int length );
uint32_t WriteSerial( int serial, const unsigned char *data, unsigned int length );
void CloseSerial( int serial );
bool PurgeComm( int serial );

#endif
