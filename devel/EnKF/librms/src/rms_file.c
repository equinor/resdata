#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <hash.h>
#include <list.h>
#include <list_node.h>
#include <util.h>

#include <rms_type.h>
#include <rms_util.h>
#include <rms_tag.h>
#include <rms_file.h>
#include <rms_tagkey.h>

/*****************************************************************/
static const char * rms_ascii_header      = "roff-asc";
static const char * rms_binary_header     = "roff-bin";

static const char * rms_comment1          = "ROFF file";
static const char * rms_comment2          = "Creator: RMS - Reservoir Modelling System, version 8.1";
/*
  static const char * rms_parameter_tagname = "parameter";
*/





struct rms_file_struct {
  char       * filename;
  bool         endian_convert;
  bool         fmt_file;
  hash_type  * type_map;
  list_type  * tag_list;
};



/*****************************************************************/
/* Pure roff routines */




static bool rms_fmt_file(const rms_file_type *rms_file , FILE *stream) {
  bool fmt_file;
  char filetype[9];
  rms_util_fread_string( filetype , 9 , stream);

  if (strncmp(filetype , rms_binary_header , 8) == 0)
    fmt_file = false;
  else if (strncmp(filetype , rms_ascii_header , 8) == 0)
    fmt_file = true;
  else {
    fprintf(stderr,"%s: header : %8s not recognized in file: %s - aborting \n",__func__ , filetype , rms_file->filename);
    abort();
  }
  return fmt_file;
}


    
static void rms_file_add_tag(rms_file_type *rms_file , const rms_tag_type *tag) {
  list_append_ref(rms_file->tag_list , tag);
}



rms_tag_type * rms_file_get_tag_ref(const rms_file_type *rms_file , const char *tagname, const char *keyname, const char *keyvalue) {
  bool cont = true;
  rms_tag_type *return_tag = NULL;
  list_node_type *tag_node = list_get_head(rms_file->tag_list);
  while (cont) {
    rms_tag_type *tag = list_node_value_ptr(tag_node);
    if (rms_tag_name_eq(tag , tagname , keyname , keyvalue)) {
      return_tag = tag;
      cont = false;
    } else 
      tag_node = list_node_get_next(tag_node);
    if (tag_node == NULL)
      cont = false;
  }
  if (return_tag == NULL) {
    if (keyname != NULL && keyvalue != NULL) 
      fprintf(stderr,"%s: failed to find tag:%s with key:%s=%s in file:%s - aborting \n",__func__ , tagname , keyname , keyvalue , rms_file->filename);
    else
      fprintf(stderr,"%s: failed to find tag:%s in file:%s - aborting \n",__func__ , tagname , rms_file->filename);
  }
  return return_tag;
}






rms_file_type * rms_file_alloc(const char *filename, bool fmt_file) {
  rms_file_type *rms_file   = malloc(sizeof *rms_file);
  rms_file->endian_convert  = false;
  rms_file->type_map  	    = hash_alloc(10);
  rms_file->tag_list  	    = list_alloc();
  
  hash_insert_hash_owned_ref(rms_file->type_map , "byte"   , rms_type_alloc(rms_byte_type ,    1) ,  rms_type_free);
  hash_insert_hash_owned_ref(rms_file->type_map , "bool"   , rms_type_alloc(rms_bool_type,     1) ,  rms_type_free);
  hash_insert_hash_owned_ref(rms_file->type_map , "int"    , rms_type_alloc(rms_int_type ,     4) ,  rms_type_free);
  hash_insert_hash_owned_ref(rms_file->type_map , "float"  , rms_type_alloc(rms_float_type  ,  4) ,  rms_type_free);
  hash_insert_hash_owned_ref(rms_file->type_map , "double" , rms_type_alloc(rms_double_type ,  8) ,  rms_type_free);

  hash_insert_hash_owned_ref(rms_file->type_map , "char"   , rms_type_alloc(rms_char_type   , -1) ,  rms_type_free);   /* Char are a f*** mix of vector and scalar */

  rms_file->filename = NULL;
  rms_file_set_filename(rms_file , filename , fmt_file);
  return rms_file;
}






