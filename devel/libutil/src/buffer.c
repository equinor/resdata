#include <util.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <buffer.h>
#include <errno.h>
#include <zlib.h>

/**
   This function implements a small buffer type. The whole point of
   this type is that it should work (more-or-less) as a drop in
   replacement of FILE * instances (at least for unformatted
   read/write).

   I.e. instead of

     int * p = util_malloc( sizeof * p * 10 , __func__);
     fread( p , sizeof * p , 10 , stream);

   To read ten integers from a FILE * instance we should be able to
   call

     int * p = util_malloc( sizeof * p * 10 , __func__);
     buffer_fread( buffer , p , sizeof * p , 10);

*/


#define BUFFER_TYPE_ID 661043


struct buffer_struct {
  size_t     __id;
  char     * data;             /* The actual storage. */
  size_t     alloc_size;       /* The total byte size of the buffer. */
  size_t     content_size;     /* The extent of initialized data in the buffer - i.e. the meaningful content in the buffer. */
  size_t     pos;              /* The current byte position in the buffer.*/
};


/*****************************************************************/






/**
   abort_on_error == true:
   -----------------------
   The function abort with util_abort() if the allocation fails.


   abort_on_error == false:
   ------------------------
   The function will SILENTLY fail if you ask for more memory than
   the system can provide.
*/


static void buffer_resize__(buffer_type * buffer , size_t new_size, bool abort_on_error) {
  if (abort_on_error) {
    buffer->data       = util_realloc(buffer->data , new_size , __func__);
    buffer->alloc_size = new_size;
  } else {
    void * tmp   = realloc(buffer->data , new_size);
    if (tmp != NULL) {
      buffer->data = tmp;
      buffer->alloc_size = new_size;
    }
  }
  buffer->content_size = util_size_t_min( buffer->content_size , new_size ); /* If the buffer has actually shrinked. */
  buffer->pos          = util_size_t_min( buffer->pos          , new_size);  /* If the buffer has actually shrinked. */
}


buffer_type * buffer_alloc( size_t buffer_size ) {
  buffer_type * buffer = util_malloc( sizeof * buffer , __func__);
  buffer->__id = BUFFER_TYPE_ID;
  buffer->data = NULL;

  buffer->alloc_size   = 0;
  buffer->content_size = 0;
  buffer->pos          = 0;
  buffer_resize__( buffer , buffer_size , true);
  return buffer;
}



void buffer_free( buffer_type * buffer) {
  free( buffer->data );
  free( buffer );
}


/*****************************************************************/
/** 
    Observe that it is the functions with _safe_ in the name which
    most closely mimicks the behaviour of fread(), and fwrite() -
    these functions will *NOT* abort if the buffer is to small,
    instead they will just return the number of items read/written,
    and it is the responsability of the calling scope to check the
    return values.

    The functions buffer_fread() and buffer_fread() will abort if
    read/write to buffer failed.
*/



static size_t buffer_fread__(buffer_type * buffer , void * target_ptr , size_t item_size , size_t items, bool abort_on_error) {
  size_t remaining_size  = buffer->content_size - buffer->pos;
  size_t remaining_items = remaining_size / item_size;
  size_t read_items      = util_size_t_min( items , remaining_items );
  size_t read_bytes      = read_items * item_size;
  
  memcpy( target_ptr , &buffer->data[buffer->pos] , read_bytes );
  buffer->pos += read_bytes;
  
  if (read_items < items) {
    /* The buffer was not large enough - what to do now???? */
    if (abort_on_error)
      util_abort("%s: tried to read beyond the length of the buffer \n",__func__);
    else 
      /* OK we emulate fread() behaviour - setting errno to EOVERFLOW*/
      errno = EOVERFLOW;
  }
  
  return read_items;
}


size_t buffer_safe_fread(buffer_type * buffer , void * target_ptr , size_t item_size , size_t items) {
  return buffer_fread__(buffer , target_ptr , item_size , items , false);
}


size_t buffer_fread(buffer_type * buffer , void * target_ptr , size_t item_size , size_t items) {
  return buffer_fread__(buffer , target_ptr , item_size , items , true);
}


/*****************************************************************/


