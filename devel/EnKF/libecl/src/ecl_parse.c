#include <stdlib.h>
#include <hash.h>
#include <stdio.h>
#include <string.h>
#include <util.h>
#include <ecl_kw.h>
#include <fortio.h>


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


static char * alloc_string_copy(const char *src ) {
  if (src != NULL) {
    char *copy = calloc(strlen(src) + 1 , sizeof *copy);
    strcpy(copy , src);
    return copy;
  } else 
    return NULL;

}




static ecl_type_node * alloc_type_node(const char *fortran_type , const char *default_value , const char *reader , const char *writer) {
  ecl_type_node *node = malloc(sizeof *node);
  node->fortran_type  = alloc_string_copy(fortran_type);
  node->default_value = alloc_string_copy(default_value);
  node->reader        = alloc_string_copy(reader);
  node->writer        = alloc_string_copy(writer);
  return node;
}


static void free_type_node(ecl_type_node *node) {
  free(node->fortran_type);
  free(node->default_value);
  free(node->reader);
  free(node->writer);
  free(node);
}


void init_type_map(hash_type *type_map) {
  hash_insert_ref(type_map , "REAL" , alloc_type_node("real*4"            		     , "0.0" 	      , "read_real"    , "write_real"));
  hash_insert_ref(type_map , "DOUB" , alloc_type_node("double precision" 	 	     , "0.0" 	      , "read_double"  , "write_double"));
  hash_insert_ref(type_map , "INTE" , alloc_type_node("integer"          		     , "1"    	      , "read_integer" , "write_integer"));
  hash_insert_ref(type_map , "CHAR" , alloc_type_node("Character(Len = eclipse_str_len)" , "\"AAAAAAAA\"" , "read_char"    , "write_char"));
  hash_insert_ref(type_map , "LOGI" , alloc_type_node("logical"          		     ,  ".false."     , "read_logical" , "write_logical"));
  hash_insert_ref(type_map , "MESS" , NULL);
}


void free_type_map(hash_type *type_map) {
  free(hash_get(type_map , "REAL")); 
  free(hash_get(type_map , "DOUB")); 
  free(hash_get(type_map , "INTE")); 
  free(hash_get(type_map , "CHAR")); 
  free(hash_get(type_map , "LOGI")); 
  free(hash_get(type_map , "MESS")); 
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



void ecl_parse(const char *refcase_path , const char *include_path) {
  ecl_kw_type *ecl_kw;
  fortio_type *fortio;
  
}

