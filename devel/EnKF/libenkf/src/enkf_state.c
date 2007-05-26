#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <list.h>
#include <hash.h>
#include <fortio.h>
#include <util.h>
#include <pathv.h>
#include <ecl_kw.h>
#include <ecl_block.h>
#include <ecl_fstate.h>
#include <list_node.h>
#include <enkf_node.h>
#include <enkf_state.h>
#include <enkf_config.h>
#include <enkf_types.h>
#include <ecl_static_kw.h>
#include <enkf_ecl_kw.h>

#include <multz.h>
#include <multflt.h>
#include <equil.h>


struct enkf_state_struct {
  list_type    	   * node_list;
  hash_type    	   * node_hash;
  hash_type    	   * enkf_types;
  hash_type    	   * enkf_var_types;
  pathv_type   	   * eclpath;
  pathv_type   	   * enspath;
  enkf_config_type * config;
  double           * serial_data;
  char             * eclbase;
};


/*****************************************************************/

#define ENKF_STATE_APPLY(node_func)                                      \
void enkf_state_ ## node_func(enkf_state_type * enkf_state , int mask) { \
  list_node_type *list_node;                                             \
  list_node = list_get_head(enkf_state->node_list);                      \
  while (list_node != NULL) {                                            \
    enkf_node_type *enkf_node = list_node_value_ptr(list_node);          \
    if (enkf_node_include_type(enkf_node , mask))                        \
      enkf_node_ ## node_func (enkf_node);                               \
    list_node = list_node_get_next(list_node);                           \
  }                                                                      \
}

/*****************************************************************/

#define ENKF_STATE_APPLY2(node_func)                                     \
void enkf_state_ ## node_func(enkf_state_type * enkf_state , const enkf_state_type *enkf_state2 , int mask) { \
  list_node_type *list_node;                                            \
  list_node_type *list_node2;                                           \
  list_node  = list_get_head(enkf_state->node_list);                    \
  list_node2 = list_get_head(enkf_state2->node_list);                   \
  while (list_node != NULL) {                                           \
    enkf_node_type *enkf_node  = list_node_value_ptr(list_node);        \
    enkf_node_type *enkf_node2 = list_node_value_ptr(list_node2);       \
    if (enkf_node_include_type(enkf_node , mask))                       \
      enkf_node_ ## node_func (enkf_node , enkf_node2);                 \
    list_node  = list_node_get_next(list_node);                         \
    list_node2 = list_node_get_next(list_node2);                        \
  }                                                                     \
}

/*****************************************************************/

#define ENKF_STATE_APPLY_SCALAR(node_func)                                     \
void enkf_state_ ## node_func(enkf_state_type * enkf_state , double scalar, int mask) { \
  list_node_type *list_node;                                            \
  list_node  = list_get_head(enkf_state->node_list);                    \
  while (list_node != NULL) {                                           \
    enkf_node_type *enkf_node  = list_node_value_ptr(list_node);        \
    if (enkf_node_include_type(enkf_node , mask))                       \
      enkf_node_ ## node_func (enkf_node , scalar);                     \
    list_node  = list_node_get_next(list_node);                         \
  }                                                                     \
}

/*****************************************************************/

#define ENKF_STATE_APPLY_PATH(node_func)                                     \
void enkf_state_ ## node_func(enkf_state_type * enkf_state , const char *path, int mask) { \
  list_node_type *list_node;                                            \
  list_node  = list_get_head(enkf_state->node_list);                    \
  while (list_node != NULL) {                                           \
    enkf_node_type *enkf_node  = list_node_value_ptr(list_node);        \
    if (enkf_node_include_type(enkf_node , mask))                       \
      enkf_node_ ## node_func (enkf_node , path);                       \
    list_node  = list_node_get_next(list_node);                         \
  }                                                                     \
}

/*****************************************************************/


void enkf_state_iset_enspath(enkf_state_type * enkf_state , int i , const char *path) {
  pathv_iset(enkf_state->enspath , i , path);
}

const char * enkf_state_get_enspath_ref(const enkf_state_type * enkf_state) {
  return pathv_get_ref(enkf_state->enspath);
}


void enkf_state_iset_eclpath(enkf_state_type * enkf_state , int i , const char *path) {
  pathv_iset(enkf_state->eclpath , i , path);
}


