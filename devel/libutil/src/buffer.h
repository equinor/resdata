#ifndef __BUFFER_H__
#define __BUFFER_H__


typedef struct     buffer_struct buffer_type;

buffer_type      * buffer_alloc( size_t buffer_size );
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
void               buffer_summarize(const buffer_type * buffer);

void               buffer_fwrite_int(buffer_type * buffer , int value);
int                buffer_fread_int(buffer_type * buffer );
buffer_type      * buffer_fread_alloc(const char * filename);
void               buffer_store(const buffer_type * buffer , const char * filename);
size_t 		   buffer_get_offset(const buffer_type * buffer);
size_t 		   buffer_get_size(const buffer_type * buffer);
size_t             buffer_get_remaining_size(const buffer_type *  buffer);

#endif
