/*
 * This file contains some system related functions.
 * By richardxx, 2009.1
 */

#ifndef LIBSYS_H
#define LIBSYS_H

struct RESCONS;
struct RESUSE;

/*
 * Request memory for all the arguments passed in.
 * Either of them fails leading to failure.
 * Arg1: size of blocks of each pointer points to
 */
int malloc_all_var( int, ... );
void free_all_var( char*, ... );

/*
 * These two are the same as above, but use array instead.
 * Arg1: how many pointers need to be allocated
 * Arg2: size of blocks of each pointer points to
 */
int malloc_all_array( int, int, char** );
void free_all_array( int, char** );

// Request two dimensional spaces
char** malloc2d( int, int );
void free2d( char**, int );

/*
 * Call relative compiler to compile program.
 * The compiler is chose by suffix matching.
 * Generate a binary file with the suffix removed.
 * Return 0 if compiling fails, otherwise return 1.
 */
extern int compile( char* );

#endif
