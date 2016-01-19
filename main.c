/*
  Software running environment setup interface.
  By Richard Leon,2007.8
  Ver 0.2 alpha by richardxx, 2008.8
  Ver 0.2 final by richardxx, 2009.1
  Ver 0.3 alpha by richardxx, 2009.5, code refactoring and adding more debug information
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "consts.h"
#include "type_def.h"
#include "judge.h"
#include "file.h"
#include "libsys.h"
#include "runtime.h"

#define DEFAULT_RUNS		10
#define DEFAULT_WAIT_TIME	10000
#define DEFAULT_MEMORY_SIZE  ( ~(1 << (sizeof(int) * 8 - 1) ) >> 10 )


#define SET_PROG_ARG( PROG_NAME, ARG ) \
do { \
    if ( optarg[0] == '/' ) \
         strcpy( sysinfo.PROG_NAME, optarg ); \
    else { \
         strcpy( sysinfo.PROG_NAME, "./" ); \
         strcpy( sysinfo.PROG_NAME + 2, optarg ); \
    } \
} while (0) \


#define LOAD_DIRECTORY( PROG_NAME, ARG ) \
do { \
    sysinfo.PROG_NAME = open_folder( optarg ); \
    if ( sysinfo.PROG_NAME == NULL ) { \
		fprintf( stderr, "Load folder \"%s\" failed\n", optarg ); \
        return 0; \
    } \
} while (0) \

#define COMPILE_SOURCE_CODE( SOURCE_NAME ) \
do { \
	 if ( !is_binary_file( (SOURCE_NAME) ) && \
          !compile( (SOURCE_NAME) ) ) { \
         fprintf( stderr, "Compile %s error.\n", SOURCE_NAME ); \
         return 0; \
     } \
} while(0) \

// Variables
static struct sys_arg_t sysinfo;

// Global information
int Verbose_mode;

static void release()
{
    if ( sysinfo.di_in != NULL ) close_folder( sysinfo.di_in );
    if ( sysinfo.di_out != NULL ) close_folder( sysinfo.di_out );
    if ( sysinfo.di_temp != NULL ) {
        // Delete temporary directory
        if ( !remove_folder( sysinfo.di_temp -> folder_name ) ) {
            fprintf( stderr, "Please remove the temporary folder manually.\n" );
        }
        
        close_folder( sysinfo.di_temp );
    }

    if ( sysinfo.sp_inout != NULL ) close_pattern( sysinfo.sp_inout );
    
    free2d( (char**)sysinfo.resp, sysinfo.num_of_progs );
}

static void print_help(const char* pname)
{
    printf( "Usage: %s [OPTION...] [PROGRAMS...]\n", pname );
    printf( "%s aims to ease your testing process.\n", pname );
    putchar( '\n' );
    printf( "Examples:\n" );
    printf( "%s -g data_gen.c prog1.c prog2.c\n", pname );
    printf( "%s -I input -O output prog1.c prog2.c\n", pname );
    printf( "%s -g data_gen.c -s2 prog1.c prog2.c\n", pname );
    putchar( '\n' );
    printf( "Options:\n" );
    printf( "-c=[NUMBER], number of testing runs\n" );
    printf( "-s=[NUMBER], specify index of the standard program in the program list ( default is 1, prog1.c in our examples )\n" );
    printf( "-g=[STRING], specify the data generator\n" );
    printf( "-I=[STRING], specify the input data folder\n" );
    printf( "-O=[STRING], specify the output data folder\n" );
    printf( "-j=[STRING], specify the special judge program\n" );
    printf( "-D=[STRING], keep all intermediate data in a folder with specified name\n" );
    printf( "-v, display  show verbose information\n" );
    printf( "-T=[NUMBER], time resource limit, measured in millionsecond\n" );
    printf( "-M=[NUMBER], memory resource limit, measured in KB\n" );
    printf( "-h, print this help\n" );
    putchar( '\n' );

    printf( "Version: %s\n", VERSION );
}

static void init_options()
{
    sysinfo.runs = DEFAULT_RUNS;
    sysinfo.passed_cases = 0;
    sysinfo.res_cons.time_limit = DEFAULT_WAIT_TIME;
    sysinfo.res_cons.mem_limit = DEFAULT_MEMORY_SIZE;
    sysinfo.num_of_progs = 0;
    sysinfo.std_inx = 0;
    sysinfo.progs = NULL;
    sysinfo.gen_prog[0] = sysinfo.checker_prog[0] = 0;
    sysinfo.input_file[0] = sysinfo.output_file[0] = sysinfo.dump_dir[0] = 0;
    sysinfo.di_in = sysinfo.di_out = sysinfo.di_temp = NULL;
    sysinfo.sp_inout = NULL;
    sysinfo.resp = NULL;
}

/*
 * 1. Guess the testing method;
 * 2. Validate the arguments.
 */
