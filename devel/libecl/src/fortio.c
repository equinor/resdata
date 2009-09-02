#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fortio.h>
#include <util.h>
#include <endian_flip.h>

extern int errno;

/**
The fortio struct is implemented to handle fortran io. The problem is
that when a Fortran program writes unformatted data to file in a
statemente like:

   integer array(100)
   write(unit) array

it actually writes a head and tail in addition to the actual
data. The header and tail is a 4 byte integer, which value is the
number of bytes in the immediately following record. I.e. what is
actually found on disk after the Fortran code above is:

  | 400 | array ...... | 400 |

Where the "400" head and tail is the number of bytes in the following
record. Fortran IO handles this transparently, but when mixing with
other programming languages care must be taken. This file implements
functionality to read and write these fortran generated files
transparently. The three functions: 

  1. fortio_fopen()
  2. fortio_fread_record()
  3. fortio_fwrite_record()

together constitute something very similar to fopen() , fread() and
fwrite() from the standard library.
*/


struct fortio_struct {
  FILE    *stream;
  char    *filename;
  int      active_header;
  int      rec_nr;
  bool     endian_flip_header;  
  bool     fmt_file;            /* This is not really used by the fortio instance - but it is very convenient to store it here. */
};



static fortio_type * fortio_alloc__(const char *filename , bool endian_flip_header, bool fmt_file) {
  fortio_type * fortio       = (fortio_type *) util_malloc(sizeof * fortio , __func__);
  fortio->filename           = util_alloc_string_copy(filename);
  fortio->endian_flip_header = endian_flip_header;
  fortio->active_header      = 0;
  fortio->rec_nr             = 0; 
  fortio->fmt_file           = fmt_file;
  return fortio;
}



/**
   Helper function for fortio_is_fortran_stream__(). 
*/

static bool __read_int(FILE * stream , int * value, bool endian_flip) {
  /* This fread() can legitemately fail - can not use util_fread() here. */
  if (fread(value , sizeof * value , 1 , stream) == 1) {
    if (endian_flip)
      util_endian_flip_vector(value , sizeof * value , 1);
    return true;
  } else
    return false;
}


/**
   Helper function for fortio_is_fortran_file(). Checks whether a
   particular stream is formatted according to fortran io, for a fixed
   endian ness.
*/

static bool fortio_is_fortran_stream__(FILE * stream , bool endian_flip) {
  const bool strict_checking = true;          /* True: requires that *ALL* records in the file are fortran formatted */
  long init_pos              = ftell(stream);
  bool is_fortran_stream     = false;
  int header , tail;
  bool cont;

  do {
    cont = false;
    if (__read_int(stream , &header , endian_flip)) {
      if (header >= 0) {
	if (fseek(stream , header , SEEK_CUR) == 0) {
	  if (__read_int(stream , &tail , endian_flip)) {
	    cont = true;
	    /* 
	       OK - now we have read a header and a tail - it might be
	       a fortran file.
	    */
	    if (header == tail) {
	      if (header != 0) {
		/* This is (most probably) a fortran file */
		is_fortran_stream = true;
		if (strict_checking)
		  cont = true;  
		else
		  cont = false;
	      }
	      /* Header == tail == 0 - we don't make any inference on this. */
	    } else {
	      /* Header != tail => this is *not* a fortran file */
	      cont = false;
	      is_fortran_stream = false;
	    }
	  }
	}
      } 
    }
  } while (cont);
  fseek(stream , init_pos , SEEK_SET);
  return is_fortran_stream;
}


/**
   This function tries (using some heuristic) to guess whether a
   particular file is a Fortran file. To complicate the matters
   further we make no assumptions regarding endian ness, if it is
   indeed determined that this is fortran file, the endian ness is
   returned by reference (if it is not recognized as a fortran file, 
   the returned endian ness will be garbage).

   The heuristic algorithm which is used is as follows:
   
    1. Read four bytes as an integer (header)
    2. Skip that number of bytes forward.
    3. Read four bytes again (tail).

   Now, when this is done we do the following tests:

    1. If header == tail. This is (probably) a fortran file, however
       if header == 0, we might have a normal file with two
       consequitive zeroes. In that case it is difficult to determine,
       and we continue.

    2. If header != tail we try to reinterpret header with an endian
       swap and read a new tail. If they are now equal we repeat test1, or
       return false (i.e. *not* a fortran file).
*/

