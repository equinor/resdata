#ifndef __BUFFER_H__
#define __BUFFER_H__

#ifdef __cplusplus
extern "C" {
#endif
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

typedef struct     buffer_struct buffer_type;

void               buffer_memshift(buffer_type * buffer , size_t offset, ssize_t shift);
bool               buffer_strstr( buffer_type * buffer , const char * expr );
buffer_type      * buffer_alloc( size_t buffer_size );
buffer_type      * buffer_alloc_wrapper(void * data , size_t buffer_size );
void               buffer_free_container( buffer_type * buffer );
void               buffer_free( buffer_type * buffer);
size_t             buffer_safe_fread(buffer_type * buffer , void * target_ptr , size_t item_size , size_t items);
size_t             buffer_fread(buffer_type * buffer , void * target_ptr , size_t item_size , size_t items);
size_t             buffer_safe_fwrite(buffer_type * buffer , const void * src_ptr , size_t item_size , size_t items);
size_t             buffer_fwrite(buffer_type * buffer , const void * src_ptr , size_t item_size , size_t items);
size_t 		   buffer_fwrite_compressed(buffer_type * buffer, const void * ptr , size_t byte_size);
size_t 		   buffer_fread_compressed(buffer_type * buffer , size_t compressed_size , void * target_ptr , size_t target_size);
const char       * buffer_fread_string(buffer_type * buffer);
char             * buffer_fread_alloc_string(buffer_type * buffer);
void               buffer_fwrite_string(buffer_type * buffer , const char * string);
void               buffer_summarize(const buffer_type * buffer , const char *);

void               buffer_fwrite_char(buffer_type * buffer , char value);
void               buffer_fwrite_int(buffer_type * buffer , int value);
int                buffer_fread_int(buffer_type * buffer );
void               buffer_store(const buffer_type * buffer , const char * filename);
size_t 		   buffer_get_offset(const buffer_type * buffer);
size_t 		   buffer_get_size(const buffer_type * buffer);
size_t             buffer_get_remaining_size(const buffer_type *  buffer);
void             * buffer_get_data(const buffer_type * buffer);
void             * buffer_alloc_data_copy(const buffer_type * buffer);
void               buffer_stream_fwrite( const buffer_type * buffer , FILE * stream );
int                buffer_fgetc( buffer_type * buffer );
void 		   buffer_fseek(buffer_type * buffer , ssize_t offset , int whence);
void 		   buffer_fskip(buffer_type * buffer, ssize_t offset);
void               buffer_clear( buffer_type * buffer );

void               buffer_fskip_int(buffer_type * buffer);
void   		   buffer_fskip_time_t(buffer_type * buffer);
time_t 		   buffer_fread_time_t(buffer_type * buffer);
void   		   buffer_fwrite_time_t(buffer_type * buffer , time_t value);
void               buffer_rewind(buffer_type * buffer );

double             buffer_fread_double(buffer_type * buffer);
void               buffer_fwrite_double(buffer_type * buffer , double value);

size_t             buffer_stream_fwrite_n( const buffer_type * buffer , size_t offset , ssize_t write_size , FILE * stream );
void               buffer_stream_fprintf( const buffer_type * buffer , FILE * stream );
void               buffer_stream_fread( buffer_type * buffer , size_t byte_size , FILE * stream);
buffer_type      * buffer_fread_alloc(const char * filename);
void               buffer_fread_realloc(buffer_type * buffer , const char * filename);

#ifdef __cplusplus
}
#endif

#endif
