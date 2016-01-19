/*
  The driver of this testing tool.
  Since our aim is to provide a testing tool library which is easily integrated in any projects, so we try best to design all components loosely coupled.
  2007.8: Write the basic functionalities
  2008.8 - 2008.9: Totally rewrite all code
  2009.1: Refactoring
  By richardxx
*/

#include <stdio.h>
#include "file.h"
#include "libprocs.h"
#include "runtime.h"
#include "judge.h"

/*
 * Print formatted result.
 */
static void
print_result( int id, struct RESUSE *resp, int res_type )
{
  int mem, use_time;

  // How many resources did the two programs use?
  mem = use_time = 0;
    
  if ( resp != NULL ) {
    use_time = time_used( resp );
    mem = mem_used( resp );
  }
    
  printf( "Prog %5d: Result=%25s, Time = %7ums, Memory = %7uKB\n",
	  id, pres_text[ res_type ], use_time, mem );
}

/*
 * Information about all cases.
 */
static void
print_summary( struct RESUSE* resp, int runs )
{
  int mem, use_time;

  // How many resources did the two programs use?
  mem = use_time = 0;
    
  if ( resp != NULL ) {
    use_time = time_used( resp );
    mem = mem_used( resp );
  }

  printf( "Tot. time = %8ums, Ave. time = %7ums, Ave. Memory = %7uKB\n",
	  use_time, use_time / runs, mem / runs );
}


/*
 * The judge main process.
 */
int
judge( struct sys_arg_t* parg )
{
  int i, case_no;
  int ret, status, abnormal;
  char* tmp_buf = NULL;
  struct RESUSE** total_resp = NULL;
    
  // Prepare
  if ( !malloc_all_var( FILE_NAME_LEN+128, &tmp_buf, NULL ) ||
       ( total_resp = ( struct RESUSE**)malloc2d(
						 parg -> num_of_progs,
						 sizeof( struct RESUSE ) ) ) == NULL ) {
#ifdef DEBUG
    fprintf( stderr, "Out of memory: %s(%d)\n",
	     __FILE__, __LINE__ );
#endif
    return 0;
  }
    
  case_no = 1;
  abnormal = 0;
  for ( i = 0; i < parg -> num_of_progs; ++i )
    resuse_start( total_resp[i] );

  // Main loop
  // Note, the standard output produces twice 
  while ( !abnormal &&
	  get_next_input( parg ) ) {

    // New test
    printf( "Test %d:\n", case_no++ );
        
    // Get correct output for this test 
    if ( !get_standard_result( parg ) ) {
      printf( "Get standard answer error, terminated.\n" );
      abnormal = 1;
      break;
    }

    /*
     * For each program listed in command line prompt,
     * generate its output and judge its correctness.
     */
    for ( i = 0; i < parg -> num_of_progs; ++i ) {
      sprintf( tmp_buf, "%s/prog%d_output.txt",
	       parg -> di_temp -> folder_name, i );

      if ( ( ret =
	     run_user_program( i, tmp_buf, parg ) ) == RES_NORMAL ) {
                
	ret = check_result( parg, tmp_buf );
      }

      print_result( i, parg -> resp[i], ret );
            
      resuse_add( total_resp[i], parg -> resp[i] );
            
      if ( ret != RES_AC ) abnormal = 1;
    }

    putchar( '\n' );
  }

  // Print summary
  printf( "Summary:\n" );
  for ( i = 0; i < parg -> num_of_progs; ++i ) {
    print_summary( total_resp[i], parg -> passed_cases );
  }

  // Copy data
  if ( abnormal && parg -> dump_dir[0] ) {
    // Move temporary data to destination
    rename_folder( parg -> di_temp -> folder_name, parg -> dump_dir );
    close_folder( parg -> di_temp );
    parg -> di_temp = NULL;
  }
    
  free_all_var( tmp_buf, NULL );
  free2d( (char**)total_resp, parg -> num_of_progs );
    
  return 1;
}
