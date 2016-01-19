/*
 * This file is provided to record the running time of each function in the testing library, if they are compiled together.
 * It utilizes the internal functionalities offered by GCC.
 * These functions are automatically inserted by GCC, you cannot find callsites anywhere.
 * Specially notice the attributes attached to each function.
 *
 * By richardxx, 2009.4
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>

/*
 * Use a stack to manage the time consumming records.
 * Actually, all items are in reverse order.
 */
struct stack_item
{
    struct timeval t;
    struct stack_item *next;
};

static struct stack_item *stk_head = NULL;
static struct stack_item *cached_stk_head = NULL;
static FILE *__flog = NULL;

/*
 * Declarations (Must?)
 */
void __cyg_profile_func_enter( void*, void*)
__attribute__ ((no_instrument_function));

void __cyg_profile_func_exit( void* , void* )
__attribute__ ((no_instrument_function));

void main_constructor( void )
__attribute__ ((no_instrument_function, constructor));

void main_destructor( void )
__attribute__ ((no_instrument_function, destructor));



void __cyg_profile_func_enter( void *this, void *callsite )
{
    struct stack_item *p;

    if ( cached_stk_head != NULL ) {
        p = cached_stk_head;
        cached_stk_head = cached_stk_head -> next;
    }
    else {
        p = (struct stack_item*)malloc( sizeof( struct stack_item ) );
    }
    
    gettimeofday( &(p->t), NULL );
    fprintf( __flog, "E%p\n", (int*)this );
    
    p->next = stk_head;
    stk_head = p;
}

void __cyg_profile_func_exit( void *this, void *callsite )
{
    struct stack_item *p;
    struct timeval now;
    int res;
    
    p = stk_head;
    stk_head = stk_head -> next;

    gettimeofday( &now, NULL );
    res = (now.tv_sec - p->t.tv_sec) * 1000 + (now.tv_usec - p->t.tv_usec) / 1000;
    fprintf( __flog, "X%p %d\n", (int*)this, res );

    p -> next = cached_stk_head;
    cached_stk_head = p;
}

void main_constructor( void )
{
    __flog = fopen( "trace_tester.txt", "w" );
    if ( __flog == NULL ) {
#ifdef DEBUG
        fprintf( stderr, "Profile program failed, caused by:\n" );
        perror( NULL );
#endif
        exit(-1);
    }
}

void main_destructor( void )
{
    if ( __flog != NULL )
        fclose( __flog );
}
