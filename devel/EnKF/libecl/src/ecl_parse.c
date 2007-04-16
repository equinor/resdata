#include <stdlib.h>
#include <hash.h>
#include <stdio.h>
#include <string.h>
#include <util.h>
#include <ecl_kw.h>
#include <fortio.h>
#include <ecl_fstate.h>
#include <str_buffer.h>

#define READER_FMT_VAR  "read_fmt"
#define WRITER_FMT_VAR  "write_fmt"

#define VERB_SILENT 0
#define VERB_DOT    1 
#define VERB_NAME   2


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
  s = malloc(strlen(path) + strlen(prefix) + strlen(file) + 7);
  sprintf(s , "%s/%s_%s.inc" , path ,prefix , file);
  return s;
}


static FILE * fopen_printf(const char *name) {
  printf("   Writing include file: %s \n", name);
  return fopen(name , "w");
}


void static ecl_parse_write_read_eclipse(const hash_type *var_hash , const hash_type *type_map , hash_type *special , const char *prefix , const char *path,
					 const char *type_arg, const char *size_arg , const char *arg_index) {
  char *read_file = alloc_3string(path , prefix , "readeclipse");
  FILE *fileH     = fopen_printf(read_file );
  char **keylist  = hash_alloc_keylist(var_hash);
  int i;
  for (i=0; i < hash_get_size(var_hash); i++) {
    const char * key          = keylist[i];
    const ecl_var_type  *var  = hash_get(var_hash , key);
    const ecl_type_node *type = hash_get(type_map , var->ecl_type);
    
    fprintf(fileH , "case(\"%s\") \n" , var->name);
    if (type != NULL) {
      fprintf(fileH , "    deallocate(%s) \n",var->name);
      fprintf(fileH , "    allocate(%s(%s(%s))) \n",var->name , size_arg , arg_index);
    }
    if (hash_has_key(special , var->name))
      fprintf(fileH , "    %s \n",hash_get_string(special , var->name));
    if (type != NULL)
      fprintf(fileH , "    call %s(%s(%s),%s,%s(%s),%s) \n" , type->reader , size_arg , arg_index , var->name, type_arg , arg_index , READER_FMT_VAR);
    fprintf(fileH , "\n");
  }
  fclose(fileH);
  util_free_string_list(keylist , hash_get_size(var_hash));
  free(read_file);
}


void static ecl_parse_write_decl(const hash_type *var_hash , const hash_type *type_map , const char *prefix , const char *path) {
  char *decl_file    = alloc_3string(path , prefix , "declare");
  char *alloc_file   = alloc_3string(path , prefix , "allocate");
  char *dealloc_file = alloc_3string(path , prefix , "deallocate");


  FILE * declH    = fopen_printf(decl_file);
  FILE * allocH   = fopen_printf(alloc_file);
  FILE * deallocH = fopen_printf(dealloc_file);
  
  char **keylist = hash_alloc_keylist(var_hash);
  int i;
  
  for (i=0; i < hash_get_size(var_hash); i++) {
    const char * key          = keylist[i];
    const ecl_var_type  *var  = hash_get(var_hash , key);
    const ecl_type_node *type = hash_get(type_map , var->ecl_type);
    
    if (type != NULL) {
      fprintf(declH    , "%s,  allocatable :: %s(:)\n", type->fortran_type , var->name);
      fprintf(allocH   , "allocate( %s(1) ) ; %s = %s \n" , var->name , var->name , type->default_value);
      fprintf(deallocH , "deallocate( %s ) \n", var->name);
    }
  }
  fclose(declH);
  fclose(allocH);
  fclose(deallocH);
  
  util_free_string_list(keylist , hash_get_size(var_hash));
  free(decl_file);
  free(alloc_file);
  free(dealloc_file);
}


void static ecl_parse_write_res_iostatic(hash_type *var_hash , hash_type *dynamic , const char *path) {
  char *file = malloc(strlen(path) + 1 + 18 + 1);
  FILE *stream;
  char tmp_buffer[128];
  int i;
  str_buffer_type *str_buffer = str_buffer_alloc(1);
  char **keyList = hash_alloc_sorted_keylist(var_hash);

  for (i=0; i < hash_get_size(var_hash); i++) {
    if (! hash_has_key(dynamic , keyList[i])) {
      sprintf(tmp_buffer , "%8s , &\n",keyList[i]);
      str_buffer_add_string(str_buffer , tmp_buffer);
    }
  }

  sprintf(file , "%s/res_iostatic.inc" , path);
  stream = fopen_printf(file );
  str_buffer_fprintf_substring(str_buffer , 0 , -4 , stream);
  fprintf(stream , "\n");
  fclose(stream);
  
  util_free_string_list(keyList , hash_get_size(var_hash));
  str_buffer_free(str_buffer);
  free(file);
}

