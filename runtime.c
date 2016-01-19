/*
 * Handing the work of running testing programs, and gathering testing result.
 * By richardxx, 2009.2
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "consts.h"
#include "libprocs.h"
#include "runtime.h"

#define TRY_TIME		5
#define DEFAULT_INPUT_NAME	"input_data.txt"
#define DEFAULT_OUTPUT_NAME "output_data.txt"


static char tmp_str1[ FILE_NAME_LEN + 1 ];
static char tmp_str2[ FILE_NAME_LEN + 1 ];
static char pcmd[ FILE_NAME_LEN * 3 + 256 ];
static int answer_from_user_program;

// Global
FP_NEXT_INPUT get_next_input = NULL;
FP_STD_RES    get_standard_result = NULL;
FP_CHK_RES    check_result = NULL;

/* Info dumping files.
   Enable them throughout the controlling macros.
*/
#if defined(DEBUG) || defined(PROFILE)
	FILE* __flog = NULL;
#endif


static int
get_input_from_folder( struct sys_arg_t* parg )
{
    // Read a file
    if ( !get_next_file( parg -> di_in, parg -> input_file ) )
        return 0;
    
    // Create a link from tmp_str2 to tmp_str1
    sprintf( tmp_str1,
             parg -> input_file[0] == '/' ? "%s" : "../%s",
             parg -> input_file );

    sprintf( tmp_str2, "%s/%s",
             parg -> di_temp -> folder_name, DEFAULT_INPUT_NAME );

    unlink( tmp_str2 );
    
    if ( symlink( tmp_str1, tmp_str2 ) == -1 &&
         link( tmp_str1, tmp_str2 ) == -1 &&
         !file_copy( tmp_str1, tmp_str2 ) ) {
#ifdef DEBUG
                fprintf( stderr, "Link to %s failed.\n",
                         parg -> input_file );
#endif
                return 0;
    }
    
    ++parg -> passed_cases;
    return 1;
}

static int
get_input_from_generator( struct sys_arg_t* parg )
{
    char* argv[2];
    int ret;
    
    if ( parg -> runs-- <= 0 ) return 0;
    
    sprintf( parg -> input_file, "%s/%s",
             parg -> di_temp -> folder_name, DEFAULT_INPUT_NAME );

    argv[0] = parg -> gen_prog;
    argv[1] = NULL;

    if ( run_program( argv[0], NULL, parg -> input_file, "/dev/null",
                      NULL, NULL, &ret, argv ) == RES_NORMAL ) {
        ++parg -> passed_cases;
        return 1;
    }

    return 0;
}

static int
get_result_from_folder( struct sys_arg_t* parg )
{
    /*
     * The output file name is generated from observed suffix mapping pattern.
     * Then we look up if this particular file exists.
     */
    if ( !map_file( parg -> sp_inout, parg -> di_out -> folder_name,
                    parg -> input_file, parg -> output_file ) ) return 0;

    // Create a symbolic link from tmp_str2 to tmp_str1
    sprintf( tmp_str1,
             parg -> output_file[0] == '/' ? "%s" : "../%s",
             parg -> output_file );

    sprintf( tmp_str2, "%s/%s",
             parg -> di_temp -> folder_name, DEFAULT_OUTPUT_NAME );
    
    unlink( tmp_str2 );
    
    if ( symlink( tmp_str1, tmp_str2 ) == -1 &&
         link( tmp_str1, tmp_str2 ) == -1 &&
         !file_copy( tmp_str1, tmp_str2 ) ) {
#ifdef DEBUG
        fprintf( stderr, "Link to %s failed.\n",
                 parg -> input_file );
#endif
        return 0;
    }
    
    return 1;
}

static int
get_result_from_specified_program( struct sys_arg_t* parg )
{
    char *argv[2];
    int ret;
    
    sprintf( parg -> output_file, "%s/%s",
             parg -> di_temp -> folder_name, DEFAULT_OUTPUT_NAME );

    argv[0] = parg -> progs[ parg -> std_inx ];
    argv[1] = NULL;

    if ( run_program( argv[0],  parg -> input_file,
                      parg -> output_file, NULL,
                      &(parg -> res_cons), parg -> resp[ parg -> std_inx],
                      &ret, argv ) == RES_NORMAL )
        ret = 1;
    else
        ret = 0;

    
    return ret;
}

/*
 * By Linux command 'diff'
 */
static int
check_result_by_comparison( struct sys_arg_t* parg,
                            char* output )
{
    char *argv[8], prog_name[16];
    int ret, res;

    
    argv[0] = "diff";
    argv[1] = parg -> output_file;
    argv[2] = output;
    argv[3] = NULL;

    if ( run_program( argv[0], NULL, "/dev/null", "/dev/null",
                      NULL, NULL, &ret, argv ) != RES_NORMAL ) {
        res = RES_VE;
    }
    else {
        if ( ret == 0 ) res = RES_AC;
        else {
            // now check presentation error
            argv[1] = "-i";
            argv[2] = "-b";
            argv[3] = "-w";
            argv[4] = "-B";
            argv[5] = parg -> output_file;
            argv[6] = output;
            argv[7] = NULL;

            if ( run_program( argv[0], NULL, "/dev/null", "/dev/null",
                          NULL, NULL, &ret, argv ) != RES_NORMAL ) {
                res = RES_VE;
            }
            else if ( ret == 0 )
                res = RES_PE;
            else
                res = RES_WA;
        }
    }
    
    return res;
}

/*
 * Send request to special judge program.
 */
static
int check_result_by_checker( struct sys_arg_t* parg,
                             char* output )
{
    char *argv[4];
    int ret, status;
    
    argv[0] = parg -> checker_prog;
    argv[1] = parg -> input_file;
    argv[2] = output;
    argv[3] = NULL;

    if ( ( status =
           run_program( argv[0], NULL, "/dev/null", "/dev/null",
                        NULL, NULL, &ret, argv ) ) != RES_NORMAL )
        return RES_VE;
    
    return status;
}

static int nop( struct sys_arg_t* parg )
{
    return 1;
}

/*
 * Public Interface
 */


void load_input( int mode )
{
    get_next_input = ( mode == INPUT_BY_GENERATOR ?
                       get_input_from_generator :
                       get_input_from_folder );
}

void load_res_gen( int mode )
{
    if ( mode == RESULT_BY_GENERATOR ) {
        get_standard_result = get_result_from_specified_program;
        answer_from_user_program = 1;
    }
    else {
        get_standard_result = ( mode == OOPS ? nop :
                                get_result_from_folder );
        answer_from_user_program = 0;
    }
}

void load_checker( int mode )
{
    check_result = ( mode == CHECK_BY_COMPARISON ?
                     check_result_by_comparison :
                     check_result_by_checker );
}

/*
 * A simple delegate
 * To avoid produce the same output twice
 */
int run_user_program( int inx, const char* output, struct sys_arg_t* parg )
{
    int ret;
    
    if ( answer_from_user_program &&
         inx == parg -> std_inx ) {

        /*
         * Why copy?
         * Since finally this temporary directory maybe renamed.
         */
        if ( !file_copy( parg -> output_file, output ) ) {
#ifdef DEBUG
            fprintf( stderr, "Produce %s failed.\n", output );
#endif
            ret = RES_SE;
        }

        ret = RES_NORMAL;
    }
    else {
        ret = run_program( parg -> progs[inx], parg -> input_file,
                           output, NULL,
                           &(parg -> res_cons), parg -> resp[inx],
                           NULL, NULL );
    }
    
    return ret;
}
