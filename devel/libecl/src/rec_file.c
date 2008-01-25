#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <stdarg.h>
#include <rec_file.h>


struct rec_file_struct {
  int       rec_size;
  int       N_field;
  int       size;
  int       active_rec;
  int       bytes_written , bytes_read;
  long int *size_list;
  long int  header_offset , active_offset;
  char     *filename;
  FILE     *stream;
};

#define min(a,b) ((a) < (b) ? (a) : (b))

static const long int rec_file_id = 1234567890L;

static bool file_exists(const char *filename) {
  FILE *stream = fopen(filename , "r");
  bool ex;
  if (stream == NULL) {
    if (errno == ENOENT)
      ex = false;
  } else {
    fclose(stream);
    ex = true;
  }
  return ex;
}


static void assert_rec_file_stream(const char *filename, FILE *stream) {
  long int  file_rec_file_id;
  fread(&file_rec_file_id , sizeof file_rec_file_id  , 1 , stream);
  if (file_rec_file_id != rec_file_id) {
    fprintf(stderr,"rec_file_id not found in file:%s - aborting \n",filename);
    abort();
  }
}


rec_file_type * rec_file_open(const char * filename) {
  FILE *stream = fopen(filename , "r+");
  fseek(stream , 0L , SEEK_SET);
  assert_rec_file_stream(filename , stream);
  {
    rec_file_type *rec_file = malloc(sizeof *rec_file);
    rec_file->stream   = stream;
    rec_file->filename = malloc(strlen(filename) + 1);
    strcpy(rec_file->filename , filename);

    fread(&rec_file->size    , sizeof rec_file->size    , 1 , stream);
    fread(&rec_file->N_field , sizeof rec_file->N_field , 1 , stream);
    rec_file->size_list = calloc(rec_file->N_field , sizeof *rec_file->size_list);
    fread(rec_file->size_list , sizeof *rec_file->size_list , rec_file->N_field , stream);
    {
      int i;
      rec_file->rec_size = 0;
      for (i=0; i < rec_file->N_field; i++) 
	rec_file->rec_size += rec_file->size_list[i];
    }
    rec_file->header_offset = ftell(rec_file->stream);
    return rec_file;
  }
}


void rec_file_read_record(rec_file_type *rec_file, int rec_nr , ...) {
  rec_file_init_read(rec_file , rec_nr);
  {
    va_list ap;
    char *buffer;
    int i;

    va_start(ap , rec_file->N_field);
    for (i =0; i < rec_file->N_field; i++) {
      buffer = va_arg(ap , char *);
      rec_file_read(rec_file , buffer , rec_file->size_list[i]);
    }
    va_end(ap);
  }
  rec_file_complete_read(rec_file);
}


void rec_file_write_record(rec_file_type *rec_file, int rec_nr , ...) {
  rec_file_init_write(rec_file , rec_nr);
  {
    va_list ap;
    char *buffer;
    int i;

    va_start(ap , rec_file->N_field);
    for (i =0; i < rec_file->N_field; i++) {
      buffer = va_arg(ap , char *);
      rec_file_write(rec_file , buffer , rec_file->size_list[i]);
    }
    va_end(ap);

  }
  rec_file_complete_write(rec_file);
}


bool rec_file_init_read(rec_file_type * rec_file , int rec_nr) {
  if (rec_nr < rec_file->size) {
    bool active;
    fseek(rec_file->stream , rec_file->header_offset + rec_nr * (rec_file->rec_size + sizeof active) , SEEK_SET);
    rec_file->active_offset = ftell(rec_file->stream);
    fread(&active , sizeof active , 1 , rec_file->stream);
    /*if (!active) {
      fprintf(stderr,"Attempt to read record:%d which is empty - aborting \n",rec_nr);
      abort();
      }
    */
    rec_file->active_rec = rec_nr;
    rec_file->bytes_read = 0;
    return active;
  } else {
    fprintf(stderr,"Attempt to read record:%d in file:%s with only:%d records - aborting \n",rec_nr , rec_file->filename , rec_file->size);
    abort();
  }
}


static void rec_file_grow(rec_file_type *rec_file , int rec_nr) {
  fseek(rec_file->stream , rec_file->header_offset + rec_file->size * (rec_file->rec_size + sizeof(bool)) , SEEK_SET);
  {
    const int new_rec = rec_nr - rec_file->size + 1;
    const bool active = false;
    char *buffer      = malloc(rec_file->rec_size);
    int irec;
    
    printf("Skal legge til:%d nye records rec_size:%d \n",new_rec,rec_file->rec_size);
    
    for (irec = 0; irec < new_rec; irec++) {
      fwrite(&active , sizeof active , 1                  , rec_file->stream);
      fwrite(buffer  , 1             , rec_file->rec_size , rec_file->stream);
    }
    free(buffer);
    
    rec_file->size += new_rec;
    fseek(rec_file->stream , sizeof rec_file_id  , SEEK_SET);
    fwrite(&rec_file->size , sizeof rec_file->size ,1 , rec_file->stream);
  }
}