void static ecl_parse_res_write_eclipse2(hash_type *var_hash , const char *include_path , const hash_type *special, const hash_type * type_map) {
  FILE *stream;
  char *filename = malloc(strlen(include_path) + 1 + 22);
  char **keyList = hash_alloc_keylist(var_hash);
  int i;
  sprintf(filename , "%s/res_writeeclipse2.inc" , include_path);
  stream = fopen_printf(filename );
  for (i=0; i < hash_get_size(var_hash); i++) {
    const char * key          = keyList[i];
    const ecl_var_type  *var  = hash_get(var_hash , key);
    const ecl_type_node *type = hash_get(type_map , var->ecl_type);

    fprintf(stream , "case(\"%s\")\n" , var->name);
    if (type != NULL) 
      fprintf(stream , "   call %s(fieldname(i) , fieldsize(i) , fieldtype(i) , %s , %s)\n" , type->writer , var->name , WRITER_FMT_VAR);

    if (hash_has_key(special , var->name))
      fprintf(stream, "   %s\n",hash_get_string(special , var->name));
    fprintf(stream , "\n");
  }
  util_free_string_list(keyList , hash_get_size(var_hash));
  free(filename);
  fclose(stream);
}



void static ecl_parse_res_write_eclipse1(hash_type *var_hash , const char *include_path , const hash_type *special, const hash_type * type_map) {
  FILE *stream;
  char *filename = malloc(strlen(include_path) + 1 + 22);
  char **keyList = hash_alloc_keylist(var_hash);
  int i;
  sprintf(filename , "%s/res_writeeclipse1.inc" , include_path);
  stream = fopen_printf(filename );
  for (i=0; i < hash_get_size(var_hash); i++) {
    const char * key          = keyList[i];
    const ecl_var_type  *var  = hash_get(var_hash , key);
    const ecl_type_node *type = hash_get(type_map , var->ecl_type);

    fprintf(stream , "case(\"%s\")\n" , var->name);
    if (type != NULL) {
      fprintf(stream , "   deallocate(%s) \n",var->name);
      fprintf(stream , "   allocate(%s(fieldsize(i)))\n",var->name);
    }
    if (hash_has_key(special , var->name))
      fprintf(stream, "   %s\n",hash_get_string(special , var->name));
    fprintf(stream , "\n");
  }
  util_free_string_list(keyList , hash_get_size(var_hash));
  free(filename);
  fclose(stream);
}


/*****************************************************************/

