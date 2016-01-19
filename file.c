/*
 * Some useful utilities to handle files in a directory, including:
 * 1. Simple pattern matching and transforming;
 * 2. Management of folders;
 * 3. Binary file detection.
 *
 * By richardxx, 2008.8
 * Revised in 2009.1
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include "file.h"

#define MAGIC_NUMBER_LEN			4
#define MAGIC_TYPE				2
#define CHUNK_SIZE				4096

const int magic_number[] = { 0x7f454c46, 0xcafebabe };
static char tmp_buf[ CHUNK_SIZE + 1 ];

/*
 * Create this folder if doesn't exist.
 */
struct dir_info_t* open_folder( const char* path )
{
    DIR* pdir;
    struct dir_info_t *pret;

    if ( !folder_exist( path ) ) {
        // Do create
        if ( mkdir( path, S_IRWXU ) == -1 ) return NULL;
    }

    pdir = opendir( path );
    if ( pdir == NULL ) return NULL;
    
    pret = ( struct dir_info_t* ) malloc( sizeof( struct dir_info_t ) );
    if ( pret == NULL ) {
        closedir( pdir );
        return NULL;
    }
  
    pret -> pdir = pdir;
    strcpy( pret -> folder_name, path );
    strcat( pret -> folder_name, "/" );
    pret -> name_len = strlen( path ) + 1;
    
    return pret;
}

int get_next_file( struct dir_info_t* my_dir, char* full_path )
{
    struct dirent* dir_ent;
    struct stat stat_buf;
  
    while ( 1 ) {
        dir_ent = readdir( my_dir -> pdir );
        if ( dir_ent == NULL ) {
            // Error or nothing left
            return 0;
        }

        if ( strcmp( dir_ent -> d_name, "." ) == 0 ||
             strcmp( dir_ent -> d_name, ".." ) == 0 ) continue;
    
        // Only these two types are allowed
        sprintf( full_path, "%s/%s",
                 my_dir -> folder_name,
                 dir_ent -> d_name );
        
        if ( stat( full_path, &stat_buf ) < 0 ) continue;
        if ( S_ISREG( stat_buf.st_mode ) ||
             S_ISLNK( stat_buf.st_mode ) ) break;
    }
  
    return my_dir -> name_len + strlen( dir_ent -> d_name );
}

int reopen_folder( struct dir_info_t* my_dir )
{
    if ( my_dir == NULL || my_dir -> pdir == NULL ) return 0;
    rewinddir( my_dir -> pdir );
    return 1;
}

int rename_folder( const char* p1, const char* p2 )
{
    sprintf( tmp_buf, "mv %s %s", p1, p2 );
    return system( tmp_buf ) == 0 ? 1 : 0;
}

int remove_folder( const char* path )
{
    sprintf( tmp_buf, "rm -rf %s", path );
    return system( tmp_buf ) == 0 ? 1 : 0;
}

int close_folder( struct dir_info_t* my_dir )
{
    int ret;
    
    if ( my_dir == NULL ) return 0;
    ret = closedir( my_dir -> pdir );
    free( my_dir );
    return ret == -1 ? 0 : 1;
}

/*
 * Open the first DETECT_NUMBER files to test.
 */
struct suf_pat_t* detect_pattern( const char* indir, const char* outdir )
{
    int i, k;
    int flag;
    DIR *dir_in, *dir_out;
    struct dirent *dirent_in, *dirent_out;
    struct suf_pat_t* psuf_ret;

    // Request resource
    dir_in = opendir( indir );
    if ( dir_in == NULL ) return NULL;
    dir_out = opendir( outdir );
    if ( dir_out == NULL ) {
        closedir( dir_in );
        return NULL;
    }
    psuf_ret = ( struct suf_pat_t* )malloc( sizeof( struct suf_pat_t ) );
    if ( psuf_ret == NULL ) {
        closedir( dir_in );
        closedir( dir_out );
        return NULL;
    }

    flag = 0;
    
    // Choose the first entry in indir
    while ( 1 ) {
        dirent_in = readdir( dir_in );
        if ( dirent_in == NULL ) {
            // Empty input folder
            goto release_code;
        }
        if ( strcmp( dirent_in -> d_name, "." ) == 0 ||
             strcmp( dirent_in -> d_name, ".." ) == 0 ) continue;

        break;
    }

