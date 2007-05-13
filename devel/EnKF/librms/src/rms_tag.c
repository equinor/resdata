#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <hash.h>
#include <list.h>
#include <util.h>
#include <rms_tag.h>
#include <rms_util.h>
#include <rms_tagkey.h>

static const char * rms_eof_tag           = "eof";
static const char * rms_starttag_string   = "tag";
static const char * rms_endtag_string     = "endtag";




struct rms_tag_struct {
  char       *name;
  list_type  *key_list;
  hash_type  *key_hash;
};

/*****************************************************************/


rms_tag_type * rms_tag_alloc(const char * name) {
  rms_tag_type *tag = malloc(sizeof *tag);
  tag->name = NULL;
  tag->key_hash = hash_alloc(10);
  tag->key_list = list_alloc();
  if (name != NULL)
    tag->name = util_alloc_string_copy(name);
  return tag;
}


void rms_tag_free(rms_tag_type *tag) {
  free(tag->name);
  hash_free(tag->key_hash);
  list_free(tag->key_list);
  free(tag);
}

  
const char * rms_tag_get_name(const rms_tag_type *tag) {
  return tag->name;
}



/*static*/ 
void rms_fread_tag_header(rms_tag_type *tag , FILE *stream , bool *eof_tag) {
  char *buffer;
  *eof_tag = false;
  buffer = malloc(4);
  if (rms_util_fread_string(buffer , 4 , stream )) {
    if (strcmp(buffer , rms_starttag_string) == 0) {
      /* OK */
      {
	char *tmp = malloc(rms_util_fread_strlen(stream) + 1);
	rms_util_fread_string(tmp , 0 , stream);
	tag->name = tmp;
	if (strcmp(tag->name , rms_eof_tag) == 0)
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


bool rms_tag_name_eq(const rms_tag_type *tag , const char * tagname , const char *tagkey_name , const char *keyvalue) {
  bool eq = false;
  if (strcmp(tag->name , tagname) == 0) {
    if (tagkey_name != NULL && keyvalue != NULL) {
      if (hash_has_key(tag->key_hash , tagkey_name)) {
	const rms_tagkey_type *tagkey = list_node_value_ptr(hash_get(tag->key_hash , tagkey_name));
	eq = rms_tagkey_char_eq(tagkey , keyvalue);
      }
    } else
      eq = true;
  }
  return eq;
}


rms_tagkey_type * rms_tag_get_key(const rms_tag_type *tag , const char *keyname) {
  if (hash_has_key(tag->key_hash , keyname)) 
    return list_node_value_ptr(hash_get(tag->key_hash, keyname));
  else {
    fprintf(stderr,"%s: tag:%s did not contain key:%s - aborting \n",__func__ , tag->name , keyname);
    abort();
  }
}


static void rms_tag_add_tagkey(const rms_tag_type *tag , const rms_tagkey_type *tagkey) {
  list_node_type * list_node = list_append_copy(tag->key_list , tagkey , rms_tagkey_copyc_ , rms_free_tagkey_);
  hash_insert_ref(tag->key_hash , rms_tagkey_get_name(tagkey) , list_node);
}


static bool rms_tag_at_endtag(FILE *stream) {
  const int init_pos = ftell(stream);
  bool at_endtag;
  char tag[7];
  if (rms_util_fread_string(tag , 7 , stream)) {
    if (strcmp(tag , rms_endtag_string) == 0)
      at_endtag = true;
    else
      at_endtag = false;
  } else
    at_endtag = false;
  
  if (!at_endtag)
    fseek(stream , init_pos , SEEK_SET);
  return at_endtag;
}


void rms_fread_tag(rms_tag_type *tag, FILE *stream , hash_type *type_map , bool endian_convert , bool *at_eof) {
  rms_fread_tag_header(tag , stream , at_eof);
  if (!*at_eof) {
    rms_tagkey_type *tagkey = rms_tagkey_alloc_empty(endian_convert);
    while (! rms_tag_at_endtag(stream)) {
      rms_tagkey_load(tagkey , endian_convert , stream , type_map);
      rms_tag_add_tagkey(tag , tagkey);
    }
    rms_free_tagkey(tagkey);
  }
}



rms_tag_type * rms_tag_fread_alloc(FILE *stream , hash_type *type_map , bool endian_convert , bool *at_eof) {
  rms_tag_type *tag = rms_tag_alloc(NULL);
  rms_fread_tag(tag , stream , type_map , endian_convert , at_eof);
  return tag;
}



void rms_tag_fwrite(const rms_tag_type * tag , FILE * stream) {
  rms_util_fwrite_string("tag"     , stream);
  rms_util_fwrite_string(tag->name , stream);
  {
    list_node_type * key_node = list_get_head(tag->key_list);
    while (key_node != NULL) {
      const rms_tagkey_type * tagkey = (const rms_tagkey_type *) list_node_value_ptr(key_node);
      rms_tagkey_fwrite( tagkey , stream);
      key_node = list_node_get_next(key_node);
    }
  }
  rms_util_fwrite_string("endtag" , stream);
}


void rms_tag_fwrite_eof(FILE *stream) {
  rms_tag_type * tag = rms_tag_alloc("eof");
  rms_tag_fwrite(tag , stream);
  rms_tag_free(tag);
}


void rms_tag_fwrite_filedata(const char * filetype, FILE *stream) {
  rms_tag_type * tag = rms_tag_alloc("filedata");
  
  rms_tag_add_tagkey(tag , rms_tagkey_alloc_byteswap());
  rms_tag_add_tagkey(tag , rms_tagkey_alloc_filetype(filetype));
  rms_tag_add_tagkey(tag , rms_tagkey_alloc_creationDate());

  rms_tag_fwrite(tag , stream);
  rms_tag_free(tag);
}


void rms_tag_printf(const rms_tag_type * tag) {
  printf("%s \n",tag->name);
}



const char *rms_tag_name_ref(const rms_tag_type * tag) {
  return tag->name;
}


