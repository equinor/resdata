#include <stdlib.h>
#include <hash.h>
#include <stdio.h>
#include <string.h>
#include <util.h>
#include <ecl_kw.h>
#include <fortio.h>
#include <ecl_fstate.h>


typedef struct {
  char *name;
  char *ecl_type;
  int size;
} ecl_var_type;


typedef struct {
  char *fortran_type;
  char *default_value;
  char *reader;
  char *writer;
} ecl_type_node;





static ecl_type_node * alloc_type_node(const char *fortran_type , const char *default_value , const char *reader , const char *writer) {
  ecl_type_node *node = malloc(sizeof *node);
  node->fortran_type  = util_alloc_string_copy(fortran_type);
  node->default_value = util_alloc_string_copy(default_value);
  node->reader        = util_alloc_string_copy(reader);
  node->writer        = util_alloc_string_copy(writer);
  return node;
}


static void free_type_node(ecl_type_node *node) {
  free(node->fortran_type);
  free(node->default_value);
  free(node->reader);
  free(node->writer);
  free(node);
}


hash_type *alloc_type_map() {
  hash_type *type_map = hash_alloc(10);
  hash_insert_ref(type_map , "REAL" , alloc_type_node("real*4"            		     , "0.0" 	      , "read_real"    , "write_real"));
  hash_insert_ref(type_map , "DOUB" , alloc_type_node("double precision" 	 	     , "0.0" 	      , "read_double"  , "write_double"));
  hash_insert_ref(type_map , "INTE" , alloc_type_node("integer"          		     , "1"    	      , "read_integer" , "write_integer"));
  hash_insert_ref(type_map , "CHAR" , alloc_type_node("Character(Len = eclipse_str_len)" , "\"AAAAAAAA\"" , "read_char"    , "write_char"));
  hash_insert_ref(type_map , "LOGI" , alloc_type_node("logical"          		     ,  ".false."     , "read_logical" , "write_logical"));
  hash_insert_ref(type_map , "MESS" , NULL);
  return type_map;
}


ecl_var_type * ecl_var_alloc(const char *name , const char *ecl_str_type , int size) {
  ecl_var_type *ecl_type = malloc( sizeof *ecl_type );
  ecl_type->name     = util_alloc_string_copy(name);
  ecl_type->ecl_type = util_alloc_string_copy(ecl_str_type);
  ecl_type->size     = size;
  return ecl_type;
}

const void * ecl_var_copyc(const void *_src) {
  ecl_var_type *src = (ecl_var_type *) _src;
  ecl_var_type *new = ecl_var_alloc(src->name , src->ecl_type , src->size);
  return new;
}

void ecl_var_free(void *_ecl_var) {
  ecl_var_type *ecl_var = (ecl_var_type *) _ecl_var;
  
  free(ecl_var->name);
  free(ecl_var->ecl_type);
  free(ecl_var);
}

  

void free_type_map(hash_type *type_map) {
  free_type_node(hash_get(type_map , "REAL")); 
  free_type_node(hash_get(type_map , "DOUB")); 
  free_type_node(hash_get(type_map , "INTE")); 
  free_type_node(hash_get(type_map , "CHAR")); 
  free_type_node(hash_get(type_map , "LOGI")); 
  /* free_type_node(hash_get(type_map , "MESS")); */
  hash_free(type_map);
}


static char * alloc_3string(const char *path , const char *prefix , const char *file) {
  char *s;
  s = malloc(strlen(path) + strlen(prefix) + strlen(file) + 6);
  sprintf(s , "%s/%s_%s.inc" , path ,prefix , file);
  return s;
}



