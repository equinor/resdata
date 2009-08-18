#ifndef __BLOCK_FS__
#define __BLOCK_FS__
#include <buffer.h>
#include <util.h>

typedef struct block_fs_struct block_fs_type;


block_fs_type * block_fs_mount( const char * mount_file , int block_size , int max_cache_size , bool preload);
void            block_fs_close( block_fs_type * block_fs , bool unlink_empty);
void            block_fs_fwrite_file(block_fs_type * block_fs , const char * filename , const void * ptr , size_t byte_size);
void            block_fs_fwrite_buffer(block_fs_type * block_fs , const char * filename , const buffer_type * buffer);
void            block_fs_fread_file( block_fs_type * block_fs , const char * filename , void * ptr);
void            block_fs_fprintf( const block_fs_type * block_fs , FILE * stream);
int             block_fs_get_filesize( const block_fs_type * block_fs , const char * filename);
void            block_fs_fread_realloc_buffer( block_fs_type * block_fs , const char * filename , buffer_type * buffer);
void            block_fs_sync( block_fs_type * block_fs );
void            block_fs_fprintf_log( block_fs_type * block_fs );
void            block_fs_fprintf_logfile( const char * filename);
void            block_fs_unlink_file( block_fs_type * block_fs , const char * filename);
bool            block_fs_has_file( const block_fs_type * block_fs , const char * filename);
const char    * block_fs_get_mount_point( const block_fs_type * block_fs );
void            block_fs_fprintf_index( const block_fs_type * block_fs , FILE * stream);

UTIL_IS_INSTANCE_HEADER( block_fs );

#endif
