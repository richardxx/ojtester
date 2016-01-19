/*
 * A wrapper to handle file/directory requests.
 * Any file dependent features go here.
 * By richardxx, 2008.8
 */

#ifndef FILE_H
#define FILE_H

#include <sys/types.h>
#include <dirent.h>

#ifndef FILE_NAME_LEN
#	define FILE_NAME_LEN	1024
#endif

#define SUFFIX_LEN	16

/*
 * Folder information packet.
 */
struct dir_info_t
{
    DIR *pdir;
    char folder_name[ FILE_NAME_LEN + 1 ];
    int name_len;
};

/*
 * Suffix mapping information
 */
struct suf_pat_t
{
    char in_suffix[ SUFFIX_LEN + 1 ];
    char out_suffix[ SUFFIX_LEN + 1 ];
    int in_len;
    int out_len;
};


// =================== Interfaces =========================

/*
 * Open a directory.
 * Arg1 is the folder name.
 * Return NULL means failed.
 */
extern struct dir_info_t*
open_folder( const char* );

/*
 * Return next file in data directory.
 * Arg1 is the folder information package;
 * Arg2 is filled with the full path;
 * Return > 0 means the length of the full path, or 0 indicates no result.
 */
extern int
get_next_file( struct dir_info_t*, char* );

/*
 * Move the cursor to the first item.
 * Arg1 is the folder information package.
 */
extern int
reopen_folder( struct dir_info_t* );

/*
 * Rename a folder using 'mv' command.
 * From Arg1 to Arg2.
 */
extern int
rename_folder( const char*, const char* );

/*
 * Delete a folder using 'rm' command.
 * Delete all files reside in this folder.
 */
extern int
remove_folder( const char* );

/*
 * Release a folder description.
 * Arg1 is the folder information package.
 * Return > 0 means OK.
 */
extern int
close_folder( struct dir_info_t* );

/*
 * To detect how to map input file to output file.
 * Arg1 is the folder of input file;
 * Arg2 is the folder of output file.
 * Return NULL indicates no pattern found, or a pointer to structure.
 */
extern struct suf_pat_t*
detect_pattern( const char*, const char* );

/*
 * Create a user specific pattern.
 * Arg1 and arg2 are the two patterns respectively.
 * Return NULL when something wrong, or a pointer to structure.
 */
extern struct suf_pat_t*
create_pattern( const char*, const char* );

/*
 * Map a input file to output file.
 * Arg1 is the mapping information structure;
 * Arg2 is the output file directory;
 * Arg3 is the full path of the input file;
 * Arg4 is the full path of the responding output file.
 * Return the length of Arg3, 0 if failed.
 */
extern int
map_file( const struct suf_pat_t*, const char*,
          const char*, char* );

/*
 * Release resource.
 */
extern void
close_pattern( struct suf_pat_t* );

/*
 * To check if a particular file is exist.
 * Return 0 if file doesn't exist, 1 does. 
 */
extern int
file_exist( const char* );

/*
 * To check if a particular folder is exist.
 * Return 0 if folder doesn't exist, 1 does.
 */
extern int
folder_exist( const char* );

/*
 * To judge if this file is a Binary executable file.
 * Two types of file: Java class and ELF file.
 * Return 0 if error, 1 Ok.
 */
extern int
is_binary_file( const char* );

/*
 * Copy file from Arg1 to Arg2.
 * Return 0 if error, 1 OK.
 */
extern int
file_copy( const char*, const char* );

#endif