static int guess_intention()
{
    int i;
    char buf[128];

    // Request needed resources
    if ( ( sysinfo.di_temp = open_folder( buf ) ) == NULL ) {
        fprintf( stderr, "Create temporary data failed.\n" );
        return 0;
    }   

    sysinfo.resp = ( struct RESUSE** )malloc2d( sysinfo.num_of_progs,
                                                sizeof( struct RESUSE ) );

    if ( sysinfo.resp == NULL ) {
        close_folder( sysinfo.di_temp );
        return 0;
    }
    
    // Check and compile all candidate programs
    for ( i = 0; i < sysinfo.num_of_progs; ++i )
        COMPILE_SOURCE_CODE( sysinfo.progs[i] );

    // Guess testing mode
    if ( sysinfo.checker_prog[0] ) {
        COMPILE_SOURCE_CODE( sysinfo.checker_prog );
        load_res_gen( OOPS );
        load_checker( CHECK_BY_JUDGE );
    }
    else {
        load_checker( CHECK_BY_COMPARISON );
    }
    
    if ( file_exist( sysinfo.gen_prog ) ) {
        // Get data from generator
        if ( !( sysinfo.std_inx >= 0 &&
                sysinfo.std_inx < sysinfo.num_of_progs ) ) {
            fprintf( stderr, "Warning: Standard program index is out of range.\n" );
            fprintf( stderr, "Change back to 0.\n" );
            sysinfo.std_inx = 0;
        }

        COMPILE_SOURCE_CODE( sysinfo.gen_prog );
        
        load_input( INPUT_BY_GENERATOR );
        load_res_gen( RESULT_BY_GENERATOR );
    }
    else if ( sysinfo.di_in != NULL ) {
        // Get data from predefined directory
        load_input( INPUT_BY_FOLDER );
        
        if ( sysinfo.di_out != NULL ) {
            sysinfo.sp_inout = detect_pattern( sysinfo.di_in -> folder_name,
                                               sysinfo.di_out -> folder_name );
            if ( sysinfo.sp_inout == NULL ) {
                fprintf( stderr, "There's no unique mapping pattern between input and output file.\n" );
                fprintf( stderr, "I suggest you should check it before runing test again.\n" );
                return 0;
            }

            load_res_gen( RESULT_BY_FOLDER );
        }
        else
            load_res_gen( RESULT_BY_GENERATOR );
    }
    else
        return 0;

    // Create temporary directory
    buf[127] = 0;
    sprintf( buf, "%d_%d", getpid(), time(NULL) );
    if ( buf[127] != 0 ) {
        // A serious security problem, please report
        fprintf( stderr, "A series problem, please send me following string:\n" );
        fprintf( stderr, "%d_%d\n", getpid(), time(NULL) );
        return 0;
    }
    
    return 1;
}