void rec_file_init_write(rec_file_type * rec_file , int rec_nr) {
  if (rec_nr < rec_file->size) {
    bool active;
    fseek(rec_file->stream , rec_file->header_offset + rec_nr * (rec_file->rec_size + sizeof active) , SEEK_SET);
    rec_file->active_offset = ftell(rec_file->stream);
    fseek(rec_file->stream , sizeof(bool) , SEEK_CUR);
    rec_file->active_rec = rec_nr;
    rec_file->bytes_written = 0;
  } else {
    rec_file_grow(rec_file , rec_nr);
    rec_file_init_write(rec_file , rec_nr);
  }
}




void rec_file_write(rec_file_type * rec_file , void *ptr , int byte_size) {
  int write_byte = min(byte_size , rec_file->rec_size - rec_file->bytes_written);
  if (fwrite(ptr , 1 , write_byte , rec_file->stream) != write_byte) {
    fprintf(stderr,"rec_file_write() failed when writing to record:%d in file:%s - aborting \n",rec_file->active_rec , rec_file->filename);
    abort();
  }
  rec_file->bytes_written += write_byte;
  if (write_byte < byte_size) {
    fprintf(stderr,"rec_file_write() attempt to write beyond end of record in rec_file_write(). Record:%d  file:%s - aborting\n",rec_file->active_rec , rec_file->filename);
    abort();
  }
}


void rec_file_read(rec_file_type * rec_file , void *ptr , int byte_size) {
  int read_byte = min(byte_size , rec_file->rec_size - rec_file->bytes_read);
  if (fread(ptr , 1 , read_byte , rec_file->stream) != read_byte) {
    fprintf(stderr,"rec_file_read() failed when trying read to record:%d in file:%s - aborting \n",rec_file->active_rec , rec_file->filename);
    abort();
  }
  rec_file->bytes_read += read_byte;
  if (read_byte < byte_size) {
    fprintf(stderr,"rec_file_read() attempt to read beyond end of record in rec_file_read(). Record:%d  file:%s - aborting\n",rec_file->active_rec , rec_file->filename);
    abort();
  }
}


void rec_file_complete_write(const rec_file_type *rec_file) {
  const bool active = true;
  const int bytes_written = ftell(rec_file->stream) - rec_file->active_offset  - sizeof active;
  
  if (bytes_written > rec_file->rec_size) {
    fprintf(stderr,"Have written %d bytes to record:%d in file:%s  - this exceeds the record size of %d bytes - aborting \n",
	    bytes_written , rec_file->active_rec , rec_file->filename , rec_file->rec_size);
    fprintf(stderr,"Warning: the file is quite probably corrupted \n");
    abort();
  }
  fseek(rec_file->stream , rec_file->active_offset , SEEK_SET);
  fwrite(&active , sizeof active , 1 , rec_file->stream);
}


void rec_file_complete_read(const rec_file_type *rec_file) {
  const bool active = true;
  const int bytes_written = ftell(rec_file->stream) - rec_file->active_offset  - sizeof active;
  
  if (bytes_written > rec_file->rec_size) {
    fprintf(stderr,"Have read %d bytes from record:%d in file:%s  - this exceeds the record size of %d bytes - aborting \n",
	    bytes_written , rec_file->active_rec , rec_file->filename , rec_file->rec_size);
    abort();
  }
  
}

void rec_file_close(rec_file_type *rec_file) {
  fclose(rec_file->stream);
  free(rec_file->filename);
  free(rec_file->size_list);
  free(rec_file);
}


void rec_file_init(const char *filename , int N_field , const long int *size_list) {
  if (file_exists(filename)) {
    int       file_N_field,size;
    long int *file_size_list;
    FILE     *stream = fopen(filename , "r");
    
    assert_rec_file_stream(filename , stream);
    fread(&size             , sizeof size              , 1 , stream);
    fread(&file_N_field     , sizeof file_N_field      , 1 , stream);
    
    if (file_N_field == N_field) {
      int i;
      bool eq = true;
      file_size_list = calloc(N_field , sizeof *file_size_list);
      fread(file_size_list , sizeof *file_size_list , N_field , stream);
      for (i=0; i < N_field; i++) 
	eq = eq & (file_size_list[i] == size_list[i]);

      free(file_size_list);
      fclose(stream);
      if (!eq) {
	fprintf(stderr,"Size list mismatch in existing record based file:%s - aborting \n", filename);
	abort();
      }
    } else {
      fclose(stream);
      fprintf(stderr,"Header mismatch in existing file:%s - can not be initialized as record based file.\n",filename);
      abort();
    }
  } else {
    FILE *stream = fopen(filename , "w");
    int size = 0;
    fwrite(&rec_file_id , sizeof rec_file_id  , 1 , stream);
    fwrite(&size        , sizeof size         , 1 , stream);
    fwrite(&N_field     , sizeof N_field      , 1 , stream);
    fwrite(size_list    , sizeof *size_list , N_field , stream);
    fclose(stream);
  }
}






