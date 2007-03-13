#include <stdlib.h>
#include <stdio.h>
#include <rms_roff.h>
#include <string.h>
#include <stdbool.h>
#include <hash.h>
#include <list.h>
#include <list_node.h>


/*****************************************************************/
static const char * rms_roff_ascii_header    = "roff-asc";
static const char * rms_roff_binary_header   = "roff-bin";
static const char * rms_roff_eof_tag         = "eof";
static const char * rms_roff_starttag_string = "tag";
static const char * rms_roff_endtag_string   = "endtag";
static const char * rms_roff_array_string    = "array";
static const char * rms_roff_comment1        = "ROFF file";
static const char * rms_roff_comment2        = "Creator: RMS - Reservoir Modelling System, version 7.5.2";



enum rms_type_enum_def {rms_char_type , rms_float_type , rms_double_type , rms_bool_type , rms_byte_type , rms_int_type};


struct rms_roff_file_struct {
  const char * filename;
  FILE       * stream;
  bool         endian_flip;
  bool         binary;
  hash_type  * type_map;
  list_type  * tag_list;
};


struct rms_roff_tagkey_struct {
  int                  size;
  int                  sizeof_ctype;
  int                  data_size;
  int                  alloc_size;
  rms_roff_type_enum   rms_type;
  char                *name;
  char                *data;
};


struct rms_roff_tag_struct {
  const char *name;
  hash_type  *keys;
};


/* Interface: */
void rms_roff_free_tag(rms_roff_tag_type *);


/*****************************************************************/
/* A microscopic (purely internal) type object only used 
   for storing the hash type_map */
/*****************************************************************/

typedef struct {
  rms_roff_type_enum   rms_type;
  int                  sizeof_ctype;
} __rms_type;


void rms_type_free(void *rms_t) {
  free( (__rms_type *) rms_t);
}

__rms_type * rms_type_set(__rms_type *rms_t , rms_roff_type_enum rms_type , int sizeof_ctype) {
  rms_t->rms_type     = rms_type;
  rms_t->sizeof_ctype = sizeof_ctype;
  return rms_t;
}


__rms_type * rms_type_alloc(rms_roff_type_enum rms_type, int sizeof_ctype) {
  __rms_type *rms_t   = malloc(sizeof *rms_t);
  rms_type_set(rms_t , rms_type , sizeof_ctype);
  return rms_t;
}


const void * rms_type_copyc(const void *__rms_t) {
  const __rms_type *rms_t = (const __rms_type *) __rms_t;
  __rms_type *new_t = rms_type_alloc(rms_t->rms_type , rms_t->sizeof_ctype);
  return new_t;
}
			    


/*
  Thid translates from the RMS data layout to "fortan" data layout.

  RMS: k index is running fastest *AND* backwards.
  F90: i is running fastest, and k is running the 'normal' way.
  
*/
void rms_roff_write_fortran_data(void *_f90_data , const void * _rms_data, int sizeof_ctype , int nx, int ny , int nz) {
  char *f90_data       = (char *)       _f90_data;
  const char *rms_data = (const char *) _rms_data;
  int i,j,k,rms_index, f90_index;
  rms_index = -1;
  for (i=0; i < nx; i++) 
    for (j=0; j < ny; j++)
      for (k= (nz -1); k >= 0; k--) {
	rms_index += 1;
	f90_index  = i + j*nx + k*nx*ny;
	memcpy(&f90_data[f90_index * sizeof_ctype] , &rms_data[rms_index * sizeof_ctype] , sizeof_ctype);
      }
}



void rms_roff_read_fortran_data(const void *_f90_data , void * _rms_data, int sizeof_ctype , int nx, int ny , int nz) {
  const char *f90_data = (const char *) _f90_data;
  char *rms_data       = (char *)       _rms_data;
  int i,j,k,rms_index, f90_index;
  rms_index = -1;
  for (i=0; i < nx; i++) 
    for (j=0; j < ny; j++)
      for (k= (nz -1); k >= 0; k--) {
	rms_index += 1;
	f90_index = i + j*nx + k*nx*ny;
	memcpy(&rms_data[rms_index * sizeof_ctype] , &f90_data[f90_index * sizeof_ctype] , sizeof_ctype);
      }
}


