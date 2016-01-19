/*
  The core part of the judging system.
  By Richard Leon,2007
  Modified by richardxx, 2008-2009
*/

#ifndef JUDGE_H
#define JUDGE_H

#include "type_def.h"

/*
 * The main judge loop.
 * Arg1 is the judge status structure.
 * Return 0 if unexpected errors occurred, otherwise 1.
 */
extern
int judge( struct sys_arg_t* );

#endif
