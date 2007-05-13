#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <util.h>
#include <time.h>
#include <hash.h>
#include <rms_type.h>
#include <rms_tagkey.h>
#include <rms_util.h>

static const char * rms_array_string      = "array";

static const char rms_type_names[6][7] = {{"char\0"},
					  {"float\0"},
					  {"double\0"},
					  {"bool\0"},
					  {"byte\0"},
					  {"int\0"}};

static const int rms_type_size[6] = {1 , 4 , 8 , 1 , 1 , 4}; /* */


struct rms_tagkey_struct {
  int                  size;
  int                  sizeof_ctype;
  int                  data_size;
  int                  alloc_size;
  rms_type_enum        rms_type;
  char                *name;
  void                *data;
  bool                 endian_convert;
};



/*****************************************************************/


			    
/*****************************************************************/


void rms_free_tagkey_(void *_tagkey) {
  rms_tagkey_type * tagkey = (rms_tagkey_type *) _tagkey;
  rms_free_tagkey(tagkey);
}

static void rms_tagkey_alloc_data(rms_tagkey_type *tagkey) {
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
  rms_tagkey_type *new_tagkey = rms_tagkey_alloc_empty(tagkey->endian_convert);
  
  new_tagkey->alloc_size     = 0;
  new_tagkey->size           = tagkey->size;
  new_tagkey->sizeof_ctype   = tagkey->sizeof_ctype;
  new_tagkey->data_size      = tagkey->data_size;
  new_tagkey->rms_type       = tagkey->rms_type;
  new_tagkey->data           = NULL;

  rms_tagkey_alloc_data(new_tagkey);    
  memcpy(new_tagkey->data , tagkey->data , tagkey->data_size);
  new_tagkey->name = malloc(strlen(tagkey->name) + 1);  
  strcpy(new_tagkey->name , tagkey->name);
  return new_tagkey;
}


const void * rms_tagkey_copyc_(const void * _tagkey) {
  const  rms_tagkey_type * tagkey = (const rms_tagkey_type *) _tagkey;
  return rms_tagkey_copyc(tagkey);
}


static void rms_tagkey_set_data_size(rms_tagkey_type *tagkey , FILE *stream , int strlen) {

  if (tagkey->rms_type == rms_char_type) {
    if (stream != NULL) {
      const long int init_pos = ftell(stream);
      int i;
      for (i=0; i < tagkey->size; i++)
	rms_util_fskip_string(stream);
      tagkey->data_size = ftell(stream) - init_pos;
      fseek(stream , init_pos , SEEK_SET);
    } else 
      tagkey->data_size = strlen + 1;
  } else
    tagkey->data_size = tagkey->size * tagkey->sizeof_ctype;  
}



static void rms_tagkey_fread_data(rms_tagkey_type *tagkey , bool endian_convert , FILE *stream) {
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
  if (endian_convert) 
    if (tagkey->sizeof_ctype > 1)
      util_endian_flip_vector(tagkey->data , tagkey->sizeof_ctype , tagkey->size);
}


void rms_tagkey_set_data(rms_tagkey_type * tagkey , const void * data) {
  memcpy(tagkey->data , data , tagkey->data_size);
}


static void rms_fskip_tagkey_data(rms_tagkey_type *tagkey , FILE *stream) {
  rms_tagkey_set_data_size(tagkey , stream , -1);
  fseek(stream , tagkey->data_size , SEEK_CUR);
}


static void rms_fread_tagkey_header(rms_tagkey_type *tagkey , FILE *stream, hash_type *type_map) {
  bool is_array;
  char type_string[7];
  
  rms_util_fread_string(type_string , 7 , stream);
  if (strcmp(type_string , rms_array_string) == 0) {
    is_array = true;
    rms_util_fread_string(type_string , 7 , stream);
  } else
    is_array = false;
  
  {
    __rms_type * rms_t   = hash_get(type_map , type_string);
    tagkey->rms_type     = rms_t->rms_type;
    tagkey->sizeof_ctype = rms_t->sizeof_ctype;
  }

  tagkey->name = realloc(tagkey->name , rms_util_fread_strlen(stream) + 1);

  rms_util_fread_string(tagkey->name , 0 , stream);
  if (is_array)
    fread(&tagkey->size , 1 , sizeof tagkey->size, stream);
  else
    tagkey->size = 1;
  rms_tagkey_set_data_size(tagkey , stream , -1);
}



static void rms_fread_realloc_tagkey(rms_tagkey_type *tagkey , bool endian_convert , FILE *stream , hash_type *type_map) {

  rms_fread_tagkey_header(tagkey , stream , type_map);
  rms_tagkey_alloc_data(tagkey);
  rms_tagkey_fread_data(tagkey , endian_convert , stream);

}


static rms_tagkey_type * rms_fread_alloc_tagkey(bool endian_convert , FILE *stream , hash_type * type_map) {
  rms_tagkey_type *tagkey = rms_tagkey_alloc_empty(endian_convert);
  rms_fread_realloc_tagkey(tagkey , endian_convert , stream , type_map );
  return tagkey;
}