/*****************************************************************/
/* Pure roff routines */

static void rms_roff_fread_data(rms_roff_file_type *roff , int byte_size , char * buffer) {
  int bytes_read = fread(buffer , byte_size , 1 , roff->stream);
  if (bytes_read != byte_size) {
    fprintf(stderr,"%s: fatal error when reading from file:%s premature EOF detected \n",__func__ , roff->filename);
    abort();
  }
}

static void rms_roff_fskip_string(const rms_roff_file_type *roff_file) {
  char c;
  bool cont = true;
  while (cont) {
    fread(&c , 1 , 1 , roff_file->stream);
    if (c == 0)
      cont = false;
  } 
}


static int rms_roff_fread_strlen(const rms_roff_file_type *roff_file) {
  long int init_pos = ftell(roff_file->stream);
  int len;
  rms_roff_fskip_string(roff_file);
  len = ftell(roff_file->stream) - init_pos;
  fseek(roff_file->stream , init_pos , SEEK_SET);
  return len;
}


/*
  max_length *includes* the trailing \0.
*/
static bool rms_roff_fread_string(const rms_roff_file_type *roff , char *string , int max_length) {
  bool read_ok = true;
  bool cont    = true;
  long int init_pos = ftell(roff->stream);
  int pos = 0;
  while (cont) {
    fread(&string[pos] , sizeof *string , 1 , roff->stream);
    if (string[pos] == 0) {
      read_ok = true;
      cont = false;
    } else {
      pos++;
      if (max_length > 0) {
	if (pos == max_length) {
	  read_ok = false;
	  fseek(roff->stream , init_pos , SEEK_SET);
	  cont = false;
	}
      }
    } 
  } 
  
  return read_ok;
}



static bool rms_roff_binary(const rms_roff_file_type *roff_file) {
  bool binary;
  char filetype[9];
  rms_roff_fread_string(roff_file , filetype , 9);

  if (strncmp(filetype , rms_roff_binary_header , 8) == 0)
    binary = true;
  else if (strncmp(filetype , rms_roff_ascii_header , 8) == 0)
    binary = false;
  else {
    fprintf(stderr,"%s: header : %8s not recognized in file: %s - aborting \n",__func__ , filetype , roff_file->filename);
    abort();
  }
  return binary;
}


/*
  Check if the roff object is positioned *at* an endtag.
  If it is at an entag the stream object is positioned at 
  the end of of the endtag, otherwise it is repositioned at
  the initial position.
*/

/*static*/
bool rms_roff_at_endtag(const rms_roff_file_type *roff) {
  
  const int init_pos = ftell(roff->stream);
  bool at_endtag;
  char tag[7];
  if (rms_roff_fread_string(roff , tag , 7)) {
    if (strcmp(tag , rms_roff_endtag_string) == 0)
      at_endtag = true;
    else
      at_endtag = false;
  } else
    at_endtag = false;
  
  if (!at_endtag)
    fseek(roff->stream , init_pos , SEEK_SET);
  return at_endtag;
}
    





static rms_roff_file_type * rms_roff_alloc_file(const char *filename) {
  rms_roff_file_type *roff_file = malloc(sizeof *roff_file);
  {
    char *tmp_name = malloc(strlen(filename) + 1);
    strcpy(tmp_name , filename);
    roff_file->filename = tmp_name;
  }
  roff_file->endian_flip = false;
  roff_file->type_map  = hash_alloc(10);
  roff_file->tag_list  = list_alloc();

  {
    __rms_type *rms_t = rms_type_alloc(1,1);

    hash_insert_copy(roff_file->type_map , "byte"   , rms_type_set(rms_t , rms_byte_type ,    1) , rms_type_copyc , rms_type_free);
    hash_insert_copy(roff_file->type_map , "bool"   , rms_type_set(rms_t , rms_bool_type,     1) , rms_type_copyc , rms_type_free);
    hash_insert_copy(roff_file->type_map , "int"    , rms_type_set(rms_t , rms_int_type ,     4) , rms_type_copyc , rms_type_free);
    hash_insert_copy(roff_file->type_map , "float"  , rms_type_set(rms_t , rms_float_type  ,  4) , rms_type_copyc , rms_type_free);
    hash_insert_copy(roff_file->type_map , "double" , rms_type_set(rms_t , rms_double_type ,  8) , rms_type_copyc , rms_type_free);
    hash_insert_copy(roff_file->type_map , "char"   , rms_type_set(rms_t , rms_char_type   , -1) , rms_type_copyc , rms_type_free);
    
    rms_type_free(rms_t);
  }
  
  return roff_file;
}