static int parse_arguments( int argc, char **argv )
{
    int i, c;

    Verbose_mode = 0;
    
    while ( ( c = getopt( argc, argv, 
                          "ac:s:g:I:O:j:D:vT:M:h" ) ) != -1 ) {
    
        switch ( c ) {
            case 'c':
                sysinfo.runs = atoi( optarg );
                if ( sysinfo.runs <= 0 ) sysinfo.runs = DEFAULT_RUNS;
                break;
      
            case 's':
                sysinfo.std_inx = atoi( optarg );
                break;

            case 'g':
                SET_PROG_ARG( gen_prog, "-g" );
                break;

            case 'I':
                LOAD_DIRECTORY( di_in, "-I" );
                break;
      
            case 'O':
                LOAD_DIRECTORY( di_out, "-O" );
                break;

            case 'j':
                SET_PROG_ARG( checker_prog, "-j" );
                break;
      
            case 'D':
                strcpy( sysinfo.dump_dir, optarg );
                break;

            case 'v':
                Verbose_mode = 1;
                break;

            case 'T':
                sysinfo.res_cons.time_limit = atoi( optarg );
                break;

            case 'M':
                sysinfo.res_cons.mem_limit = atoi( optarg );
                break;
                
            case 'h':
                print_help( argv[0] );
                exit( 0 );

            case '?':
                printf( "Invalid option: %s\n", optarg );
                exit( -1 );
        }
    }

    // Copy left arguments
    sysinfo.num_of_progs = argc - optind;
    if ( sysinfo.num_of_progs == 0 ) {
        printf( "No program specified. Abort.\n" );
        return 0;
    }
    
    sysinfo.progs = (char**)malloc2d( sysinfo.num_of_progs, FILE_NAME_LEN );
    if ( sysinfo.progs == NULL ) {
#ifdef DEBUG
        fprintf( stderr, "Out of memory: %s(%d)\n",
                 __FILE__, __LINE__ );
#endif
        return 0;
    }
    
    for ( i = 0; i < sysinfo.num_of_progs; ++i ) {
        sprintf( sysinfo.progs[i],
                 argv[optind+i][0] == '/' ? "%s" : "./%s",
                 argv[optind+i] );
    }
    
    return 1;
}

static int load_arguments_from_file( const char* file, char **args )
{
    int i;
    FILE *fp;

    fp = fopen( file, "r" );
    if ( fp == NULL ) return 0;

    for ( i = 0;
          fscanf( fp, "%s", args[i] ) == 1;
          ++i );

    fclose( fp );
    return i;
}

static void save_arguments_to_file( const char* file,
                                    int argc, char **args )
{
    int i;
    FILE *fp;

    fp = fopen( file, "w" );
    if ( fp == NULL ) return;

    for ( i = 0; i < argc; ++i ) fprintf( fp, "%s ", args[i] );
    fputc( '\n', fp );

    fclose( fp );
}

static int read_options_from_file( const char* run_name )
{
    int i, argc;
    char **argv;
    
    // Request memory for arguments
    argv = malloc2d( ARGUMENTS_NUM, OPTION_LEN );
    if ( argv == NULL ) {
#ifdef DEBUG
        fprintf( stderr, "Out of memory: %s(%d)\n",
                 __FILE__, __LINE__ );
#endif
        return 0;
    }

    // Make consistency
    argc = load_arguments_from_file( TESTER_RC, argv+1 ) + 1;
    strcpy( argv[0], run_name );

    // Initialize
    if ( !parse_arguments( argc, argv ) || !guess_intention() ) {
        // Actually, this code is not reachable
        free2d( argv, ARGUMENTS_NUM );
        return 0;
    }

    // Tell user what we have done
    printf( "\nUsing arguments:\n" );
    for ( i = 0; i < argc; ++i ) printf( i == 0 ? "%s" : " %s", argv[i] );
    printf( "\n\n" );
        
    free2d( argv, ARGUMENTS_NUM );

    return 1;
}

static void sig_handler( int sigid )
{
    // Clear temporary data
    release();
    
    // Terminate all child processes
    if ( sigid == SIGINT ||
         sigid == SIGTERM ) kill( 0, SIGKILL );
}

/*
 * Process:
 * 1. Handle arguments, use last successful one while this is broken;
 * 2. Set up running environment;
 * 3. Run.
 */
int main( int argc, char** argv )
{
    int i;
    
    init_options();

    if ( argc == 1 ) {
        if ( !read_options_from_file( argv[0] ) )
            return -1;
    }
    else {
        if ( !parse_arguments( argc, argv ) || !guess_intention() ) {
            fprintf( stderr, "You can also use -h to see help if you wish.\n" );
            return -1;
        }
        
        save_arguments_to_file( TESTER_RC, argc - 1, argv + 1 );
    }

    // Install signal handlers
    if ( signal( SIGINT, sig_handler ) == SIG_ERR ) return -1;
    if ( signal( SIGTERM, sig_handler ) == SIG_ERR ) return -1;
    if ( signal( SIGSEGV, sig_handler ) == SIG_ERR ) return -1;
    
    // Run supervised judge
    judge( &sysinfo );
    release();
    
    return 0;
}