void static ecl_parse_write_decl(const hash_type *var_hash , const hash_type *type_map , const char *prefix , const char *path) {
  char *decl_file    = alloc_3string(path , prefix , "declare");
  char *alloc_file   = alloc_3string(path , prefix , "allocate");
  char *dealloc_file = alloc_3string(path , prefix , "deallocate");

  FILE * declH    = fopen(decl_file    , "w");
  FILE * allocH   = fopen(alloc_file   , "w");
  FILE * deallocH = fopen(dealloc_file , "w");
  
  char **keylist = hash_alloc_keylist(var_hash);
  int i;

  for (i=0; i < hash_get_size(var_hash); i++) {
    const char * key          = keylist[i];
    const ecl_var_type  *var  = hash_get(var_hash , key);
    const ecl_type_node *type = hash_get(type_map , var->ecl_type);

    fprintf(declH , "%20s,  allocatable :: %s(:)\n", type->fortran_type , var->name);
    
    


  }
  fclose(declH);
  fclose(allocH);
  fclose(deallocH);

  free(decl_file);
  free(alloc_file);
  free(dealloc_file);
}


/*****************************************************************/
static void ecl_parse_file(hash_type *hash , const char *filename, const hash_type *type_map , bool endian_flip) {
  bool fmt_file       = util_fmt_bit8(filename , 65536);
  fortio_type *fortio = fortio_open(filename , "r" , endian_flip);
  ecl_kw_type *ecl_kw = ecl_kw_alloc_empty(fmt_file , endian_flip);
  
  printf("Parsing: %s \n",filename);
  while (ecl_kw_fread_header(ecl_kw , fortio)) {
    ecl_kw_fskip_data(ecl_kw, fortio);
    if (!hash_has_key(hash , ecl_kw_get_header_ref(ecl_kw))) {
      ecl_var_type *ecl_var = ecl_var_alloc(ecl_kw_get_header_ref(ecl_kw) , ecl_kw_get_str_type_ref(ecl_kw) , ecl_kw_get_size(ecl_kw));
      hash_insert_copy(hash , ecl_kw_get_header_ref(ecl_kw) , ecl_var , ecl_var_copyc , ecl_var_free);
      printf("Var: %s :%s:%8d \n",ecl_kw_get_header_ref(ecl_kw) , ecl_var->ecl_type , ecl_var->size);
      ecl_var_free(ecl_var);
    }
  }
}
/*****************************************************************/


static void ecl_parse_restart(const char *refcase_path , const char *ecl_base , const char *include_path, const hash_type *type_map , bool fmt_file , bool unified , bool endian_flip) {
  char **fileList;
  int files;
  if (unified) {
    files = 1;
    fileList    = calloc(1 , sizeof *fileList);
    fileList[0] = malloc(strlen(refcase_path) + 1 + strlen(ecl_base) + 1 + 7);
    if (fmt_file)
      sprintf(fileList[0] , "%s/%s.FUNRST" , refcase_path , ecl_base);
    else
      sprintf(fileList[0] , "%s/%s.UNRST" , refcase_path , ecl_base);
  } else {
    if (fmt_file)
      fileList = ecl_fstate_alloc_filelist(refcase_path , ecl_base , "F", &files);
    else
      fileList = ecl_fstate_alloc_filelist(refcase_path , ecl_base , "X", &files);
  }
  {
    hash_type *hash = hash_alloc(10);
    int i;
    for (i=0; i < files; i++)
      ecl_parse_file(hash , fileList[i] , type_map , endian_flip);
    
    hash_free(hash);
  }
  
  
  util_free_string_list(fileList , files);
}




static void ecl_parse_summary_data(const char *refcase_path , const char *ecl_base , const char *include_path, const hash_type *type_map , bool fmt_file , bool unified, bool endian_flip) {
  char **fileList;
  int files;
  if (unified) {
    files = 1;
    fileList = calloc(1 , sizeof *fileList);
    fileList[0] = malloc(strlen(refcase_path) + 1 + strlen(ecl_base) + 1 + 8);
    if (fmt_file)
      sprintf(fileList[0] , "%s/%s.FUNSMRY" , refcase_path , ecl_base);
    else
      sprintf(fileList[0] , "%s/%s.UNSMRY" , refcase_path , ecl_base);
  } else {
    if (fmt_file)
      fileList = ecl_fstate_alloc_filelist(refcase_path , ecl_base , "A", &files);
    else
      fileList = ecl_fstate_alloc_filelist(refcase_path , ecl_base , "S", &files);
  }
  
  {
    hash_type *hash = hash_alloc(10);
    int i;
    for (i=0; i < files; i++)
      ecl_parse_file(hash , fileList[i] , type_map , endian_flip);
    
    hash_free(hash);
  }
  
  
  util_free_string_list(fileList , files);
}



