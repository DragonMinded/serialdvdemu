#include <stdio.h>
#include <stdint.h>
#include "Toshiba_SD-B100.h"

void ToshibaInit()
{
    /* TODO: Init packet buffer and response buffer */
}

int ToshibaReceiveByte( unsigned char byte )
{
    /* TODO: Read and parse bytes, return 1 if there is a response to send */
    return 0;
}

unsigned char *ToshibaGetResponse( int *length )
{
    /* TODO: Return response buffer */
    *length = 0;
    return NULL;
}