bool fortio_is_fortran_file(const char * filename, bool * _endian_flip) {
  FILE * stream = util_fopen(filename , "r");
  bool endian_flip = false;          
  bool is_fortran_stream = fortio_is_fortran_stream__(stream , endian_flip);
  if (!is_fortran_stream) {
    endian_flip = !endian_flip;
    is_fortran_stream = fortio_is_fortran_stream__(stream , endian_flip);
  }

  *_endian_flip = endian_flip;
  fclose(stream);
  return is_fortran_stream;
}


/**
   This function tries to determine automatically whether a certain
   file has endian flip or not. 

   Observe that the return value is whether we managed to determine the
   endian-ness or not, whereas the endian_flip flag is returned by
   reference.

   To be able to determine endianness the file *must* be a binary
   fortran file - this is essentially the return value.
*/

bool fortio_guess_endian_flip(const char * filename , bool * _endian_flip) {
  return fortio_is_fortran_file(filename , _endian_flip);
}





fortio_type * fortio_alloc_FILE_wrapper(const char *filename , bool endian_flip_header , bool fmt_file , FILE * stream) {
  fortio_type * fortio = fortio_alloc__(filename , endian_flip_header , fmt_file);
  fortio->stream = stream;
  return fortio;
}


fortio_type * fortio_fopen(const char *filename , const char *mode, bool endian_flip_header , bool fmt_file) {
  fortio_type *fortio = fortio_alloc__(filename , endian_flip_header , fmt_file);
  
  fortio->stream = util_fopen(fortio->filename , mode);
  return fortio;
}



static void fortio_free__(fortio_type * fortio) {
  if (fortio->filename != NULL) free(fortio->filename);
  free(fortio);
}

void fortio_free_FILE_wrapper(fortio_type * fortio) {
  fortio_free__(fortio);
}


void fortio_fclose(fortio_type *fortio) {
  fclose(fortio->stream);
  fortio_free__(fortio);
}


bool fortio_is_fortio_file(fortio_type * fortio) {
  FILE * stream = fortio->stream;
  int init_pos = ftell(stream);
  int elm_read;
  bool is_fortio_file = false;
  elm_read = fread(&fortio->active_header , sizeof(fortio->active_header) , 1 , fortio->stream);
  if (elm_read == 1) {
    int trailer;

    if (fortio->endian_flip_header)
      util_endian_flip_vector(&fortio->active_header , sizeof fortio->active_header , 1);

    if (fseek(stream , fortio->active_header , SEEK_CUR) == 0) {
      if (fread(&trailer , sizeof(fortio->active_header) , 1 , fortio->stream) == 1) {
	if (fortio->endian_flip_header)
	  util_endian_flip_vector(&trailer , sizeof trailer , 1);
	
	if (trailer == fortio->active_header)
	  is_fortio_file = true;
      }
    } 
  }

  fseek(stream , init_pos , SEEK_SET);
  return is_fortio_file;
}


/*
  This function returns -1 on error - it does *NOT* fail. The -1
  return value is interpreted in the ecl_kw layer.
*/

int fortio_init_read(fortio_type *fortio) {
  int elm_read;
  elm_read = fread(&fortio->active_header , sizeof(fortio->active_header) , 1 , fortio->stream);
  if (elm_read == 1) {
    if (fortio->endian_flip_header)
      util_endian_flip_vector(&fortio->active_header , sizeof fortio->active_header , 1);

    fortio->rec_nr++;
    return fortio->active_header;
  } else 
    return -1;
}



void fortio_complete_read(fortio_type *fortio) {
  int trailer;
  trailer = util_fread_int( fortio->stream );
  
  if (fortio->endian_flip_header)
    util_endian_flip_vector(&trailer , sizeof trailer , 1);
  
  if (trailer != fortio->active_header) {
    fprintf(stderr,"%s: fatal error reading record:%d in file: %s - aborting \n",__func__ , fortio->rec_nr , fortio->filename);
    util_abort("%s: Header: %d   Trailer: %d \n",__func__ , fortio->active_header , trailer);
  }
  fortio->active_header = 0;
}


/**
   This function reads one record from the fortio stream, and fills
   the buffer with the content. The return value is the number of bytes read.
*/

int fortio_fread_record(fortio_type *fortio, char *buffer) {
  fortio_init_read(fortio);
  int record_size = fortio->active_header; /* This is reset in fortio_complete_read - must store it for the return. */
  util_fread(buffer , 1 , fortio->active_header , fortio->stream , __func__);
  fortio_complete_read(fortio);
  return record_size;
}


