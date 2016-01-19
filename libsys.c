/*
 * Some auxiliary functions go here, including memory manipulation and compiling wrapper.
 * By richardxx, 2009.1
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include "libsys.h"
#include "libprocs.h"

#define TRY_TIME				5
#define SUPPORT_SOURCE_NUM		6
#define GCC		1
#define GPP		2
#define JAVA	3
#define PASCAL	4

extern int Verbose_mode;

static char* suffix[] = { "c", "cc", "cpp",
                          "C", "cxx", "java", "pas" };

static int compiler[] = { GCC, GPP, GPP, GPP, GPP, JAVA, PASCAL };


// Remove a suffix separated by "."
// Suffix should be listed in Arg2.
// Return -1 if no recognized suffix exist.
static int get_rid_of_suffix( char* pstr )
{
    int i, j, len;

    len = strlen( pstr );
    for ( i = len - 1; i > -1 && pstr[i] != '/'; --i )
        if ( pstr[i] == '.' ) break;

    if ( !(i > -1 && pstr[i] == '.' ) ) return -1;
    pstr[i] = 0;
    
    for ( j = 0; j < SUPPORT_SOURCE_NUM; ++j )
        if ( strcmp( pstr + i + 1, suffix[j] ) == 0 ) return j;
    
    return -1;
}


// Batch request memory
int malloc_all_var( int size, ... )
{
    int i;
    va_list args1, args2;
    char **ptr1, **ptr2;

    i = 0;
    va_start( args1, size );
    while ( ptr1 = va_arg( args1, char** ) ) {
        *ptr1 = (char*)malloc( size );
        if ( *ptr1 == NULL ) {
            // Withdraw all the allocated memories
            va_start( args2, size );
            while ( ( ptr2 = va_arg( args2, char** ) ) != ptr1 ) {
                free( *ptr2 );
            }
            va_end( args2 );
            return 0;
        }
    }
    
    va_end( args1 );
    return 1;
}
            
void free_all_var( char *p, ... )
{
    va_list args;

    if ( p != NULL ) free( p );
    va_start( args, p );
    
    while ( (p = va_arg( args, char* )) )
        if ( p != NULL ) free( p );
}

int malloc_all_array( int num, int size, char** p )
{
    int i;

    for ( i = 0; i < num; ++i ) {
        p[i] = (char*)malloc( size );
        if ( p[i] == NULL ) {
            while ( --i > -1 ) free( p[i] );
            return 0;
        }
    }

    return 1;
}

void free_all_array( int num, char** p )
{
    int i;

    if ( p == NULL ) return;
    for ( i = 0; i < num; ++i ) {
        if ( p[i] == NULL ) continue;
        free( p[i] );
    }
}

char** malloc2d( int sz1, int sz2 )
{
    char** p;
    int i;

    /*
     * Note that please allocate sz1 * sizeof( void* ) spaces.
     */
    p = (char**)malloc( sz1 * sizeof(void*) );
    if ( p == NULL ) return NULL;
    
    for ( i = 0; i < sz1; ++i ) {
        p[i] = (char*)malloc( sz2 );
        if ( p[i] == NULL ) {
            while ( --i > -1 ) free( p[i] );
            free( p );
            return NULL;
        }
    }

    return p;
}

void free2d( char** p, int sz1 )
{
    int i;

    if ( p == NULL ) return;
    
    for ( i = 0; i < sz1; ++i ) {
        if ( p[i] == NULL ) continue;
        free( p[i] );
        p[i] = NULL;
    }
    
    free( p );
}

/*
 * Note: All source file need follow by a suffix such as .c
 * Once the program is compiled, the suffix is truncted.
 * Call g++ compiler to compile a source program.
 */
int compile( char* psrc )
{
    char *pbin = NULL, *argv[5];
    int i;
    int ret, tp_inx;
    
    // Request resource
    pbin = strdup( psrc );
    
    if ( ( tp_inx = get_rid_of_suffix( pbin ) ) == -1 ) {
        // Unsupported source type
        free( pbin );
        return 0;
    }
    
    // Select compiler
    switch ( compiler[tp_inx] ) {
        case GCC:
            argv[0] = "gcc";
            argv[1] = psrc;
            argv[2] = "-o";
            argv[3] = pbin;
            argv[4] = NULL;
            break;

        case GPP:
            argv[0] = "g++";
            argv[1] = psrc;
            argv[2] = "-o";
            argv[3] = pbin;
            argv[4] = NULL;
            break;

        case JAVA:
            argv[0] = "javac";
            argv[1] = psrc;
            argv[2] = NULL;
            break;

        case PASCAL:
            argv[0] = "fpc";
            argv[1] = psrc;
            argv[2] = NULL;
            break;
    }

    printf( "Compile source files:\n" );
    
    if ( Verbose_mode ) {
        printf( "Compile %s .......", psrc );
        fflush( stdout );
    }

    // Try compiling program
    for ( i = 0; i < TRY_TIME; ++i ) {
        if ( run_program( argv[0],
                          NULL, "/dev/null", "/dev/null",
                          NULL, NULL, &ret, argv ) == RES_NORMAL ) {
            
            if ( ret == 0 ) break;
        }
        
        sleep( 1 );
        
        if ( Verbose_mode ) {
            printf( ".." );
            fflush( stdout );
        }
    }
    
    if ( i < TRY_TIME ) strcpy( psrc, pbin );
    if ( Verbose_mode ) {
        printf( i < TRY_TIME ? " OK\n" : " Failed\n" );
        fflush( stdout );
    }
    
    free_all_var( pbin, NULL );
    return 1;
}

