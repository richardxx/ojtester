/*
 * To test the function separately.
 * And, it's a good example to teach you learn how to use this library, :>
 * By richardxx
 */

#include <stdio.h>
#include "libprocs.h"

int main( int argc, char** argv )
{
    if ( argc < 3 ) {
        printf( "Usage: %s prog_name input_file\n", argv[0] );
        return -1;
    }

    if ( run_program( argv[1], argv[2], "/dev/null", NULL,
                      NULL, NULL, NULL, NULL ) == RES_NORMAL ) {
        printf( "OK\n" );
    }
    else
        printf( "Sorry.\n" );

    return 0;
}
