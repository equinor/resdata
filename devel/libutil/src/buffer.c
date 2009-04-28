#include <util.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <buffer.h>
#include <errno.h>

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
  char     * data;      /* The actual storage */
  size_t     size;      /* The total byte size of the buffer. */
  size_t     pos;       /* The byte position in the buffer - this defines the meaningful content of the buffer. */
};


/*****************************************************************/


static void buffer_reset(buffer_type * buffer) {
  buffer->pos = 0;
}



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
    buffer->data = util_realloc(buffer->data , new_size , __func__);
    buffer->size = new_size;
  } else {
    void * tmp   = realloc(buffer->data , new_size);
    if (tmp != NULL) {
      buffer->data = tmp;
      buffer->size = new_size;
    }
  }
}


buffer_type * buffer_alloc( size_t buffer_size ) {
  buffer_type * buffer = util_malloc( sizeof * buffer , __func__);
  buffer->__id = BUFFER_TYPE_ID;
  buffer->data = NULL;
  buffer->size = 0;
  buffer_resize__( buffer , buffer_size , true);
  buffer_reset( buffer );
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
  size_t remaining_size  = buffer->size - buffer->pos;
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
  size_t remaining_size  = buffer->size - buffer->pos;
  size_t target_size     = item_size * items;
  if (target_size < remaining_size)
    buffer_resize__(buffer , buffer->pos + 2*(item_size * items) , abort_on_error);
  /**
     OK - now we have the buffer size we are going to get.
  */
  remaining_size = buffer->size - buffer->pos;
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
    return write_items;
  }
}


size_t buffer_safe_fwrite(buffer_type * buffer , const void * src_ptr , size_t item_size , size_t items) {
  return buffer_fwrite__(buffer , src_ptr , item_size , items , false);
}


size_t buffer_fwrite(buffer_type * buffer , const void * src_ptr , size_t item_size , size_t items) {
  return buffer_fwrite__(buffer , src_ptr , item_size , items , true);
}


/*****************************************************************/
/* Various (slighly) higher level functions                      */


int buffer_fread_int(buffer_type * buffer) {
  int value;
  buffer_fread(buffer , &value , sizeof value , 1);
  return value;
}


void buffer_fwrite_int(buffer_type * buffer , int value) {
  buffer_fwrite(buffer , &value , sizeof value , 1);
}





/**
   This file will read in the full content of file, and allocate a
   buffer instance with that content. Observe that the buffer->pos
   pointer is at the beginning of the buffer.
*/

void buffer_fread_realloc(buffer_type * buffer , const char * filename) {
  size_t file_size     = util_file_size( filename );
  if (buffer->size < file_size)
    buffer_resize__(buffer , file_size , true);
  {
    FILE * stream        = util_fopen(filename , "r");
    if (fread( buffer->data , 1 , file_size , stream ) != file_size) 
      util_abort("%s: failed to read all elements in file:%s \n",__func__ , filename);
    fclose( stream );
  }
  buffer_reset( buffer ); /* Positioning the pos pointer at the start of the buffer */
}



buffer_type * buffer_fread_alloc(const char * filename) {
  buffer_type * buffer = buffer_alloc( 0 );
  buffer_fread_realloc( buffer , filename );
  return buffer;
}



void buffer_fsave(const buffer_type * buffer , const char * filename) {
  FILE * stream        = util_fopen(filename , "w");
  if (fwrite( buffer->data , 1 , buffer->pos , stream ) != buffer->pos) 
    util_abort("%s: failed to write all elements to file:%s \n",__func__ , filename);
  fclose( stream );
}

