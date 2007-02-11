#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fortio.h>

extern int errno;

#define FLIP32(var) (((var >> 24) & 0x000000ff) | \
		     ((var >>  8) & 0x0000ff00) | \
		     ((var <<  8) & 0x00ff0000) | \
		     ((var << 24) & 0xff000000))

struct fortio_struct {
  FILE    *stream;
  char    *filename;
  char    *mode;
  int      active_header;
  int      rec_nr;
  bool     endian_flip_header;
};


fortio_type *fortio_open(const char *filename , const char *mode, bool endian_flip_header) {
  fortio_type *fortio;
  fortio = malloc(sizeof *fortio);
  fortio->filename = malloc(strlen(filename) + 1);
  fortio->mode     = malloc(strlen(mode)     + 1);
  strcpy(fortio->filename , filename);
  strcpy(fortio->mode     , mode);
  

  fortio->active_header      = 0;
  fortio->rec_nr             = 0; 
  fortio->endian_flip_header = endian_flip_header;
  fortio->stream             = fopen(fortio->filename , fortio->mode);
  if (fortio->stream == NULL) {
    fprintf(stderr,"fortio_open() failed to open:%s with mode:%s - aborting \n", fortio->filename , fortio->mode);
    fprintf(stderr,"%d:%s\n",errno, strerror(errno));
    abort();
  }
  return fortio;
}



void fortio_close(fortio_type *fortio) {
  fclose(fortio->stream);
  free(fortio->filename);
  free(fortio->mode);
  free(fortio);
}




int fortio_init_read(fortio_type *fortio) {
  int elm_read;
  elm_read = fread(&fortio->active_header , sizeof(fortio->active_header) , 1 , fortio->stream);
  if (elm_read == 1) {
    if (fortio->endian_flip_header)
      fortio->active_header = FLIP32(fortio->active_header);
    
    fortio->rec_nr++;
    return fortio->active_header;
  } else 
    return -1;
}

void fortio_complete_read(fortio_type *fortio) {
  int trailer;
  fread(&trailer , sizeof(fortio->active_header) , 1 , fortio->stream);
  if (fortio->endian_flip_header)
    trailer = FLIP32(trailer);
  
  if (trailer != fortio->active_header) {
    fprintf(stderr,"\nHeader: %d   Trailer: %d \n",fortio->active_header , trailer);
    fprintf(stderr,"Fatal error reading record:%d in file: %s - aborting \n",fortio->rec_nr , fortio->filename);
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


/*****************************************************************/

void  fortio_init_write(fortio_type *fortio , int record_size) {
  int file_header;
  fortio->active_header = record_size;
  file_header = fortio->active_header;
  if (fortio->endian_flip_header)
    file_header = FLIP32(file_header);
  
  fwrite(&file_header , sizeof(fortio->active_header) , 1 , fortio->stream);
  fortio->rec_nr++;
}

void fortio_complete_write(fortio_type *fortio) {
  int file_header = fortio->active_header;
  if (fortio->endian_flip_header)
    file_header = FLIP32(file_header);

  fwrite(&file_header, sizeof(fortio->active_header) , 1 , fortio->stream);
  fortio->active_header = 0;
}


void fortio_write_record(fortio_type *fortio, const char *buffer , int record_size) {
  fortio_init_write(fortio , record_size);
  fwrite(buffer , 1 , record_size , fortio->stream);
  fortio_complete_write(fortio);
}

/*****************************************************************/

FILE        * fortio_get_FILE(const fortio_type *fortio)        { return fortio->stream; }
int           fortio_get_record_size(const fortio_type *fortio) { return fortio->active_header; }
