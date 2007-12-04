#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fortio.h>
#include <util.h>

extern int errno;

/*
#define FLIP32(var) (((var >> 24) & 0x000000ff) | \
		     ((var >>  8) & 0x0000ff00) | \
		     ((var <<  8) & 0x00ff0000) | \
		     ((var << 24) & 0xff000000))
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