/**
   This function fills the buffer with 'buffer_size' bytes from the
   fortio stream. The function works by repeated calls to
   fortio_read_record(), until the desired number of bytes of been
   read. The point of this is to handle the ECLIPSE system with blocks
   of e.g. 1000 floats (which then become one fortran record), in a
   transparent, low-level way.
*/

void fortio_fread_buffer(fortio_type * fortio, char * buffer , int buffer_size) {
  int bytes_read = 0;
  while (bytes_read < buffer_size) {
    char * buffer_ptr = &buffer[bytes_read];
    bytes_read += fortio_fread_record(fortio , buffer_ptr);
  }
  if (bytes_read > buffer_size) 
    util_abort("%s: hmmmm - something is broken. The individual records in %s did not sum up to the expected buffer size \n",__func__ , fortio->filename);

}


int fortio_fskip_record(fortio_type *fortio) {
  int record_size = fortio_init_read(fortio);
  fseek(fortio->stream , record_size , SEEK_CUR);
  fortio_complete_read(fortio);
  return record_size;
}

void fortio_fskip_buffer(fortio_type * fortio, int buffer_size) {
  int bytes_skipped = 0;
  while (bytes_skipped < buffer_size) 
    bytes_skipped += fortio_fskip_record(fortio);

  if (bytes_skipped > buffer_size) 
    util_abort("%s: hmmmm - something is broken. The individual records in %s did not sum up to the expected buffer size \n",__func__ , fortio->filename);
}


void fortio_copy_record(fortio_type * src_stream , fortio_type * target_stream , int buffer_size , void * buffer , bool *at_eof) {
  int bytes_read;
  int record_size = fortio_init_read(src_stream); 
  fortio_init_write(target_stream , record_size);

  bytes_read = 0;
  while (bytes_read < record_size) {
    int bytes;
    if (record_size > buffer_size)
      bytes = buffer_size;
    else
      bytes = record_size - bytes_read;

    util_fread(buffer , 1 , bytes , src_stream->stream     , __func__);
    util_fwrite(buffer , 1 , bytes , target_stream->stream , __func__);
    
    bytes_read += bytes;
  }

  fortio_complete_read(src_stream);
  fortio_complete_write(target_stream);

  if (feof(src_stream->stream))
    *at_eof = true;
  else
    *at_eof = false;
}


/*****************************************************************/

void  fortio_init_write(fortio_type *fortio , int record_size) {
  int file_header;
  fortio->active_header = record_size;
  file_header = fortio->active_header;
  if (fortio->endian_flip_header)
    util_endian_flip_vector(&file_header , sizeof file_header , 1);
  
  util_fwrite_int( file_header , fortio->stream );
  fortio->rec_nr++;
}

void fortio_complete_write(fortio_type *fortio) {
  int file_header = fortio->active_header;
  if (fortio->endian_flip_header)
    util_endian_flip_vector(&file_header , sizeof file_header , 1);

  util_fwrite_int( file_header , fortio->stream );
  fortio->active_header = 0;
}


void fortio_fwrite_record(fortio_type *fortio, const char *buffer , int record_size) {
  fortio_init_write(fortio , record_size);
  util_fwrite( buffer , 1 , record_size , fortio->stream , __func__);
  fortio_complete_write(fortio);
}


void * fortio_fread_alloc_record(fortio_type * fortio) {
  void * buffer;
  fortio_init_read(fortio);
  buffer = util_malloc(fortio->active_header , __func__);
  util_fread(buffer , 1 , fortio->active_header , fortio->stream , __func__);
  fortio_complete_read(fortio);
  return buffer;
}


/*****************************************************************/
void          fortio_fflush(fortio_type * fortio) { fflush( fortio->stream); }
FILE        * fortio_get_FILE(const fortio_type *fortio)        { return fortio->stream; }
int           fortio_get_record_size(const fortio_type *fortio) { return fortio->active_header; }
bool          fortio_endian_flip(const fortio_type *fortio) 	{ return fortio->endian_flip_header; }
bool          fortio_fmt_file(const fortio_type *fortio)    	{ return fortio->fmt_file; }
void          fortio_rewind(const fortio_type *fortio)          { rewind(fortio->stream); }
const char  * fortio_filename_ref(const fortio_type * fortio)   { return (const char *) fortio->filename; }
