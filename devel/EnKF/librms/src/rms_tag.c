#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <hash.h>
#include <rms_tag.h>
#include <rms_util.h>
#include <rms_tagkey.h>

static const char * rms_eof_tag           = "eof";
static const char * rms_starttag_string   = "tag";
static const char * rms_endtag_string     = "endtag";




struct rms_tag_struct {
  const char *name;
  hash_type  *keys;
};

/*****************************************************************/


rms_tag_type * rms_alloc_empty_tag() {
  rms_tag_type *tag = malloc(sizeof *tag);
  tag->name = NULL;
  tag->keys = hash_alloc(10);
  return tag;
}

void rms_tag_free(rms_tag_type *tag) {
  free( (char *) tag->name);
  hash_free(tag->keys);
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
  if (rms_fread_string(stream , buffer , 4)) {
    if (strcmp(buffer , rms_starttag_string) == 0) {
      /* OK */
      {
	char *tmp = malloc(rms_fread_strlen(stream) + 1);
	rms_fread_string(stream , tmp , 0);
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
      if (hash_has_key(tag->keys , tagkey_name)) {
	eq = rms_tagkey_char_eq(hash_get(tag->keys , tagkey_name) , keyvalue);
      }
    } else
      eq = true;
  }
  return eq;
}


rms_tagkey_type * rms_tag_get_key(const rms_tag_type *tag , const char *keyname) {
  if (hash_has_key(tag->keys , keyname))
    return hash_get(tag->keys, keyname); 
  else {
    fprintf(stderr,"%s: tag:%s did not contain key:%s - aborting \n",__func__ , tag->name , keyname);
    abort();
  }
}


static void rms_tag_add_tagkey(const rms_tag_type *tag , const rms_tagkey_type *tagkey) {
  hash_insert_copy(tag->keys , rms_tagkey_get_name(tagkey) , tagkey , rms_tagkey_copyc_ , rms_free_tagkey_);
}


static bool rms_tag_at_endtag(FILE *stream) {
  const int init_pos = ftell(stream);
  bool at_endtag;
  char tag[7];
  if (rms_fread_string(stream , tag , 7)) {
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


void rms_fread_tag(rms_tag_type *tag, FILE *stream , hash_type *type_map , bool *at_eof) {
  rms_fread_tag_header(tag , stream , at_eof);
  if (!*at_eof) {
    rms_tagkey_type *tagkey = rms_alloc_empty_tagkey();
    while (! rms_tag_at_endtag(stream)) {
      rms_tagkey_load(tagkey , stream , type_map);
      rms_tag_add_tagkey(tag , tagkey);
    }
    rms_free_tagkey(tagkey);
  }
}

rms_tag_type * rms_tag_fread_alloc(FILE *stream , hash_type *type_map , bool *at_eof) {
  rms_tag_type *tag = rms_alloc_empty_tag();
  rms_fread_tag(tag , stream , type_map , at_eof);
  return tag;
}