static void ecl_parse_file(hash_type *hash , const char *filename, const hash_type *type_map , bool endian_flip , int verbosity) {
  bool fmt_file       = util_fmt_bit8(filename , 65536);
  fortio_type *fortio = fortio_open(filename , "r" , endian_flip);
  ecl_kw_type *ecl_kw = ecl_kw_alloc_empty(fmt_file , endian_flip);

  if (verbosity == VERB_DOT)
    printf("."); 
  else if (verbosity == VERB_NAME)
    printf("Parsing(2): %s" , filename);
  fflush(stdout);
  
  while (ecl_kw_fread_header(ecl_kw , fortio)) {
    char *name            = util_alloc_strip_copy(ecl_kw_get_header_ref(ecl_kw));
    ecl_kw_fskip_data(ecl_kw, fortio);
    if (!hash_has_key(hash , name)) {
      ecl_var_type *ecl_var = ecl_var_alloc(name , ecl_kw_get_str_type_ref(ecl_kw) , ecl_kw_get_size(ecl_kw));
      hash_insert_copy(hash , name , ecl_var , ecl_var_copyc , ecl_var_free);
      ecl_var_free(ecl_var);
    }
    free(name);
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
    int verbosity;
    if (unified) 
      verbosity = VERB_NAME;
    else {
      verbosity = VERB_DOT;
      printf("Parsing restart files: ");
    }
    
    for (i=0; i < files; i++) 
      ecl_parse_file(hash , fileList[i] , type_map , endian_flip , verbosity);
    printf("\n");
    ecl_parse_write_decl(hash , type_map , "res" , include_path);
    {
      hash_type *dynamic = hash_alloc(10);
      hash_insert_int(dynamic , "PRESSURE", 1);
      hash_insert_int(dynamic , "RS", 1);
      hash_insert_int(dynamic , "STARTSOL", 1);
      hash_insert_int(dynamic , "ENDSOL", 1);
      hash_insert_int(dynamic , "SGAS", 1);
      hash_insert_int(dynamic , "SWAT", 1);
      hash_insert_int(dynamic , "RV", 1);

      ecl_parse_write_res_iostatic(hash , dynamic , include_path);
      hash_free(dynamic);
    }

    
    {
      hash_type *special = hash_alloc(10);
      hash_insert_string_copy(special , "PRESSURE"    , "ipres  = i");
      hash_insert_string_copy(special , "SGAS"        , "isgas  = i");
      hash_insert_string_copy(special , "SWAT"        , "iswat  = i");
      hash_insert_string_copy(special , "RS"          , "irs    = i");
      hash_insert_string_copy(special , "RV"          , "irv    = i");

      ecl_parse_write_read_eclipse(hash , type_map , special, "res" , include_path , "fieldtype" , "fieldsize" , "i");
      ecl_parse_res_write_eclipse1(hash , include_path , special , type_map);
      
      hash_clear(special);
      {
	char tmp_string[256];
	str_buffer_type *pressure_string = str_buffer_alloc(1);
	str_buffer_type *sol_string      = str_buffer_alloc_with_string("call write_eclipse_kwheader(fieldname(i), fieldsize(i) , fieldtype(i) , 10 , write_fmt)\n");
	
	sprintf(tmp_string , "   if (iopt == 22) call write_real('PERMX   ',ndim,'REAL',PERMX,%s)\n" , WRITER_FMT_VAR); str_buffer_add_string(pressure_string , tmp_string);
	sprintf(tmp_string , "      if (iopt == 22) call write_real('PERMZ   ',ndim,'REAL',PERMZ,%s)\n" , WRITER_FMT_VAR); str_buffer_add_string(pressure_string , tmp_string);
	sprintf(tmp_string , "      if (iopt == 22) call write_real('PORO    ',ndim,'REAL',PORO ,%s)\n" , WRITER_FMT_VAR); str_buffer_add_string(pressure_string , tmp_string);
	str_buffer_add_string(pressure_string , "#ifdef MULTPV\n");
	sprintf(tmp_string , "      if (iopt == 22) call write_real('MULTPV    ',ndim,'REAL',MULTPV ,%s)\n" , WRITER_FMT_VAR); str_buffer_add_string(pressure_string , tmp_string);
	str_buffer_add_string(pressure_string , "#endif\n");
	str_buffer_add_string(pressure_string , "#ifdef GAUSS2\n");
	sprintf(tmp_string , "      if (iopt == 22) call write_real('GAUSS1   ',ndim,'REAL',GAUSS ,mem4%%gauss1,%s)\n" , WRITER_FMT_VAR); str_buffer_add_string(pressure_string , tmp_string);
	sprintf(tmp_string , "      if (iopt == 22) call write_real('GAUSS2   ',ndim,'REAL',GAUSS ,mem4%%gauss2,%s)\n" , WRITER_FMT_VAR); str_buffer_add_string(pressure_string , tmp_string);
	str_buffer_add_string(pressure_string , "#endif\n");
	
	hash_insert_string_copy(special , "PRESSURE" , str_buffer_get_char_ptr(pressure_string));
	hash_insert_string_copy(special , "STARTSOL" , str_buffer_get_char_ptr(sol_string));
	hash_insert_string_copy(special , "ENDSOL"   , str_buffer_get_char_ptr(sol_string));

	str_buffer_free(pressure_string);
	str_buffer_free(sol_string);
      }
      ecl_parse_res_write_eclipse2(hash , include_path , special , type_map);
      hash_free(special);
    }
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
    int verbosity;
    if (unified) 
      verbosity = VERB_NAME;
    else {
      verbosity = VERB_DOT;
      printf("Parsing summary files: ");
    }
    
    for (i=0; i < files; i++) 
      ecl_parse_file(hash , fileList[i] , type_map , endian_flip , verbosity);
    printf("\n");
    ecl_parse_write_decl(hash , type_map , "sum" , include_path);
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
    hash_type *hash    = hash_alloc(10);

    ecl_parse_file(hash , spec_file ,  type_map , endian_flip , VERB_NAME);  printf("\n");
    ecl_parse_write_decl(hash , type_map , "fsm" , include_path);

    {
      hash_type *special = hash_alloc(10);
      hash_insert_string_copy(special , "KEYWORDS" , "ikeywords = ihead");
      hash_insert_string_copy(special , "WGNAMES"  , "iwgnames  = ihead");
      hash_insert_string_copy(special , "UNITS"    , "iunits    = ihead");
      ecl_parse_write_read_eclipse(hash , type_map , special, "fsm" , include_path , "headtype" , "headsize" , "ihead");
      hash_free(special);
    }
    hash_free(hash);
  }
  free(spec_file);
}


  

void ecl_parse_grid(const char *refcase_path , const char *ecl_base , const char *include_path, bool fmt_file , bool endian_flip) {
  char *grid_file;
  ecl_fstate_type *grid;
  if (fmt_file) {
    grid_file = malloc(strlen(refcase_path) + strlen(ecl_base) + 9);
    sprintf(grid_file  , "%s/%s.FEGRID" , refcase_path , ecl_base);
  } else {
    grid_file = malloc(strlen(refcase_path) + strlen(ecl_base) + 8);
    sprintf(grid_file , "%s/%s.EGRID" , refcase_path , ecl_base);
  } 
  printf("Parsing: %s\n",grid_file); 
  if (fmt_file)
    grid = ecl_fstate_load_unified(grid_file , ECL_FORMATTED , endian_flip);
  else
    grid = ecl_fstate_load_unified(grid_file , ECL_BINARY    , endian_flip);
  
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

    fileH = fopen_printf(inc_file );
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
  ecl_parse_grid(refcase_path , eclbase , include_path , fmt_file , endian_flip);

  free_type_map(type_map);
}