    // Check if there's an entry with corressponding prefix
    while ( flag == 0 ) {
      
        dirent_out = readdir( dir_out );
        if ( dirent_out == NULL ) {
            // Empty output folder
            goto release_code;
        }
        if ( strcmp( dirent_in -> d_name, "." ) == 0 ||
             strcmp( dirent_in -> d_name, ".." ) == 0 ) continue;
        
        for ( i = 0;
              dirent_in -> d_name[ i ] && dirent_out -> d_name[ i ];
              ++i ) {
            if ( dirent_in -> d_name[ i ] != dirent_out -> d_name[ i ] ) break;
            if ( dirent_in -> d_name[ i ] == '.' ) {
                flag = 1;
                break;
            }
        }

        // If neither of them has dot separated suffix
        if ( dirent_in -> d_name[i] == dirent_out -> d_name[i] &&
             dirent_in -> d_name[i] == 0 ) {
            flag = 1;
            continue;
        }

        if ( flag == 1 ) {
            
            psuf_ret -> in_suffix[ SUFFIX_LEN - 1 ] = 0;
            psuf_ret -> out_suffix[ SUFFIX_LEN - 1 ] = 0;
            
            for ( k = i;
                  dirent_in -> d_name[k] && k - i + 1 < SUFFIX_LEN - 1;
                  ++k )
                psuf_ret -> in_suffix[ k - i ] = dirent_in -> d_name[k];

            // Length of suffix of input files exceed the limit
            if ( dirent_in -> d_name[k] != 0 ) goto release_code;
            psuf_ret -> in_suffix[ k - i ] = 0;
            psuf_ret -> in_len = k - i;
            
            for ( k = i;
                  dirent_out -> d_name[k] && k - i + 1 < SUFFIX_LEN - 1;
                  ++k )
                psuf_ret -> out_suffix[ k - i ] = dirent_out -> d_name[k];

            // Length of suffix of ouput files exceed the limit
            if ( dirent_out -> d_name[k] != 0 ) goto release_code;
            psuf_ret -> out_suffix[ k - i ] = 0;
            psuf_ret -> out_len = k - i;
        }
    }

  release_code:
    closedir( dir_in );
    closedir( dir_out );
    if ( flag == 0 ) {
        free( psuf_ret );
        return NULL;
    }
    
    return psuf_ret;
}

/*
 * Don't touch anything if the suffix of input file miss the the detected pattern.
 */
int map_file( const struct suf_pat_t* psuf, const char* out_dir, 
              const char* fin, char* fout )
{
    int i, j, k;
    int len_in;

    for ( i = psuf -> in_len - 1, j = ( len_in = strlen( fin ) ) - 1;
          i > -1 && j > -1;
          --i, --j )
        if ( psuf -> in_suffix[ i ] != fin[ j ] ) return 0;

    if ( j == -1 ) return 0;

    // Terrible!
    sprintf( fout, "%s/", out_dir );
    for ( k = j; k > -1 && fin[k] != '/'; --k );
    for ( ++k, i = strlen( fout ); k <= j; ++k, ++i )
        fout[i] = fin[k];
    
    for ( j = 0; j < psuf -> out_len; ++j, ++i )
        fout[ i ] = psuf -> out_suffix[j];
    
    fout[ i ] = 0;
    
    return 1;
}

void close_pattern( struct suf_pat_t* psuf )
{
    if ( psuf != NULL ) free( psuf );
}

/*
 * Note that the file is not actually opened.
 */
int file_exist( const char* fname )
{
    extern int errno;

    errno = 0;
    if ( open( fname, O_CREAT | O_EXCL ) == -1 
         && errno == EEXIST ) {
        return 1;
    }
    
    return 0;
}

int folder_exist( const char* folder )
{
    extern int errno;

    errno = 0;
    if ( open( folder, O_WRONLY ) == -1
         && errno == EISDIR ) {
        return 1;
    }
    
    return 0;
}

// Exam the magic number of an executable file
int is_binary_file( const char* fname )
{
    int i;
    int fd, number;
    char buf[ MAGIC_NUMBER_LEN + 1 ];
    
    if ( ( fd = open( fname, O_RDONLY ) ) == -1 )
        // Unexpected error, maybe this file doesn't exist?
        return 0;

    buf[ MAGIC_NUMBER_LEN ] = 0;
    if ( read( fd, buf, MAGIC_NUMBER_LEN ) != MAGIC_NUMBER_LEN ) {
        // This file is broken
        close( fd );
        return 0;
    }

    close( fd );

    number = (buf[0]<<24) | (buf[1]<<16) | (buf[2]<<8) | (buf[3]);
    
    for ( i = 0; i < MAGIC_TYPE; ++i )
        if ( magic_number[i] == number ) return 1;
    
    return 0;
}

// Copy file
int file_copy( const char* psrc, const char* pdest )
{
    int ret, fd1, fd2;

    if ( ( fd1 = open( psrc, O_RDONLY ) ) == -1 ) {
        return 0;
    }

    if ( ( fd2 = open( pdest, O_CREAT | O_WRONLY | O_TRUNC,
                       S_IRUSR | S_IWUSR ) ) == -1 ) {
        close( fd1 );
        return 0;
    }

    while ( (ret = read( fd1, tmp_buf, CHUNK_SIZE )) ) {
        if ( write( fd2, tmp_buf, ret ) < ret ) {
#ifdef DEBUG
            fprintf( stderr, "Write %s error.\n", pdest );
#endif
            break;
        }
    }

    close( fd1 );
    close( fd2 );
    return 1;
}
