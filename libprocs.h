/*
 * Process management library.
 * Part of the source code coming from UNIX 'time' command, thanks.
 * By richardxx, 2009.2
 */

#ifndef _LIBPROCS_H
#define _LIBPROCS_H 1

#include <sys/time.h>
#include <sys/resource.h>

/* Information on the resources used by a child process.  */
struct RESUSE
{
    struct rusage ru;              /* Real CPU time of process. */
    struct timeval start, end;     /* Wallclock time of process.  */
};

/* Information on resource limitations owned by a child process. */
struct RESCONS
{
    int time_limit;
    int mem_limit;
};

#define TV_SEC  ru_utime.tv_sec
#define TV_USEC ru_utime.tv_usec
#define TV_MSEC1 TV_SEC * 1000
#define TV_MSEC2 TV_USEC / 1000

#if defined(sun3) || defined(hp300) || defined(ibm032)
#define TICKS_PER_SEC 50
#endif
#if defined(mips)
#define TICKS_PER_SEC 250
#endif
#ifndef TICKS_PER_SEC
#define TICKS_PER_SEC 100
#endif

/* The number of milliseconds in one `tick' used by the `rusage' structure.  */
#define MSEC_PER_TICK (1000 / TICKS_PER_SEC)

/* Return the number of clock ticks that occur in M milliseconds.  */
#define MSEC_TO_TICKS(m) ((m) / MSEC_PER_TICK)

// System code for process status
#define RES_NORMAL		0
#define RES_AC			1
#define RES_WA			2
#define RES_PE			3
#define RES_TLE			4
#define RES_MLE         5
#define RES_SE			6
#define RES_VE			7
#define RES_NOT_CHECK	8

// Text description of constants above
extern const char* pres_text[];

/*
 * Create a process to run particular program.
 * This function could take full control of child process.
 * Return:
 * System code
 */
int run_program( const char*,     // program name
                 const char*,     // input file
                 const char*,     // output file
                 const char*,     // error output file
                 struct RESCONS*, // resource usage constraints
                 struct RESUSE*,  // resource measurement
                 int*,            // return value of child process
                 char**           // arguments for child process
                 );

/* Clear */
inline void resuse_start( struct RESUSE* );

/* Simply end with time recording */
inline int resuse_bare_measure_end( struct RESUSE* );

/* Add two program's resources */
inline void resuse_add( struct RESUSE*, struct RESUSE* );

/* Get how many microseconds a program occupies. */
inline int time_used( struct RESUSE* );

/* Get the memory peak a program reaches. */
inline int mem_used( struct RESUSE* );

#endif /* _RESUSE_H */
