/*
 * Runtime operations.
 * Common interfaces to run testing programs.
 * By richardxx, 2009.1
 */

#ifndef RUNTIME_H
#define RUNTIME_H

#include "type_def.h"

#define INPUT_BY_GENERATOR	1
#define INPUT_BY_FOLDER		2

#define RESULT_BY_GENERATOR	1
#define RESULT_BY_FOLDER	2

#define CHECK_BY_COMPARISON	1
#define CHECK_BY_JUDGE		2

#define OOPS			100

/* Global Variables */
extern FILE* __flop;

typedef int (*FP_NEXT_INPUT)( struct sys_arg_t* );
typedef int (*FP_STD_RES)( struct sys_arg_t* );
typedef int (*FP_CHK_RES)( struct sys_arg_t*, char* );

/*
 * Funtion handlers to different testing mode:
 * get_next_input:      open a input file
 * get_standard_result: get result from this input file
 * check_result:        check the result
 */
extern FP_NEXT_INPUT get_next_input;
extern FP_STD_RES   get_standard_result;
extern FP_CHK_RES   check_result;

/*
 * Loaders
 */
extern void load_input( int );
extern void load_res_gen( int );
extern void load_checker( int );

/* Run user's program */
extern int run_user_program( int, const char*, struct sys_arg_t* );

#endif