enkf_state_type *enkf_state_alloc(const enkf_config_type * config , const char * eclbase) {
  enkf_state_type * enkf_state = malloc(sizeof *enkf_state);
  
  enkf_state->config         = (enkf_config_type *) config;
  enkf_state->node_list      = list_alloc();
  enkf_state->node_hash      = hash_alloc(10);
  enkf_state->enkf_types     = hash_alloc(10);
  enkf_state->enkf_var_types = hash_alloc(10);
  enkf_state->eclpath        = pathv_alloc(enkf_config_get_eclpath_depth(config) , NULL);
  enkf_state->enspath        = pathv_alloc(enkf_config_get_enspath_depth(config) , NULL);
  enkf_state->eclbase        = util_alloc_string_copy(eclbase);
  enkf_state->serial_data    = NULL;

  hash_insert_int(enkf_state->enkf_types     , "MULTZ"      , MULTZ);
  hash_insert_int(enkf_state->enkf_types     , "MULTFLT"    , MULTFLT);
  hash_insert_int(enkf_state->enkf_types     , "EQUIL"      , EQUIL);
  
  hash_insert_int(enkf_state->enkf_var_types , "MULTZ"      , parameter);
  hash_insert_int(enkf_state->enkf_var_types , "MULTFLT"    , parameter);
  hash_insert_int(enkf_state->enkf_var_types , "EQUIL"      , parameter);
  
  return enkf_state;
}

/*
  hash_node -> list_node -> enkf_node -> {Actual enkf object: multz_type/equil_type/multflt_type/...}
*/


static void enkf_state_add_node__(enkf_state_type * enkf_state , const char * node_name , const enkf_node_type * node) {
  list_node_type *list_node = list_append_ref(enkf_state->node_list , node);
  /*
    The hash contains a pointer to a list_node structure, which contain a pointer
    to an enkf_node which contains a pointer to the actual enkf object.
  */
  hash_insert_ref(enkf_state->node_hash , node_name  , list_node);
}


enkf_state_type * enkf_state_copyc(const enkf_state_type * src) {
  enkf_state_type * new = enkf_state_alloc(src->config , src->eclbase);
  list_node_type *list_node;                                          
  list_node = list_get_head(src->node_list);                     
  printf("Totalt antall noder: %d \n",list_get_size(src->node_list));

  while (list_node != NULL) {                                           
    printf("Kopierer node .... \n");
    {
      enkf_node_type *enkf_node = list_node_value_ptr(list_node);         
      enkf_node_type *new_node  = enkf_node_copyc(enkf_node);
      enkf_state_add_node__(new , enkf_node_get_key_ref(new_node) , new_node);
      list_node = list_node_get_next(list_node);                          
    }
  }
  
  return new;
}



static bool enkf_state_has_node(const enkf_state_type * enkf_state , const char * node_key) {
  return hash_has_key(enkf_state->node_hash , node_key);
}



void enkf_state_add_node(enkf_state_type * enkf_state , const char * type_str , const char * node_name) {
  if (enkf_state_has_node(enkf_state , node_name)) {
    fprintf(stderr,"%s: node:%s already added  - aborting \n",__func__ , node_name);
    abort();
  }

  if (!hash_has_key(enkf_state->enkf_types , type_str)) {
    fprintf(stderr,"%s: type:%s is not recognized - aborting \n",__func__ , type_str);
    abort();
  }

  if (!enkf_config_has_key(enkf_state->config , node_name)) {
    fprintf(stderr,"%s could not find configuration object for:%s - aborting \n",__func__ , node_name);
    abort();
  }
  
  {
    enkf_type     type     = hash_get_int(enkf_state->enkf_types     , type_str);
    enkf_var_type var_type = hash_get_int(enkf_state->enkf_var_types , type_str);
    const void *config     = enkf_config_get_ref(enkf_state->config , node_name);
    enkf_node_type *enkf_node;
    
    switch (type) {
    case(MULTZ):
      enkf_node = enkf_node_alloc(type_str , var_type , config , multz_alloc__   , multz_ecl_write__ , multz_alloc_ensfile__ 
				  , multz_ens_read__ , multz_ens_write__ , multz_swapout__ , 
				  multz_swapin__, 
				  multz_copyc__ , multz_sample__ , multz_free__);
      break;
    case(MULTFLT):
      enkf_node = enkf_node_alloc(type_str , var_type , config , multflt_alloc__ , multflt_ecl_write__ , multflt_alloc_ensfile__ , 
				  multflt_ens_read__ , multflt_ens_write__ , multflt_swapout__, 
				  multz_swapin__, multflt_copyc__ , multflt_sample__ , multflt_free__);
      break;
    case(EQUIL):
      enkf_node = enkf_node_alloc(type_str , var_type , config , equil_alloc__ , equil_ecl_write__ , equil_alloc_ensfile__, 
				  equil_ens_read__ , equil_ens_write__ , equil_swapout__ , equil_swapin__ , equil_copyc__ , equil_sample__ , equil_free__);
      break;
    default:
      fprintf(stderr,"%s: Internal programming error --- aborting \n",__func__);
      abort();
    }
    enkf_state_add_node__(enkf_state , node_name , enkf_node);
  }
}