static void ecl_parse_summary_spec(const char *refcase_path , const char *ecl_base , const char *include_path, const hash_type *type_map , bool fmt_file , bool endian_flip) {
  char *spec_file;

  if (fmt_file) {
    spec_file = malloc(strlen(refcase_path) + strlen(ecl_base) + 9);
    sprintf(spec_file  , "%s/%s.FSMSPEC" , refcase_path , ecl_base);
  } else {
    spec_file = malloc(strlen(refcase_path) + strlen(ecl_base) + 8);
    sprintf(spec_file , "%s/%s.SMSPEC" , refcase_path , ecl_base);
  }
  
  {
    hash_type *hash = hash_alloc(10);
    ecl_parse_file(hash , spec_file ,  type_map , endian_flip);
    
    hash_free(hash);
  }
  free(spec_file);
}


  

static void ecl_parse_grid(const char *refcase_path , const char *ecl_base , const char *include_path, const hash_type *type_map , bool fmt_file , bool endian_flip) {
  char *grid_file;
  ecl_fstate_type *grid;
  if (fmt_file) {
    grid_file = malloc(strlen(refcase_path) + strlen(ecl_base) + 8);
    sprintf(grid_file  , "%s/%s.FEGRID" , refcase_path , ecl_base);
  } else {
    grid_file = malloc(strlen(refcase_path) + strlen(ecl_base) + 7);
    sprintf(grid_file , "%s/%s.EGRID" , refcase_path , ecl_base);
  } 
  printf("Parsing: %s \n",grid_file);
  if (fmt_file)
    grid = ecl_fstate_load_unified(grid_file , ECL_FORMATTED , endian_flip);
  else
    grid = ecl_fstate_load_unified(grid_file , ECL_BINARY , endian_flip);

  {
    FILE *fileH;
    int *gridhead  = ecl_fstate_kw_get_data_ref(grid , 0 , "GRIDHEAD");
    int *actnum    = ecl_fstate_kw_get_data_ref(grid , 0 , "ACTNUM");
    char *inc_file = malloc(strlen(include_path) + 1 + strlen("dimensions.inc") + 1);
    int i;
    int active = 0;
    
    sprintf(inc_file , "%s/dimensions.inc" , include_path);
    for (i=0; i < ecl_fstate_kw_get_size(grid , 0 , "ACTNUM"); i++)
      if (actnum[i] != 0)
	active += 1;

    fileH = fopen(inc_file , "w");
    fprintf(fileH , "integer, parameter :: nx      = %8d \n",gridhead[1]);
    fprintf(fileH , "integer, parameter :: ny      = %8d \n",gridhead[2]);
    fprintf(fileH , "integer, parameter :: nz      = %8d \n",gridhead[3]);
    fprintf(fileH , "integer, parameter :: nactive = %8d \n",active);
    fclose(fileH);
  
    free(inc_file);
  }
  
  free(grid_file);
  ecl_fstate_free(grid);
}



void ecl_parse(const char *refcase_path , const char *eclbase, const char *include_path, bool fmt_file , bool unified, bool endian_flip) {
  hash_type   *type_map = alloc_type_map();
  
  ecl_parse_summary_spec(refcase_path , eclbase , include_path, type_map , fmt_file , endian_flip);
  ecl_parse_summary_data(refcase_path , eclbase , include_path , type_map , fmt_file , unified , endian_flip);
  ecl_parse_restart(refcase_path , eclbase , include_path , type_map , fmt_file , unified, endian_flip);
  ecl_parse_grid(refcase_path , eclbase , include_path , type_map , fmt_file , endian_flip);

  free_type_map(type_map);
}

