/* Modified from resuse.c - child process resource use library
 * Written by David MacKenzie, with help from
 * arnej@imf.unit.no (Arne Henrik Juul)
 * and pinard@iro.umontreal.ca (Francois Pinard).
 *
 * richardxx, 2009.2, reorganize the code to better serve our requirements
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include "libprocs.h"

# ifndef HZ
#  include <sys/param.h>
# endif
# if !defined(HZ) && defined(CLOCKS_PER_SEC)
#  define HZ CLOCKS_PER_SEC
# endif
# if !defined(HZ) && defined(CLK_TCK)
#  define HZ CLK_TCK
# endif
# ifndef HZ
#  define HZ 60
# endif

#ifndef WIFSTOPPED
#define WIFSTOPPED(w) (((w) & 0xff) == 0x7f)
#endif
#ifndef WIFSIGNALED
#define WIFSIGNALED(w) (((w) & 0xff) != 0x7f && ((w) & 0xff) != 0)
#endif
#ifndef WIFEXITED
#define WIFEXITED(w) (((w) & 0xff) == 0)
#endif

#ifndef WSTOPSIG
#define WSTOPSIG(w) (((w) >> 8) & 0xff)
#endif
#ifndef WTERMSIG
#define WTERMSIG(w) ((w) & 0x7f)
#endif
#ifndef WEXITSTATUS
#define WEXITSTATUS(w) (((w) >> 8) & 0xff)
#endif

#define RETURN_VALUE( t1, t2 ) ( ((t1) << 16) | WEXITSTATUS(t2) )

const char* pres_text[] = { "Normal",
                            "Accepted",
                            "Wrong Answer",
                            "Presentation Error",
                            "Time Limit Exceed",
                            "Memory Limit Excedd",
                            "System Error",
                            "Validation Error",
                            "Not Checked" };


/* Return the number of kilobytes corresponding to a number of pages PAGES.
   (Actually, we use it to convert pages*ticks into kilobytes*ticks.)

   Try to do arithmetic so that the risk of overflow errors is minimized.
   This is funky since the pagesize could be less than 1K.
   Note: Some machines express getrusage statistics in terms of K,
   others in terms of pages.  */
static unsigned ptok ( unsigned pages )
{
    unsigned ps = 0;
    unsigned tmp;
    int size = INT_MAX;
  
    /* Initialization.  */
    ps = sysconf( _SC_PAGESIZE );

    /* Conversion.  */
    if (pages > (LONG_MAX / ps))
    {                           /* Could overflow.  */
        tmp = pages / 1024;       /* Smaller first, */
        size = tmp * ps;          /* then larger.  */
    }
    else
    {                           /* Could underflow.  */
        tmp = pages * ps;         /* Larger first, */
        size = tmp / 1024;        /* then smaller.  */
    }
    
    return size;
}

static int timeval_time_used( struct timeval* tv1, struct timeval* tv2 )
{
    return ( ( tv2->tv_sec - tv1->tv_sec ) * 1000000 +
             ( tv2->tv_usec - tv1->tv_usec ) ) / 1000; 
}

/* Wait for and fill in data on child process PID.
 * Additonally features:
 * Sleep every 20 microseconds
 * Return if the program is terminated within the system resource limit
 */