static void enkf_state_load_ecl_restart__(enkf_state_type * enkf_state , const ecl_block_type *ecl_block) {
  ecl_kw_type * ecl_kw = ecl_block_get_first_kw(ecl_block);
  while (ecl_kw != NULL) {
    const char *kw        = ecl_kw_get_header_ref(ecl_kw);
    const bool restart_kw = enkf_config_restart_kw(enkf_state->config , kw);

    if (!enkf_config_has_key(enkf_state->config , kw)) {
      if (restart_kw)
	enkf_config_add_type(enkf_state->config , kw , enkf_ecl_kw_config_alloc(ecl_kw_get_size(ecl_kw) , kw , kw) , enkf_ecl_kw_config_free__ , enkf_ecl_kw_config_get_size__);
      else
	enkf_config_add_type(enkf_state->config , kw , ecl_static_kw_config_alloc(ecl_kw_get_size(ecl_kw) , kw , kw) , ecl_static_kw_config_free__ , ecl_static_kw_config_get_size__);
    }
    
    if (!enkf_state_has_node(enkf_state , kw)) {
      const enkf_ecl_kw_config_type *config = enkf_config_get_ref(enkf_state->config , kw);
      enkf_node_type * enkf_node;
      
      if (restart_kw) 
	enkf_node = enkf_node_alloc(kw , ecl_restart , config , enkf_ecl_kw_alloc__ , NULL /* ecl_write */ , 
				    enkf_ecl_kw_alloc_ensfile__ , enkf_ecl_kw_ens_read__ , 
				    enkf_ecl_kw_ens_write__ , enkf_ecl_kw_swapout__ , enkf_ecl_kw_swapin__ , 
				    enkf_ecl_kw_copyc__ , NULL /* sample */ , enkf_ecl_kw_free__);
      else
	enkf_node = enkf_node_alloc(kw , ecl_static  , config , ecl_static_kw_alloc__ , NULL , 
				    ecl_static_kw_alloc_ensfile__ , ecl_static_kw_ens_read__ , 
				    ecl_static_kw_ens_write__ , ecl_static_kw_swapout__, ecl_static_kw_swapin__ , 
				    ecl_static_kw_copyc__ , NULL , ecl_static_kw_free__);
      
      enkf_state_add_node__(enkf_state , kw , enkf_node);
    } 
    
    {
      enkf_node_type * enkf_node = enkf_state_get_node(enkf_state , kw);
      
      if (restart_kw) 
	enkf_ecl_kw_get_data(enkf_node_value_ptr(enkf_node) , ecl_kw);    
      else 
	ecl_static_kw_init(enkf_node_value_ptr(enkf_node) , ecl_kw);
    }
    
    ecl_kw = ecl_block_get_next_kw(ecl_block , ecl_kw);
  }
}



void enkf_state_load_ecl_restart(enkf_state_type * enkf_state , const char * restart_file , bool endian_swap , bool unified , int time_step) {
  bool at_eof;
  ecl_block_type       * ecl_block;
  bool fmt_file        = ecl_fstate_fmt_file(restart_file);
  fortio_type * fortio = fortio_open(restart_file , "r" , endian_swap);
  
  if (unified)
    ecl_block_fseek(time_step , fmt_file , true , fortio);
  
  ecl_block = ecl_block_alloc(time_step , 10 , fmt_file , endian_swap);
  ecl_block_fread(ecl_block , fortio , &at_eof);
  fortio_close(fortio);

  enkf_state_load_ecl_restart__(enkf_state , ecl_block);
  ecl_block_free(ecl_block);
}