void rms_file_set_filename(rms_file_type * rms_file , const char *filename , bool fmt_file) {
  rms_file->filename = util_realloc_string_copy(rms_file->filename , filename);
  rms_file->fmt_file   = fmt_file;
}


static void rms_file_delete_tag(rms_file_type * rms_file , list_node_type *tag_node) {
  rms_tag_free( list_node_value_ptr(tag_node) );
  list_del_node(rms_file->tag_list , tag_node);
}


void rms_file_free_data(rms_file_type * rms_file) {
  list_node_type    *tag_node , *next_tag_node;
  list_type         *tag_list = rms_file->tag_list;
    
  tag_node = list_get_head(tag_list);
  while (tag_node) {
    next_tag_node = list_node_get_next(tag_node);
    rms_file_delete_tag(rms_file , tag_node);
    tag_node = next_tag_node;
  }
}



void rms_file_free(rms_file_type * rms_file) {
  rms_file_free_data(rms_file);
  list_free(rms_file->tag_list);
  hash_free(rms_file->type_map);
  free(rms_file->filename);
  free(rms_file);
}


static int rms_file_get_dim(const rms_tag_type *tag , const char *dim_name) {
  rms_tagkey_type *key = rms_tag_get_key(tag , dim_name);
  if (key == NULL) {
    fprintf(stderr,"%s: failed to find tagkey:%s aborting \n" , __func__ , dim_name);
    abort();
  }
  return * (int *) rms_tagkey_get_data_ref(key);
}



void rms_file_assert_dimensions(const rms_file_type *rms_file , int nx , int ny , int nz) {
  bool OK = true;  
  rms_tag_type    *tag    = rms_file_get_tag_ref(rms_file , "dimensions" , NULL , NULL);
  OK =       (nx == rms_file_get_dim(tag , "nX"));
  OK = OK && (ny == rms_file_get_dim(tag , "nY"));
  OK = OK && (nz == rms_file_get_dim(tag , "nZ"));

  if (!OK) {
    fprintf(stderr,"%s: dimensions on file: %s (%d, %d, %d) did not match with input dimensions (%d,%d,%d) - aborting \n",__func__ , rms_file->filename,
	    rms_file_get_dim(tag , "nX"), rms_file_get_dim(tag , "nY"), rms_file_get_dim(tag , "nZ"),
	    nx , ny , nz);
    abort();
  }
}




static void rms_file_init_fread(rms_file_type * rms_file , FILE * stream) {

  rms_file->fmt_file = rms_fmt_file(rms_file , stream);
  if (rms_file->fmt_file) {
    fprintf(stderr,"%s only binary files implemented - aborting \n",__func__);
    abort();
  }
  /* Skipping two comment lines ... */
  rms_util_fskip_string(stream);
  rms_util_fskip_string(stream);  
  {
    bool eof_tag;
    rms_tag_type    * filedata_tag = rms_tag_fread_alloc(stream , rms_file->type_map , rms_file->endian_convert , &eof_tag);
    rms_tagkey_type * byteswap_key = rms_tag_get_key(filedata_tag , "byteswaptest");
    if (byteswap_key == NULL) {
      fprintf(stderr,"%s: failed to find filedata/byteswaptest - aborting \n", __func__);
      abort();
    }
    int byteswap_value             = *( int *) rms_tagkey_get_data_ref(byteswap_key);
    if (byteswap_value == 1)
      rms_file->endian_convert = false;
    else
      rms_file->endian_convert = true;
    rms_tag_free(filedata_tag);
  }

}


rms_tag_type * rms_file_fread_alloc_tag(rms_file_type * rms_file , const char *tagname , const char * keyname , const char *keyvalue ) {
  FILE *stream       = rms_file_fopen(rms_file , true);
  rms_tag_type * tag = NULL;
  bool cont          = true;
  bool tag_found     = false;
  long int start_pos = ftell(stream);
  fseek(stream , 0 , SEEK_SET);
  rms_file_init_fread(rms_file , stream);
  while (cont) {
    bool eof_tag;
    rms_tag_type * tmp_tag = rms_tag_fread_alloc(stream , rms_file->type_map , rms_file->endian_convert , &eof_tag);
    if (rms_tag_name_eq(tmp_tag , tagname , keyname , keyvalue)) {
      tag_found = true;
      tag = tmp_tag;
    } else 
      rms_tag_free(tmp_tag);
    if (tag_found || eof_tag)
      cont = false;
  }
  if (tag == NULL)
    fseek(stream , start_pos , SEEK_SET);
  fclose(stream);
  return tag;
}