static size_t buffer_fwrite__(buffer_type * buffer , const void * src_ptr , size_t item_size , size_t items, bool abort_on_error) {
  size_t remaining_size  = buffer->alloc_size - buffer->pos;
  size_t target_size     = item_size * items;

  if (target_size > remaining_size) {
    buffer_resize__(buffer , buffer->pos + 2 * (item_size * items) , abort_on_error);
    /**
       OK - now we have the buffer size we are going to get.
    */
    remaining_size = buffer->alloc_size - buffer->pos;
  }
  

  {
    size_t remaining_items = remaining_size / item_size;
    size_t write_items     = util_size_t_min( items , remaining_items );
    size_t write_bytes     = write_items * item_size;

    memcpy( &buffer->data[buffer->pos] , src_ptr , write_bytes );
    buffer->pos += write_bytes;
    
    if (write_items < items) {
      /* The buffer was not large enough - what to do now???? */
      if (abort_on_error)
	util_abort("%s: failed to write %d elements to the buffer \n",__func__ , items); /* This code is never executed - abort is in resize__(); */
      else 
	/* OK we emulate fwrite() behaviour - setting errno to ENOMEM */
	errno = ENOMEM;
    }
    buffer->content_size = util_size_t_max(buffer->content_size , buffer->pos);
    return write_items;
  }
}


size_t buffer_safe_fwrite(buffer_type * buffer , const void * src_ptr , size_t item_size , size_t items) {
  return buffer_fwrite__(buffer , src_ptr , item_size , items , false);
}


size_t buffer_fwrite(buffer_type * buffer , const void * src_ptr , size_t item_size , size_t items) {
  return buffer_fwrite__(buffer , src_ptr , item_size , items , true);
}

/**
   Return value is the size (in bytes) of the compressed buffer.
*/
size_t buffer_fwrite_compressed(buffer_type * buffer, const void * ptr , size_t byte_size) {
  size_t compressed_size = 0;
  bool abort_on_error    = true;
  buffer->content_size   = buffer->pos;   /* Invalidating possible buffer content coming after the compressed content; that is uninterpretable anyway. */

  if (byte_size > 0) {
    size_t remaining_size = buffer->alloc_size - buffer->pos;
    size_t compress_bound = compressBound( byte_size );  /* Calling zlib function */
    if (compress_bound > remaining_size)
      buffer_resize__(buffer , remaining_size + compress_bound + 32 , abort_on_error); /* 32 - some extra ... */
    
    compressed_size = buffer->alloc_size - buffer->pos;
    util_compress_buffer( ptr , byte_size , &buffer->data[buffer->pos] , &compressed_size);
    buffer->pos          += compressed_size;
    buffer->content_size += compressed_size;
  }
  
  return compressed_size;
}


/**
   Return value is the size of the uncompressed buffer.
*/
size_t buffer_fread_compressed(buffer_type * buffer , size_t compressed_size , void * target_ptr , size_t target_size) {
  size_t remaining_size    = buffer->content_size - buffer->pos;
  size_t uncompressed_size = target_size;
  if (remaining_size < compressed_size)
    util_abort("%s: trying to read beyond end of buffer\n",__func__);
  

  if (compressed_size > 0) {
    if (uncompress(target_ptr , &uncompressed_size , &buffer->data[buffer->pos] , compressed_size) != Z_OK)
      util_abort("uncompress returned results != Z_OK \n",__func__);
  } else
    uncompressed_size = 0;
  
  buffer->pos += compressed_size;
  return uncompressed_size;
}


/*****************************************************************/
/* Various (slighly) higher level functions                      */

void buffer_fseek(buffer_type * buffer , ssize_t offset , int whence) {
  ssize_t new_pos = 0;

  if (whence == SEEK_SET)
    new_pos = offset;
  else if (whence == SEEK_CUR)
    new_pos = buffer->pos + offset;
  else if (whence == SEEK_END)
    new_pos = buffer->content_size;
  else 
    util_abort("%s: unrecognized whence indicator - aborting \n",__func__);

  if ((new_pos > 0) && (new_pos < buffer->content_size))
    buffer->pos = new_pos;
  else
    util_abort("%s: tried to seek to position:%ld - outside of bounds \n",__func__ , new_pos);
}

void buffer_fskip(buffer_type * buffer, ssize_t offset) {
  buffer_fseek( buffer , offset , SEEK_CUR );
}