static int
resuse_end ( pid_t pid, struct RESUSE *resp, struct RESCONS *res_cons_p, int *prog_ret )
{
    int ret, status;
    struct rusage* pus;
    struct timeval tv1, tv2;
    
    pus = ( resp == NULL ? NULL : &(resp -> ru) );
    
    /*
     * Before the child process terminated, any attempt to access its resource usage is no use.
     * Currently, we use gettimeofday to measure its time usage.
     */
    gettimeofday( &tv1, NULL );
    while ( wait4( pid, &status, WNOHANG, pus ) == 0 )  {

        if ( resp != NULL && res_cons_p != NULL ) {
            gettimeofday( &tv2, NULL );
            if ( timeval_time_used( &tv1, &tv2 ) >= res_cons_p -> time_limit ) {
                kill( pid, SIGKILL );

                /* The information get is not accurate */
                getrusage( RUSAGE_CHILDREN, pus );
                pus -> ru_utime.tv_sec = tv2.tv_sec - tv1.tv_sec;
                pus -> ru_utime.tv_usec = tv2.tv_usec - tv1.tv_usec;
                
                return RES_TLE;
            }
        }
        
        usleep( 20 );
    }
    
    /*
     * Check how was the program terminiated.
     * I don't know which return code is normal for a program.
     * I realize your program aborted abnormally iif suspended by system.
     */
    ret = WIFEXITED( status ) ? RES_NORMAL : RES_SE;
    
    if ( ret == RES_NORMAL ) {
        if ( resp != NULL && res_cons_p != NULL ) {
            if ( time_used( resp ) >= res_cons_p -> time_limit ) return RES_TLE;
            if ( mem_used( resp ) >= res_cons_p -> mem_limit ) return RES_MLE;
        }
    }
    
    if ( prog_ret != NULL ) *prog_ret = status;
    
    return ret;
}


/* Prepare to measure a child process.  */
inline void
resuse_start ( struct RESUSE *resp )
{
    memset( resp, 0, sizeof( struct RESUSE ) );
    gettimeofday (&(resp->start), (struct timezone *) 0);
}

inline int
resuse_bare_measure_end( struct RESUSE* resp )
{
    int res = 0;
    
    gettimeofday(&(resp->end), (struct timezone*) 0 );
    res = (resp->end.tv_sec - resp->start.tv_sec) * 1000;
    res += (resp->end.tv_usec - resp->start.tv_usec) / 1000;
    return res;
}

inline
void resuse_add( struct RESUSE* r1, struct RESUSE* r2 )
{
    r1 -> ru.TV_SEC += r2 -> ru.TV_SEC;
    r1 -> ru.TV_USEC += r2 -> ru.TV_USEC;
    r1 -> ru.ru_minflt += r2 -> ru.ru_minflt;
}

inline
int time_used( struct RESUSE *resp )
{
    return resp -> ru.TV_MSEC1 + resp -> ru.TV_MSEC2;
}

inline
int mem_used( struct RESUSE* resp )
{
    return ptok( resp -> ru.ru_minflt );
}

/*
 * Run program under supervision.
 */
int
run_program( const char* program,
             const char* finput,
             const char* foutput,
             const char* ferror,
             struct RESCONS* res_cons_p,
             struct RESUSE* resp,
             int *prog_ret, char** argv )
{
    int ret, status = -1;
    int pid_child;
    int fd_in = -1, fd_out = - 1, fd_err = -1;
    
    // Open file descriptor
    fd_in = ( finput == NULL ? 0 : open( finput, O_RDONLY ) );
    fd_out = ( foutput == NULL ? 1 :
               open( foutput, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR ) );
    fd_err = ( ferror == NULL ? 2 : open( ferror, O_WRONLY ) );

    // Start child process
    if ( fd_in != -1 && fd_out != -1 && fd_err != -1 ) {
        
        pid_child = fork();
        if ( resp != NULL ) resuse_start( resp );
    
        if ( pid_child == 0 ) {
            /*
             * Child:
             * Override standard file descriptors
             */
            dup2( fd_in, 0 );
            dup2( fd_out, 1 );
            dup2( fd_err, 2 );

            // Return from child process by exit system call
            if ( argv != NULL ) {
                if ( execvp( program, argv ) == -1 ) exit( -1 );
            }
            else {
                if ( execlp( program, program, (char*)NULL ) == -1 ) exit(-1);
            }
        }
        else if ( pid_child > 0 ) {
            /* Parent:
             * Supervise resource usage
             */
            status = resuse_end( pid_child, resp, res_cons_p, prog_ret );
        }
        else {
            // fork Error
            fprintf( stderr, "The system call fork failed.\n" );
            status = RES_SE;
        }
    }
    
    if ( fd_in > 0 ) close( fd_in );
    if ( fd_out > 0 ) close( fd_out );
    if ( fd_err > 0 ) close( fd_err );
    
    return status;
}