static void rms_roff_init_existing_file(rms_roff_file_type * roff_file) {
  if ( (roff_file->stream = fopen(roff_file->filename , "r")) == NULL) {
    fprintf(stderr,"%s: failed to open: %s - aborting \n",__func__ , roff_file->filename);
    abort();
  }
  
  roff_file->binary = rms_roff_binary(roff_file);
  if (!roff_file->binary) {
    fprintf(stderr,"%s only binary files implemented - aborting \n",__func__);
    abort();
  }
  /* Skipping two comment lines ... */
  rms_roff_fskip_string(roff_file);
  rms_roff_fskip_string(roff_file);
}

static void rms_roff_init_new_file(rms_roff_file_type * roff_file , bool binary) {
  
}



rms_roff_file_type * rms_roff_open(const char *filename , bool new_file , bool create_binary) {
  rms_roff_file_type * roff_file = rms_roff_alloc_file(filename);
  if (new_file)
    rms_roff_init_new_file(roff_file , create_binary);
  else 
    rms_roff_init_existing_file(roff_file);
  return roff_file;
}


void rms_roff_close(rms_roff_file_type * roff_file) {
  hash_free(roff_file->type_map);
  {
    list_node_type    *tag_node , *next_tag_node;
    list_type         *tag_list = roff_file->tag_list;
    
    tag_node = list_get_head(tag_list);
    while (tag_node) {
      next_tag_node = list_node_get_next(tag_node);
      rms_roff_free_tag( list_node_value_ptr(tag_node) );
      tag_node = next_tag_node;
    }
  }

  list_free(roff_file->tag_list);
  free( (char *) roff_file->filename);
  fclose(roff_file->stream); 
  free(roff_file);
}


/******************************************************************/
/** Starting tagkey routines                                     **/
/******************************************************************/


rms_roff_tagkey_type * rms_roff_alloc_empty_tagkey() {
  rms_roff_tagkey_type *tagkey = malloc(sizeof *tagkey);

  tagkey->alloc_size = 0;
  tagkey->name       = NULL;
  tagkey->data       = NULL;
  
  return tagkey;
}



void rms_roff_free_tagkey(rms_roff_tagkey_type *tagkey) {
  if (tagkey->name != NULL) free(tagkey->name);
  if (tagkey->data != NULL) free(tagkey->data);
  free(tagkey);
}


void rms_roff_free_tagkey_(void *_tagkey) {
  rms_roff_tagkey_type * tagkey = (rms_roff_tagkey_type *) _tagkey;
  rms_roff_free_tagkey(tagkey);
}


/* stream er godt nok */
static void rms_roff_set_tagkey_data_size(const rms_roff_file_type *roff_file , rms_roff_tagkey_type *tagkey) {

  if (tagkey->rms_type == rms_char_type) {
    const long int init_pos = ftell(roff_file->stream);
    int i;
    for (i=0; i < tagkey->size; i++)
      rms_roff_fskip_string(roff_file);
    tagkey->data_size = ftell(roff_file->stream) - init_pos;
    fseek(roff_file->stream , init_pos , SEEK_SET);
  } else
    tagkey->data_size = tagkey->size * tagkey->sizeof_ctype;  
}


static void rms_roff_alloc_tagkey_data(rms_roff_tagkey_type *tagkey) {
  if (tagkey->data_size > tagkey->alloc_size) {
    void *tmp = realloc(tagkey->data , tagkey->data_size);
    if (tmp == NULL) {
      fprintf(stderr,"%s: failed to allocate: %d bytes of storage - aborting \n",__func__ , tagkey->data_size);
      abort();
    } 
    tagkey->data       = tmp;
    tagkey->alloc_size = tagkey->data_size;
  }
}