int buffer_fread_int(buffer_type * buffer) {
  int value;
  buffer_fread(buffer , &value , sizeof value , 1);
  return value;
}


void buffer_fwrite_int(buffer_type * buffer , int value) {
  buffer_fwrite(buffer , &value , sizeof value , 1);
}


char buffer_fread_char(buffer_type * buffer) {
  char value;
  buffer_fread(buffer , &value , sizeof value , 1);
  return value;
}


void buffer_fwrite_char(buffer_type * buffer , char value) {
  buffer_fwrite(buffer , &value , sizeof value , 1);
}



/**
   Storing strings:
   ----------------

   When storing a string (\0 terminated char pointer) what is actually
   written to the buffer is

     1. The length of the string - as returned from strlen().
     2. The string content INCLUDING the terminating \0.

*/


/**
   This function will return a pointer to the current position in the
   buffer, and advance the buffer position forward until a \0
   terminater is found. If \0 is not found the thing will abort().
   
   Observe that the return value will point straight into the buffer,
   this is highly volatile memory, and in general it will be safer to
   use buffer_fread_alloc_string() to get a copy of the string.
*/

const char * buffer_fread_string(buffer_type * buffer) {
  int    string_length = buffer_fread_int( buffer );
  char * string_ptr    = &buffer->data[buffer->pos];
  char   c;
  buffer_fskip( buffer , string_length );
  c = buffer_fread_char( buffer );
  if (c != '\0')
    util_abort("%s: internal error - malformed string representation in buffer \n",__func__);
  return string_ptr;
}



char * buffer_fread_alloc_string(buffer_type * buffer) {
  return util_alloc_string_copy( buffer_fread_string( buffer ));
}



void buffer_fwrite_string(buffer_type * buffer , const char * string) {
  buffer_fwrite_int( buffer , strlen( string ));               /* Writing the length of the string */
  buffer_fwrite(buffer , string , 1 , strlen( string ) + 1);   /* Writing the string content ** WITH ** the terminating \0 */
}




/**
   This file will read in the full content of file, and allocate a
   buffer instance with that content. Observe that the buffer->pos
   pointer is at the beginning of the buffer.
*/

void buffer_fread_realloc(buffer_type * buffer , const char * filename) {
  size_t file_size     = util_file_size( filename );
  if (buffer->alloc_size < file_size)
    buffer_resize__(buffer , file_size , true);
  {
    FILE * stream        = util_fopen(filename , "r");
    if (fread( buffer->data , 1 , file_size , stream ) != file_size) 
      util_abort("%s: failed to read all elements in file:%s \n",__func__ , filename);
    fclose( stream );
  }
  buffer->pos          = 0;
  buffer->content_size = file_size;
}



buffer_type * buffer_fread_alloc(const char * filename) {
  buffer_type * buffer = buffer_alloc( 0 );
  buffer_fread_realloc( buffer , filename );
  return buffer;
}



void buffer_store(const buffer_type * buffer , const char * filename) {
  FILE * stream        = util_fopen(filename , "w");
  
  if (fwrite( buffer->data , 1 , buffer->content_size , stream ) != buffer->content_size) 
    util_abort("%s: failed to write all elements to file:%s \n",__func__ , filename);

  fclose( stream );
}


/*****************************************************************/

size_t buffer_get_offset(const buffer_type * buffer) {
  return buffer->pos;
}


size_t buffer_get_size(const buffer_type * buffer) {
  return buffer->content_size;
}


size_t buffer_get_remaining_size(const buffer_type *  buffer) {
  return buffer->content_size - buffer->pos;
}

/** 
    Returns a pointer to the internal storage of the buffer. Observe
    that this storage is volatile, and the return value from this
    function should not be kept around.
*/

void * buffer_get_data(const buffer_type * buffer) { 
  return buffer->data;
}


void buffer_summarize(const buffer_type * buffer , const char * header) {
  printf("-----------------------------------------------------------------\n");
  if (header != NULL)
    printf("%s \n",header);
  printf("   Allocated size .....: %10ld bytes \n",buffer->alloc_size);
  printf("   Content size .......: %10ld bytes \n",buffer->content_size);
  printf("   Current position ...: %10ld bytes \n",buffer->pos);
  printf("-----------------------------------------------------------------\n");
}