static void rms_fskip_tagkey(FILE *stream , hash_type * type_map) {
  rms_tagkey_type *tagkey = rms_tagkey_alloc_empty(false);
  rms_fread_tagkey_header(tagkey , stream , type_map);
  rms_fskip_tagkey_data(tagkey , stream);
  rms_free_tagkey(tagkey);
}


static void rms_tagkey_fwrite_data(const rms_tagkey_type * tagkey , FILE *stream) {
  int elm = fwrite(tagkey->data , 1 , tagkey->data_size , stream);
  if (elm != tagkey->data_size) {
    fprintf(stderr,"%s: failed to write %d bytes to file [tagkey:%s] - aborting \n",__func__ , tagkey->data_size , tagkey->name);
    abort();
  }
}


void rms_tagkey_fwrite(const rms_tagkey_type * tagkey , FILE *stream) {
  if (tagkey->size > 1)
    rms_util_fwrite_string("array" , stream);
  rms_util_fwrite_string(rms_type_names[tagkey->rms_type] , stream);
  rms_util_fwrite_string(tagkey->name , stream);
  if (tagkey->size > 1) {
    fwrite(&tagkey->size , sizeof tagkey->size , 1 , stream);
    rms_util_fwrite_newline(stream);
  }
  rms_tagkey_fwrite_data(tagkey , stream);
}


const char * rms_tagkey_get_name(const rms_tagkey_type *tagkey) {
  return tagkey->name;
}

void * rms_tagkey_get_data_ref(const rms_tagkey_type *tagkey) {
  return tagkey->data;
}


void rms_tagkey_load(rms_tagkey_type *tagkey , bool endian_convert , FILE *stream, hash_type *type_map) {
  rms_fread_realloc_tagkey(tagkey , endian_convert , stream , type_map);
}


bool rms_tagkey_char_eq(const rms_tagkey_type *tagkey , const char *keyvalue) {
  bool eq = false;
  if (tagkey->rms_type == rms_char_type) {
    if (strcmp(keyvalue , tagkey->data) == 0)
      eq = true;
  }
  return eq;
}


rms_tagkey_type * rms_tagkey_alloc_empty(bool endian_convert) {
  
  rms_tagkey_type *tagkey = malloc(sizeof *tagkey);
  tagkey->alloc_size 	  = 0;
  tagkey->name       	  = NULL;
  tagkey->data       	  = NULL;
  tagkey->endian_convert  = endian_convert;
  
  return tagkey;
}


static rms_tagkey_type * rms_tagkey_alloc_initialized(const char * name , int size , rms_type_enum rms_type , bool endian_convert) {
  rms_tagkey_type *tagkey = rms_tagkey_alloc_empty(endian_convert);
  tagkey->size         	  = size;
  tagkey->rms_type     	  = rms_type;
  tagkey->sizeof_ctype 	  = rms_type_size[rms_type];
  tagkey->data_size    	  = tagkey->size * tagkey->sizeof_ctype;
  tagkey->name         	  = util_alloc_string_copy(name);
  return tagkey;
}


rms_tagkey_type * rms_tagkey_alloc_complete(const char * name , int size , rms_type_enum rms_type , const void * data) {
  rms_tagkey_type * tag = rms_tagkey_alloc_initialized(name , size , rms_type , false);
  rms_tagkey_alloc_data(tag);
  rms_tagkey_set_data(tag , data);
  return tag;
}


void rms_free_tagkey(rms_tagkey_type *tagkey) {
  if (tagkey->name != NULL) free(tagkey->name);
  if (tagkey->data != NULL) free(tagkey->data);
  free(tagkey);
}


rms_tagkey_type * rms_tagkey_alloc_byteswap() {
  rms_tagkey_type *tagkey = rms_tagkey_alloc_initialized("byteswaptest" , 1 , rms_int_type , false);
  rms_tagkey_alloc_data(tagkey);
  ((int *) tagkey->data)[0] = 1;
  return tagkey;
}


rms_tagkey_type * rms_tagkey_alloc_filetype(const char * filetype) {
  rms_tagkey_type *tagkey = rms_tagkey_alloc_initialized("filetype" , 1 , rms_char_type , false);
  rms_tagkey_set_data_size(tagkey , NULL , strlen(filetype));
  rms_tagkey_alloc_data(tagkey);
  sprintf(tagkey->data , "%s" , filetype);
  return tagkey;
}


rms_tagkey_type * rms_tagkey_alloc_creationDate() {
  const int len = strlen("08/05/2007 08:31:39");
  struct tm ts;
  time_t now;
  rms_tagkey_type *tagkey = rms_tagkey_alloc_initialized("creationDate" , 1 , rms_char_type , false);
  
  now = time(NULL);
  localtime_r(&now , &ts);

  rms_tagkey_set_data_size(tagkey , NULL , len);
  rms_tagkey_alloc_data(tagkey);
  sprintf(tagkey->data , "%02d/%02d/%4d %02d:%02d:%02d" , 
	  ts.tm_mday, 
	  ts.tm_mon,
	  ts.tm_year + 1900, 
	  ts.tm_hour,
	  ts.tm_min,
	  ts.tm_sec);

  return tagkey;
}


