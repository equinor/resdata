#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <hash.h>
#include <rms_type.h>
#include <rms_tagkey.h>
#include <rms_util.h>

static const char * rms_array_string      = "array";


struct rms_tagkey_struct {
  int                  size;
  int                  sizeof_ctype;
  int                  data_size;
  int                  alloc_size;
  rms_type_enum        rms_type;
  char                *name;
  char                *data;
};



/*****************************************************************/


			    
/*****************************************************************/


void rms_free_tagkey_(void *_tagkey) {
  rms_tagkey_type * tagkey = (rms_tagkey_type *) _tagkey;
  rms_free_tagkey(tagkey);
}

static void rms_alloc_tagkey_data(rms_tagkey_type *tagkey) {
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




static const rms_tagkey_type * rms_tagkey_copyc(const rms_tagkey_type *tagkey) {
  rms_tagkey_type *new_tagkey = rms_alloc_empty_tagkey();
  
  new_tagkey->alloc_size   = 0;
  new_tagkey->size         = tagkey->size;
  new_tagkey->sizeof_ctype = tagkey->sizeof_ctype;
  new_tagkey->data_size    = tagkey->data_size;
  new_tagkey->rms_type     = tagkey->rms_type;
  new_tagkey->data         = NULL;

  rms_alloc_tagkey_data(new_tagkey);    
  memcpy(new_tagkey->data , tagkey->data , tagkey->data_size);
  new_tagkey->name = malloc(strlen(tagkey->name) + 1);  
  strcpy(new_tagkey->name , tagkey->name);
  return new_tagkey;
}


const void * rms_tagkey_copyc_(const void * _tagkey) {
  const  rms_tagkey_type * tagkey = (const rms_tagkey_type *) _tagkey;
  return rms_tagkey_copyc(tagkey);
}


static void rms_set_tagkey_data_size(rms_tagkey_type *tagkey , FILE *stream) {

  if (tagkey->rms_type == rms_char_type) {
    const long int init_pos = ftell(stream);
    int i;
    for (i=0; i < tagkey->size; i++)
      rms_fskip_string(stream);
    tagkey->data_size = ftell(stream) - init_pos;
    fseek(stream , init_pos , SEEK_SET);
  } else
    tagkey->data_size = tagkey->size * tagkey->sizeof_ctype;  
}







static void rms_fread_tagkey_data(rms_tagkey_type *tagkey , FILE *stream) {
  if (tagkey->alloc_size < tagkey->data_size) {
    fprintf(stderr,"%s: fatal error buffer to small - aborting \n",__func__);
    abort();
  }

  int bytes_read = fread(tagkey->data , 1 , tagkey->data_size ,  stream);
  if (bytes_read != tagkey->data_size) {
    fprintf(stderr,"%s: failed to read %d bytes - premature EOF? \n",__func__ , tagkey->data_size);
    fprintf(stderr,"%s: tagkey:  %s \n",__func__ , tagkey->name);
    abort();
  }
}


static void rms_fskip_tagkey_data(rms_tagkey_type *tagkey , FILE *stream) {
  rms_set_tagkey_data_size(tagkey , stream);
  fseek(stream , tagkey->data_size , SEEK_CUR);
}


static void rms_fread_tagkey_header(rms_tagkey_type *tagkey , FILE *stream, hash_type *type_map) {
  bool is_array;
  char type_string[7];
  
  rms_fread_string(stream , type_string , 7);
  if (strcmp(type_string , rms_array_string) == 0) {
    is_array = true;
    rms_fread_string(stream , type_string , 7);
  } else
    is_array = false;
  
  {
    __rms_type * rms_t   = hash_get(type_map , type_string);
    tagkey->rms_type     = rms_t->rms_type;
    tagkey->sizeof_ctype = rms_t->sizeof_ctype;
  }

  tagkey->name = realloc(tagkey->name , rms_fread_strlen(stream) + 1);
  rms_fread_string(stream , tagkey->name , 0);
  if (is_array)
    fread(&tagkey->size , 1 , sizeof tagkey->size, stream);
  else
    tagkey->size = 1;
  rms_set_tagkey_data_size(tagkey , stream);
}



static void rms_fread_realloc_tagkey(rms_tagkey_type *tagkey , FILE *stream , hash_type *type_map) {

  rms_fread_tagkey_header(tagkey , stream , type_map);
  rms_alloc_tagkey_data(tagkey);
  rms_fread_tagkey_data(tagkey , stream);

}


static rms_tagkey_type * rms_fread_alloc_tagkey(FILE *stream , hash_type * type_map) {
  rms_tagkey_type *tagkey = rms_alloc_empty_tagkey();
  rms_fread_realloc_tagkey(tagkey , stream , type_map );
  return tagkey;
}


static void rms_fskip_tagkey(FILE *stream , hash_type * type_map) {
  rms_tagkey_type *tagkey = rms_alloc_empty_tagkey();
  rms_fread_tagkey_header(tagkey , stream , type_map);
  rms_fskip_tagkey_data(tagkey , stream);
  rms_free_tagkey(tagkey);
}



const char * rms_tagkey_get_name(const rms_tagkey_type *tagkey) {
  return tagkey->name;
}


void rms_tagkey_load(rms_tagkey_type *tagkey , FILE *stream, hash_type *type_map) {
  rms_fread_realloc_tagkey(tagkey , stream , type_map);
}


rms_tagkey_type * rms_alloc_empty_tagkey() {
  rms_tagkey_type *tagkey = malloc(sizeof *tagkey);

  tagkey->alloc_size = 0;
  tagkey->name       = NULL;
  tagkey->data       = NULL;
  
  return tagkey;
}

void rms_free_tagkey(rms_tagkey_type *tagkey) {
  if (tagkey->name != NULL) free(tagkey->name);
  if (tagkey->data != NULL) free(tagkey->data);
  free(tagkey);
}

