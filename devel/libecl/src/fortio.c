#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fortio.h>
#include <util.h>

extern int errno;

/**
The fortio struct is implemented to handle fortran io. The problem is
that when a Fotran program writes unformatted data to file in a
statemente like:

   integer array(100)
   write(unit) array

it actually writes a head and tail in addition to the actual
data. The header and tail is a 4 byte integer, which value is the
number of bytes in the immediately following record. I.e. what is
actually found on disk after the Fotran code above is:

  | 400 | array ...... | 400 |

Where the "400" head and tail is the number of bytes in the following
record. Fortran IO handles this transparently, but when mixing with
other programming languages care must be taken. This file implements
functionality to read and write these fortran generated files
transparently. The three functions: 

  1. fortio_open()
  2. fortio_read_record()
  3. fortio_write_record()

together constitute something very similar to fopen() , fread() and
fwrite() from the standard library.
*/


struct fortio_struct {
  FILE    *stream;
  char    *filename;
  int      active_header;
  int      rec_nr;
  bool     endian_flip_header;
};



static fortio_type * fortio_alloc__(const char *filename , bool endian_flip_header) {
  fortio_type * fortio       = malloc(sizeof * fortio);
  fortio->filename           = util_alloc_string_copy(filename);
  fortio->endian_flip_header = endian_flip_header;
  fortio->active_header      = 0;
  fortio->rec_nr             = 0; 
  return fortio;
}



/**
   Helper function for fortio_is_fortran_stream__(). 
*/

static bool __read_int(FILE * stream , int * value, bool endian_flip) {
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
   returned by reference.

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




fortio_type * fortio_alloc_FILE_wrapper(const char *filename , bool endian_flip_header , FILE * stream) {
  fortio_type * fortio = fortio_alloc__(filename , endian_flip_header);
  fortio->stream = stream;
  return fortio;
}


fortio_type *fortio_open(const char *filename , const char *mode, bool endian_flip_header) {
  fortio_type *fortio = fortio_alloc__(filename , endian_flip_header);
  
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


void fortio_close(fortio_type *fortio) {
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
  fread(&trailer , sizeof(fortio->active_header) , 1 , fortio->stream);
  if (fortio->endian_flip_header)
    util_endian_flip_vector(&trailer , sizeof trailer , 1);
  
  if (trailer != fortio->active_header) {
    fprintf(stderr,"%s: fatal error reading record:%d in file: %s - aborting \n",__func__ , fortio->rec_nr , fortio->filename);
    fprintf(stderr,"\nHeader: %d   Trailer: %d \n",fortio->active_header , trailer);
    abort();
  }
  fortio->active_header = 0;
}

void fortio_read_record(fortio_type *fortio, char *buffer) {
  fortio_init_read(fortio);
  fread(buffer , 1 , fortio->active_header , fortio->stream);
  fortio_complete_read(fortio);
}


void fortio_skip_record(fortio_type *fortio) {
  int record_size = fortio_init_read(fortio);
  fseek(fortio->stream , record_size , SEEK_CUR);
  fortio_complete_read(fortio);
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

    {
      bool ok;
      ok = true;
      ok =        (fread (buffer , 1 , bytes , src_stream->stream)    == bytes);
      ok = (ok && (fwrite(buffer , 1 , bytes , target_stream->stream) == bytes));
      if (!ok) {
	fprintf(stderr,"%s: failed to read/write %d bytes - aborting \n",__func__ , bytes);
	abort();
      }
    }
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
  
  fwrite(&file_header , sizeof(fortio->active_header) , 1 , fortio->stream);
  fortio->rec_nr++;
}

void fortio_complete_write(fortio_type *fortio) {
  int file_header = fortio->active_header;
  if (fortio->endian_flip_header)
    util_endian_flip_vector(&file_header , sizeof file_header , 1);

  fwrite(&file_header, sizeof(fortio->active_header) , 1 , fortio->stream);
  fortio->active_header = 0;
}


void fortio_write_record(fortio_type *fortio, const char *buffer , int record_size) {
  fortio_init_write(fortio , record_size);
  fwrite(buffer , 1 , record_size , fortio->stream);
  fortio_complete_write(fortio);
}


void * fortio_fread_alloc_record(fortio_type * fortio) {
  void * buffer;
  fortio_init_read(fortio);
  buffer = malloc(fortio->active_header);
  fread(buffer , 1 , fortio->active_header , fortio->stream);
  fortio_complete_read(fortio);
  return buffer;
}


/*****************************************************************/

FILE        * fortio_get_FILE(const fortio_type *fortio)        { return fortio->stream; }
int           fortio_get_record_size(const fortio_type *fortio) { return fortio->active_header; }
bool          fortio_get_endian_flip(const fortio_type *fortio) { return fortio->endian_flip_header; }
void          fortio_rewind(const fortio_type *fortio)          { rewind(fortio->stream); }
const char  * fortio_filename_ref(const fortio_type * fortio)   { return (const char *) fortio->filename; }
