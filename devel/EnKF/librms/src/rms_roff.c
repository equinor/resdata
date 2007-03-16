#include <stdlib.h>
#include <stdio.h>
#include <rms_roff.h>
#include <string.h>
#include <stdbool.h>
#include <hash.h>
#include <list.h>
#include <list_node.h>


/*****************************************************************/
static const char * rms_roff_ascii_header      = "roff-asc";
static const char * rms_roff_binary_header     = "roff-bin";
static const char * rms_roff_comment1          = "ROFF file";
static const char * rms_roff_comment2          = "Creator: RMS - Reservoir Modelling System, version 7.5.2";
static const char * rms_roff_parameter_tagname = "parameter";
static const char * rms_roff_tagkey_name       = "name";





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



/* Interface: */
void rms_roff_free_tag(rms_roff_tag_type *);


#include "type.c"
#include "fortran.c"
#include "file.c"
#include "tagkey.c"
#include "tag.c"






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



				  