void enkf_state_ecl_write(const enkf_state_type * enkf_state , const char * restart_file , int mask , bool fmt_file , bool endian_swap ) {
  const int buffer_size = 16384;
  fortio_type * fortio = fortio_open(restart_file , "w" , endian_swap);
  void *buffer = malloc(buffer_size);
  list_node_type *list_node;                                            

  list_node = list_get_head(enkf_state->node_list);                     
  while (list_node != NULL) {                                           
    enkf_node_type * enkf_node = list_node_value_ptr(list_node);         

    if (enkf_node_include_type(enkf_node , mask)) {

      if (enkf_node_include_type(enkf_node , ecl_restart)) {      
	ecl_kw_type * ecl_kw = enkf_ecl_kw_alloc_ecl_kw(enkf_node_value_ptr(enkf_node) , fmt_file , endian_swap);
	ecl_kw_fwrite(ecl_kw , fortio);
	ecl_kw_free(ecl_kw);
      } else if (enkf_node_include_type(enkf_node , ecl_static)) {
	/*
	  This is extremely fragile - because it requires the enspath
	  variable still points to the same place as the last time
	  ens_write() was called...  Maybe it would be better to use
	  swapin / swapout - and store (swap) filename in the node
	  variable.
	*/

	char * ens_name      = enkf_node_alloc_ensfile(enkf_node , pathv_get_ref(enkf_state->enspath));
	FILE * target_stream = fortio_get_FILE(fortio);
	FILE * src_stream    = enkf_util_fopen_r(ens_name , __func__);
	
	util_copy_stream(src_stream , target_stream , buffer_size , buffer);
	fclose(src_stream);
	free(ens_name);
	
      } else if (enkf_node_include_type(enkf_node , parameter))
        enkf_node_ecl_write(enkf_node , pathv_get_ref(enkf_state->eclpath));
    }
    list_node = list_node_get_next(list_node);
  }
  fortio_close(fortio);
  free(buffer);
}



void enkf_state_ens_write(const enkf_state_type * enkf_state , int mask) {
  const char *path = enkf_state_get_enspath_ref(enkf_state);
  list_node_type *list_node;                                            
  list_node  = list_get_head(enkf_state->node_list);                    
  while (list_node != NULL) {                                           
    enkf_node_type *enkf_node = list_node_value_ptr(list_node);        
    if (enkf_node_include_type(enkf_node , mask))                       
      enkf_node_ens_write(enkf_node , path);                       
    list_node  = list_node_get_next(list_node);                         
  }                                                                     
}



void enkf_state_free_nodes(enkf_state_type * enkf_state, int mask) {
  list_node_type *list_node;                                            
  list_node = list_get_head(enkf_state->node_list);                     
  while (list_node != NULL) {                                           
    list_node_type * next_node = list_node_get_next(list_node);
    enkf_node_type *enkf_node  = list_node_value_ptr(list_node);         

    if (enkf_node_include_type(enkf_node , mask))      
      enkf_state_del_node(enkf_state , enkf_node_get_key_ref(enkf_node));
    
    list_node = next_node;
  } 
}


void enkf_state_free(enkf_state_type *enkf_state) {
  enkf_state_free_nodes(enkf_state , all_types);
  list_free(enkf_state->node_list);
  hash_free(enkf_state->node_hash);
  hash_free(enkf_state->enkf_types);
  hash_free(enkf_state->enkf_var_types);
  pathv_free(enkf_state->eclpath);
  pathv_free(enkf_state->enspath);
  free(enkf_state->eclbase);
  if (enkf_state->serial_data != NULL)
    free(enkf_state->serial_data);
  free(enkf_state);
}



enkf_node_type * enkf_state_get_node(const enkf_state_type * enkf_state , const char * node_key) {
  if (hash_has_key(enkf_state->node_hash , node_key)) {
    list_node_type * list_node = hash_get(enkf_state->node_hash , node_key);
    enkf_node_type * enkf_node = list_node_value_ptr(list_node);
    return enkf_node;
  } else {
    fprintf(stderr,"%s: node:%s not found in state object - aborting \n",__func__ , node_key);
    abort();
  }
}



void enkf_state_del_node(enkf_state_type * enkf_state , const char * node_key) {
  if (hash_has_key(enkf_state->node_hash , node_key)) {
    list_node_type * list_node = hash_get(enkf_state->node_hash , node_key);
    enkf_node_type * enkf_node = list_node_value_ptr(list_node);
    
    list_del_node(enkf_state->node_list , list_node);
    hash_del(enkf_state->node_hash , node_key);
    enkf_node_free(enkf_node);  /* If a destructor is supplied - it is done here, not in the container classes .*/
    
  } else {
    fprintf(stderr,"%s: node:%s not found in state object - aborting \n",__func__ , node_key);
    abort();
  } 
}


/*****************************************************************/
/* Generatad functions - iterating through all members.          */
/*****************************************************************/

ENKF_STATE_APPLY_PATH(ens_read);
ENKF_STATE_APPLY(sample);
ENKF_STATE_APPLY(clear);
ENKF_STATE_APPLY_SCALAR(scale);
ENKF_STATE_APPLY2(imul);
ENKF_STATE_APPLY2(iadd);
ENKF_STATE_APPLY2(iaddsqr);