FILE * rms_file_fopen(const rms_file_type *rms_file, bool _read) {
  FILE * stream;
  if (_read)
    stream = fopen(rms_file->filename , "r");
  else
    stream = fopen(rms_file->filename , "w");

  if (stream == NULL) {
    fprintf(stderr,"%s: failed to open file: %s for ",__func__ , rms_file->filename);
    if (_read) 
      fprintf(stderr," reading - aborting \n");
    else
      fprintf(stderr," writing - aborting \n");
    abort();
  }
  return stream;
}

rms_tagkey_type * rms_file_fread_alloc_data_tagkey(rms_file_type * rms_file , const char *tagname , const char * keyname , const char *keyvalue) {
  rms_tag_type * tag = rms_file_fread_alloc_tag(rms_file , tagname , keyname , keyvalue);
  if (tag != NULL) {
    rms_tagkey_type *tagkey = rms_tagkey_copyc( rms_tag_get_key(tag , "data") );
    rms_tag_free(tag);
    return tagkey;
  } else
    return NULL;
}



void rms_file_fread(rms_file_type *rms_file) {
  FILE *stream = rms_file_fopen(rms_file , true);
  rms_file_init_fread(rms_file , stream);
  
  /* The main read loop */
  {
    bool eof_tag = false;
    while (!eof_tag) {
      rms_tag_type * tag = rms_tag_fread_alloc(stream ,  rms_file->type_map , rms_file->endian_convert , &eof_tag );
      if (!eof_tag)
	rms_file_add_tag(rms_file , tag);
      else
	rms_tag_free(tag);
      
    }
  }
  fclose(stream);
}



/*static */
void rms_file_init_fwrite(const rms_file_type * rms_file , const char * filetype , FILE *stream) {
  if (!rms_file->fmt_file)
    rms_util_fwrite_string(rms_binary_header , stream);
  else {
    fprintf(stderr,"%s: Sorry only binary writes implemented ... \n",__func__);
    rms_util_fwrite_string(rms_ascii_header , stream);
  }
  
  rms_util_fwrite_comment(rms_comment1 , stream);
  rms_util_fwrite_comment(rms_comment2 , stream);
  rms_tag_fwrite_filedata(filetype , stream);
}


void rms_file_complete_fwrite(const rms_file_type * rms_file , FILE * stream) {
  rms_tag_fwrite_eof(stream);
}


void rms_file_fwrite(const rms_file_type * rms_file, const char * filetype) {
  FILE * stream = rms_file_fopen(rms_file , false);
  rms_file_init_fwrite(rms_file , filetype , stream);

  {
    list_node_type * tag_node = list_get_head(rms_file->tag_list);
    while (tag_node != NULL) {
      const rms_tag_type *tag = list_node_value_ptr(tag_node);
      rms_tag_fwrite(tag , stream);
      tag_node = list_node_get_next(tag_node);
    }
  }
  rms_file_complete_fwrite(rms_file , stream);
  fclose(stream);
}


void rms_file_printf(const rms_file_type *rms_file , FILE *stream) {
  fprintf(stream , "<%s>\n",rms_file->filename);
  {
    list_node_type * tag_node = list_get_head(rms_file->tag_list);
    while (tag_node != NULL) {
      const rms_tag_type *tag = list_node_value_ptr(tag_node);
      rms_tag_printf(tag , stream);
      tag_node = list_node_get_next(tag_node);
    }
  }
  fprintf(stream , "</%s>\n",rms_file->filename);
}



/*****************************************************************/
/* Old hack version: */
  

void old_rms_roff_load(const char *filename , const char *param_name , float *param) {
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
