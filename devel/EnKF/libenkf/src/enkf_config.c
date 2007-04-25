#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <util.h>
#include <hash.h>
#include <enkf_config.h>



/*
  path:

  ens_root_path/ens_path2 = full_ens_path

*/
  

struct enkf_config_struct {
  int  		    ens_size;
  char 		   *ecl_root_path;
  char 		   *ens_root_path;
  char             *ens_path2;
  char             *full_ens_path;
  char             *ecl_path2;
  char             *full_ecl_path;
  hash_type        *config_hash;
  hash_type        *enkf_ecl_kw;  
};


typedef struct {
  config_set_ensfile_ftype *set_ensfile;
  config_set_eclfile_ftype *set_eclfile;
  config_free_ftype        *freef;

  void *data;
} enkf_config_node_type;



/*****************************************************************/
/* enkf_config_node_type - entirely static. */

static enkf_config_node_type * enkf_config_node_alloc(const void *data, 
						      config_set_ensfile_ftype * set_ensfile , 
						      config_set_eclfile_ftype * set_eclfile , 
						      config_free_ftype        * freef ) {      
  
  enkf_config_node_type * node = malloc( sizeof *node);
  node->data = (void *) data;
  node->set_ensfile = set_ensfile;
  node->set_eclfile = set_eclfile;
  node->freef       = freef;
  
  return node;
}

static void enkf_config_node_free(enkf_config_node_type * node) {
  if (node->freef != NULL) node->freef(node->data);
  free(node);
}

static void enkf_config_node_free__(void * void_node) {
  enkf_config_node_type * node = (enkf_config_node_type *) void_node;
  enkf_config_node_free(node);
}


static void enkf_config_node_set_ensfile(enkf_config_node_type * node, const char * ens_path) {
  if (node->set_ensfile == NULL) {
    fprintf(stderr,"%s: called with set_ensfile function pointer=NULL - aborting \n",__func__);
    abort();
  }
  if (ens_path != NULL) 
    node->set_ensfile(node->data , ens_path);
}

static void enkf_config_node_set_eclfile(enkf_config_node_type * node, const char * ecl_path) {
  if (node->set_eclfile == NULL) {
    fprintf(stderr,"%s: called with set_eclfile function pointer=NULL - aborting \n",__func__);
    abort();
  }
  if (ecl_path != NULL)
    node->set_eclfile(node->data , ecl_path);
}


/*****************************************************************/


bool enkf_config_restart_kw(const enkf_config_type *enkf_config, const char * ecl_kw_name) {
  if (hash_has_key(enkf_config->enkf_ecl_kw , ecl_kw_name))
    return true;
  else
    return false;
}


void enkf_config_add_enkf_kw(const enkf_config_type *enkf_config, const char *ecl_kw_name) {
  hash_insert_int(enkf_config->enkf_ecl_kw , ecl_kw_name , 1); 
}


/*****************************************************************/




static void enkf_config_reset_ens_path(enkf_config_type * enkf_config) {
  if (enkf_config->ens_root_path != NULL) {
    if (enkf_config->ens_path2 != NULL) {
      enkf_config->full_ens_path = util_realloc_full_path(enkf_config->full_ens_path , enkf_config->ens_root_path , enkf_config->ens_path2);
      util_make_path(enkf_config->full_ens_path);
      enkf_config_set_ensfile(enkf_config);
    } else
      util_make_path(enkf_config->ens_root_path);
  }
}



void enkf_config_set_ens_root_path(enkf_config_type * enkf_config , const char * ens_root_path) {
  enkf_config->ens_root_path = util_realloc_string_copy(enkf_config->ens_root_path , ens_root_path);
  enkf_config_reset_ens_path(enkf_config);
}



void enkf_config_set_ens_path(enkf_config_type * enkf_config , const char * ens_path) {
  enkf_config->ens_path2 = util_realloc_string_copy(enkf_config->ens_path2 , ens_path);
  enkf_config_reset_ens_path(enkf_config);
}



static void enkf_config_reset_ecl_path(enkf_config_type * enkf_config) {
  if (enkf_config->ecl_root_path != NULL) {
    if (enkf_config->ecl_path2 != NULL) {
      enkf_config->full_ecl_path = util_realloc_full_path(enkf_config->full_ecl_path , enkf_config->ecl_root_path , enkf_config->ecl_path2);
      util_make_path(enkf_config->full_ecl_path);
      enkf_config_set_eclfile(enkf_config);
    } else
      util_make_path(enkf_config->ecl_root_path);
  }
}



