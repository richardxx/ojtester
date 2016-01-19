/*
 * The major types used in this library, which is used to illustrate a testing scenario.
 * Sure, you can create more instances to denote different testing scenarios.
 * By richardxx, 2009.1
 */

#ifndef TYPE_DEF_H
#define TYPE_DEF_H

#include "consts.h"
#include "libprocs.h"
#include "file.h"

struct sys_arg_t
{
    // Upper running times and already evaluated times
    int runs, passed_cases;

    // Resource constraint
    struct RESCONS res_cons;
    
    // Number of testing programs
    int num_of_progs;

    // Which program produces the standard output
    int std_inx;
    
    // Pointer to list of programs
    char **progs;

    // Data generator and checking program
    char gen_prog[ FILE_NAME_LEN+1 ], checker_prog[ FILE_NAME_LEN+1 ];

    // Input and output files' name
    char input_file[ FILE_NAME_LEN + 1 ];
    char output_file[ FILE_NAME_LEN + 1 ];

    // Intermediate data storage place
    char dump_dir[ DIR_NAME_LEN + 1 ];
    
    /* Directory handlers:
     * di_in: input folder
     * di_out: output folder
     * di_temp: intermediate data folder
     */
    struct dir_info_t *di_in, *di_out, *di_temp;

    // Input and output file suffix information
    struct suf_pat_t *sp_inout;

    // Resource detector
    struct RESUSE** resp;
};

#endif