static const rms_roff_tagkey_type * rms_roff_tagkey_copyc(const rms_roff_tagkey_type *tagkey) {
  rms_roff_tagkey_type *new_tagkey = rms_roff_alloc_empty_tagkey();
  
  new_tagkey->alloc_size   = 0;
  new_tagkey->size         = tagkey->size;
  new_tagkey->sizeof_ctype = tagkey->sizeof_ctype;
  new_tagkey->data_size    = tagkey->data_size;
  new_tagkey->rms_type     = tagkey->rms_type;
  new_tagkey->data         = NULL;

  rms_roff_alloc_tagkey_data(new_tagkey);               memcpy(new_tagkey->data , tagkey->data , tagkey->data_size);
  new_tagkey->name = malloc(strlen(tagkey->name) + 1);  strcpy(new_tagkey->name , tagkey->name);
  return new_tagkey;
}

static const void * rms_roff_tagkey_copyc_(const void * _tagkey) {
  const rms_roff_tagkey_type * tagkey = (const rms_roff_tagkey_type *) _tagkey;

  return rms_roff_tagkey_copyc(tagkey);
}


static void rms_roff_fread_tagkey_data(const rms_roff_file_type *roff_file , rms_roff_tagkey_type *tagkey) {
  if (tagkey->alloc_size < tagkey->data_size) {
    fprintf(stderr,"%s: fatal error buffer to small - aborting \n",__func__);
    abort();
  }

  int bytes_read = fread(tagkey->data , 1 , tagkey->data_size ,  roff_file->stream);
  if (bytes_read != tagkey->data_size) {
    fprintf(stderr,"%s: failed to read %d bytes - premature EOF? \n",__func__ , tagkey->data_size);
    fprintf(stderr,"%s: tagkey:  %s \n",__func__ , tagkey->name);
    abort();
  }
}


static void rms_roff_fskip_tagkey_data(const rms_roff_file_type *roff_file , rms_roff_tagkey_type *tagkey) {
  rms_roff_set_tagkey_data_size(roff_file , tagkey);
  fseek(roff_file->stream , tagkey->data_size , SEEK_CUR);
}


static void rms_roff_fread_tagkey_header(const rms_roff_file_type *roff_file , rms_roff_tagkey_type *tagkey) {
  bool is_array;
  char type_string[7];
  
  rms_roff_fread_string(roff_file , type_string , 7);
  if (strcmp(type_string , rms_roff_array_string) == 0) {
    is_array = true;
    rms_roff_fread_string(roff_file , type_string , 7);
  } else
    is_array = false;
  
  {
    __rms_type * rms_t   = hash_get(roff_file->type_map , type_string);
    tagkey->rms_type     = rms_t->rms_type;
    tagkey->sizeof_ctype = rms_t->sizeof_ctype;
  }

  tagkey->name = realloc(tagkey->name , rms_roff_fread_strlen(roff_file) + 1);
  rms_roff_fread_string(roff_file , tagkey->name , 0);
  if (is_array)
    fread(&tagkey->size , 1 , sizeof tagkey->size, roff_file->stream);
  else
    tagkey->size = 1;
  rms_roff_set_tagkey_data_size(roff_file , tagkey);
}


static void rms_roff_fread_realloc_tagkey(const rms_roff_file_type *roff_file , rms_roff_tagkey_type *tagkey) {

  rms_roff_fread_tagkey_header(roff_file , tagkey);
  rms_roff_alloc_tagkey_data(tagkey);
  rms_roff_fread_tagkey_data(roff_file , tagkey);

}


static rms_roff_tagkey_type * rms_roff_fread_alloc_tagkey(const rms_roff_file_type *roff_file) {
  rms_roff_tagkey_type *tagkey = rms_roff_alloc_empty_tagkey();
  rms_roff_fread_realloc_tagkey(roff_file , tagkey);
  return tagkey;
}

static void rms_roff_fskip_tagkey(const rms_roff_file_type *roff_file) {
  rms_roff_tagkey_type *tagkey = rms_roff_alloc_empty_tagkey();
  rms_roff_fread_tagkey_header(roff_file , tagkey);
  rms_roff_fskip_tagkey_data(roff_file , tagkey);
  rms_roff_free_tagkey(tagkey);
}