void enkf_config_set_ecl_root_path(enkf_config_type * enkf_config , const char * ecl_root_path) {
  enkf_config->ecl_root_path = util_realloc_string_copy(enkf_config->ecl_root_path , ecl_root_path);
  enkf_config_reset_ecl_path(enkf_config);
}



void enkf_config_set_ecl_path(enkf_config_type * enkf_config , const char * ecl_path) {
  enkf_config->ecl_path2 = util_realloc_string_copy(enkf_config->ecl_path2 , ecl_path);
  enkf_config_reset_ecl_path(enkf_config);
}

/*****************************************************************/

enkf_config_type * enkf_config_alloc(const char * ens_root_path, const char * ens_path , const char *ecl_root_path) {
  enkf_config_type * enkf_config = malloc(sizeof *enkf_config);
  enkf_config->config_hash = hash_alloc(10);
  enkf_config->enkf_ecl_kw = hash_alloc(10);

  enkf_config->ens_root_path = NULL;
  enkf_config->ens_path2     = NULL;
  enkf_config->full_ens_path = NULL;

  enkf_config->ecl_root_path = NULL;
  enkf_config->ecl_path2     = NULL;
  enkf_config->full_ecl_path = NULL;
  
  enkf_config_set_ecl_root_path(enkf_config , ecl_root_path );
  enkf_config_set_ens_path(enkf_config , ens_path );
  enkf_config_set_ens_root_path(enkf_config , ens_root_path );

  printf("Har satt ens_path: %s \n",enkf_config->full_ens_path);

  return enkf_config;
}




void enkf_config_add_type(enkf_config_type * enkf_config, const char * key , const void *data ,  
			  config_set_ensfile_ftype * set_ensfile , 
			  config_set_eclfile_ftype * set_eclfile , 
			  config_free_ftype        * freef ) {
  if (hash_has_key(enkf_config->config_hash , key)) {
    fprintf(stderr,"%s: a configuration object:%s has already been added - aborting \n",__func__ , key);
    abort();
  }
  {
    enkf_config_node_type * node = enkf_config_node_alloc(data , set_ensfile , set_eclfile , freef);
    hash_insert_managed_ref(enkf_config->config_hash , key , node , enkf_config_node_free__);
    enkf_config_node_set_ensfile(node , enkf_config->full_ens_path);
    enkf_config_node_set_eclfile(node , enkf_config->full_ecl_path);
  }
}


void enkf_config_free(enkf_config_type * enkf_config) {  
  if (enkf_config->ens_path2 != NULL) 	  free(enkf_config->ens_path2);
  if (enkf_config->ecl_root_path != NULL) free(enkf_config->ecl_root_path);
  if (enkf_config->ens_root_path != NULL) free(enkf_config->ens_root_path);
  if (enkf_config->full_ens_path != NULL) free(enkf_config->full_ens_path);
  if (enkf_config->full_ecl_path != NULL) free(enkf_config->full_ecl_path);
  if (enkf_config->ecl_path2 != NULL) 	  free(enkf_config->ecl_path2);

  hash_free(enkf_config->config_hash);
  hash_free(enkf_config->enkf_ecl_kw);
  free(enkf_config);
}


const void * enkf_config_get_node_value(const enkf_config_type * config, const char * key) {
  if (hash_has_key(config->config_hash , key)) {
    enkf_config_node_type * node = hash_get(config->config_hash , key);
    return node->data;
  } else {
    fprintf(stderr,"%s: config node:%s does not exist \n",__func__ , key);
    abort();
  }
}



void enkf_config_set_ensfile(enkf_config_type * config) {
  char **keylist = hash_alloc_keylist(config->config_hash);
  int i;
  for (i=0; i < hash_get_size(config->config_hash); i++) {
    enkf_config_node_type * node = hash_get(config->config_hash , keylist[i]);
    enkf_config_node_set_ensfile(node , config->full_ens_path);
  }
  hash_free_ext_keylist(config->config_hash , keylist);
}


void enkf_config_set_eclfile(enkf_config_type * config) {
  char **keylist = hash_alloc_keylist(config->config_hash);
  int i;
  for (i=0; i < hash_get_size(config->config_hash); i++) {
    enkf_config_node_type * node = hash_get(config->config_hash , keylist[i]);
    enkf_config_node_set_eclfile(node , config->full_ecl_path);
  }
  hash_free_ext_keylist(config->config_hash , keylist);
}


