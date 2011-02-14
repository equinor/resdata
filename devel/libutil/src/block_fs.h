/*
   Copyright (C) 2011  Statoil ASA, Norway. 
    
   The file 'block_fs.h' is part of ERT - Ensemble based Reservoir Tool. 
    
   ERT is free software: you can redistribute it and/or modify 
   it under the terms of the GNU General Public License as published by 
   the Free Software Foundation, either version 3 of the License, or 
   (at your option) any later version. 
    
   ERT is distributed in the hope that it will be useful, but WITHOUT ANY 
   WARRANTY; without even the implied warranty of MERCHANTABILITY or 
   FITNESS FOR A PARTICULAR PURPOSE.   
    
   See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html> 
   for more details. 
*/

#ifndef __BLOCK_FS__
#define __BLOCK_FS__
#include <buffer.h>
#include <util.h>
#include <vector.h>

typedef struct block_fs_struct  block_fs_type;
typedef struct file_node_struct file_node_type;

typedef enum {
  NO_SORT     = 0,
  STRING_SORT = 1,
  OFFSET_SORT = 2
} block_fs_sort_type;


double          block_fs_get_fragmentation( const block_fs_type * block_fs );
bool            block_fs_rotate( block_fs_type * block_fs , double fragmentation_limit);
void            block_fs_fsync( block_fs_type * block_fs );
bool            block_fs_is_mount( const char * mount_file );
block_fs_type * block_fs_mount( const char * mount_file , int block_size , int max_cache_size , float fragmentation_limit , int fsync_interval , bool preload , bool read_only);
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
vector_type   * block_fs_alloc_filelist( const block_fs_type * block_fs  , const char * pattern , block_fs_sort_type sort_mode , bool include_free_nodes );
void            block_fs_defrag( block_fs_type * block_fs );

long int        file_node_get_node_offset( const file_node_type * file_node );
long int        file_node_get_data_offset( const file_node_type * file_node );
int             file_node_get_node_size( const file_node_type * file_node );
int             file_node_get_data_size( const file_node_type * file_node );
bool            file_node_in_use( const file_node_type * file_node );
const char *    file_node_get_filename( const file_node_type * file_node );



UTIL_IS_INSTANCE_HEADER( block_fs );

#endif
