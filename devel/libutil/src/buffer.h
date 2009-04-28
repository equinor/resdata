#ifndef __BUFFER_H__
#define __BUFFER_H__


typedef struct     buffer_struct buffer_type;

buffer_type      * buffer_alloc( size_t buffer_size );
void               buffer_free( buffer_type * buffer);
size_t             buffer_safe_fread(buffer_type * buffer , void * target_ptr , size_t item_size , size_t items);
size_t             buffer_fread(buffer_type * buffer , void * target_ptr , size_t item_size , size_t items);
size_t             buffer_safe_fwrite(buffer_type * buffer , const void * src_ptr , size_t item_size , size_t items);
size_t             buffer_fwrite(buffer_type * buffer , const void * src_ptr , size_t item_size , size_t items);

void               buffer_fwrite_int(buffer_type * buffer , int value);
int                buffer_fread_int(buffer_type * buffer );
buffer_type      * buffer_fread_alloc(const char * filename);

#endif