const char * rms_roff_get_tagkey_name(const rms_roff_tagkey_type *tagkey) {
  return tagkey->name;
}

void rms_roff_load_tagkey(const rms_roff_file_type *roff_file , rms_roff_tagkey_type *tagkey) {
  rms_roff_fread_realloc_tagkey(roff_file , tagkey);
}


/*****************************************************************/
/* tag routines */


rms_roff_tag_type * rms_roff_alloc_empty_tag() {
  rms_roff_tag_type *tag = malloc(sizeof *tag);
  tag->name = NULL;
  tag->keys = hash_alloc(10);
  return tag;
}

void rms_roff_free_tag(rms_roff_tag_type *tag) {
  free( (char *) tag->name);
  hash_free(tag->keys);
  free(tag);
}

  
const char * rms_roff_get_tag_name(const rms_roff_tag_type *tag) {
  return tag->name;
}



/*static*/ void rms_roff_fread_tag_header(const rms_roff_file_type *roff_file , rms_roff_tag_type *tag , bool *eof_tag) {
  char *buffer;
  *eof_tag = false;
  buffer = malloc(4);
  if (rms_roff_fread_string(roff_file , buffer , 4)) {
    if (strcmp(buffer , rms_roff_starttag_string) == 0) {
      /* OK */
      {
	char *tmp = malloc(rms_roff_fread_strlen(roff_file) + 1);
	rms_roff_fread_string(roff_file , tmp , 0);
	tag->name = tmp;
	if (strcmp(tag->name , rms_roff_eof_tag) == 0)
	  *eof_tag = true;
      }
    } else {
      fprintf(stderr,"%s: not at tag - header aborting \n",__func__);
      abort();
    }   
  } else {
    fprintf(stderr,"%s: not at tag - header aborting \n",__func__);
    abort();
  }
  free(buffer);
}


static void rms_roff_tag_add_tagkey(const rms_roff_tag_type *tag , const rms_roff_tagkey_type *tagkey) {
  hash_insert_copy(tag->keys , tagkey->name , tagkey , rms_roff_tagkey_copyc_ , rms_roff_free_tagkey_);
}


void rms_roff_fread_tag(const rms_roff_file_type * roff_file , rms_roff_tag_type *tag, bool *at_eof) {
  rms_roff_fread_tag_header(roff_file , tag , at_eof);
  if (!*at_eof) {
    rms_roff_tagkey_type *tagkey = rms_roff_alloc_empty_tagkey();
    while (! rms_roff_at_endtag(roff_file)) {
      rms_roff_load_tagkey(roff_file , tagkey);
      rms_roff_tag_add_tagkey(tag , tagkey);
      printf("Have loaded: %s/%s \n",rms_roff_get_tag_name(tag) , rms_roff_get_tagkey_name(tagkey));
    }
    rms_roff_free_tagkey(tagkey);
  }
}

rms_roff_tag_type * rms_roff_fread_alloc_tag(const rms_roff_file_type *roff_file , bool *at_eof) {
  rms_roff_tag_type *tag = rms_roff_alloc_empty_tag();
  rms_roff_fread_tag(roff_file , tag , at_eof);
  return tag;
}


void rms_roff_load_file(rms_roff_file_type *roff_file) {
  bool eof_tag = false;
  
  while (!eof_tag) {
    rms_roff_tag_type * tag = rms_roff_fread_alloc_tag(roff_file ,  &eof_tag);
    if (!eof_tag)
      list_append_ref(roff_file->tag_list , tag);
    else
      rms_roff_free_tag(tag);
  }
}


/*****************************************************************/
/* Old hack version: */
  

void rms_roff_load(const char *filename , const char *param_name , float *param) {
  const int offset = 327 + strlen(param_name);
  int n_read;
  int size;
  FILE *stream     = fopen(filename , "r");
  
  fseek(stream , offset , SEEK_SET);
  fread(&size  , 1 , sizeof size , stream);
  n_read = fread(param , sizeof *param , size , stream);
  
  fclose(stream);
  if (n_read != size) {
    fprintf(stderr,"%s: wanted:%d elements - only read:%d - aborting \n",__func__, size , n_read);
    abort();
  }
}



				  
