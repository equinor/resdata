#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <util.h>
#include <hash.h>
#include <multz_config.h>
#include <enkf_config.h>
#include <enkf_config_node.h>
#include <pathv.h>
#include <ecl_static_kw_config.h>
#include <enkf_types.h>
#include <well_config.h>
#include <field_config.h>
#include <equil_config.h>
#include <multz_config.h>
#include <multflt_config.h>
#include <well_obs.h>
#include <obs_node.h>
#include <obs_data.h>
#include <hist.h>
  

struct enkf_config_struct {
  int  		    ens_size;
  hash_type        *config_hash;
  hash_type        *obs_hash;
  int               enspath_depth;
  int               eclpath_depth;
  bool              endian_swap;
  int               Nwells;
  char            **well_list;
};


/*****************************************************************/




enkf_impl_type enkf_config_impl_type(const enkf_config_type *enkf_config, const char * ecl_kw_name) {
  enkf_impl_type impl_type;

  if (hash_has_key(enkf_config->config_hash , ecl_kw_name)) {
    enkf_config_node_type * node = hash_get(enkf_config->config_hash , ecl_kw_name);
    impl_type = enkf_config_node_get_impl_type(node);
  } else
    impl_type = STATIC;

  return impl_type;
}



/*****************************************************************/
static void enkf_config_realloc_well_list(enkf_config_type * enkf_config) {
  enkf_config->well_list = realloc(enkf_config->well_list , enkf_config->Nwells * sizeof * enkf_config->well_list);
}

bool enkf_config_get_endian_swap(const enkf_config_type * enkf_config) { return enkf_config->endian_swap; }

int enkf_config_get_eclpath_depth(const enkf_config_type * enkf_config) { return enkf_config->eclpath_depth; }

int enkf_config_get_enspath_depth(const enkf_config_type * enkf_config) { return enkf_config->enspath_depth; }


enkf_config_type * enkf_config_alloc(int enspath_depth , int eclpath_depth, bool endian_swap) {
  enkf_config_type * enkf_config = malloc(sizeof *enkf_config);
  enkf_config->config_hash = hash_alloc(10);
  enkf_config->obs_hash    = hash_alloc(10);
  
  enkf_config->eclpath_depth = eclpath_depth;
  enkf_config->enspath_depth = enspath_depth;
  enkf_config->endian_swap   = endian_swap;
  enkf_config->Nwells        = 0;
  enkf_config->well_list     = NULL;
  enkf_config_realloc_well_list(enkf_config);
  return enkf_config;
}



bool enkf_config_has_key(const enkf_config_type * enkf_config , const char * key) {
  return hash_has_key(enkf_config->config_hash , key);
}


const char ** enkf_config_get_well_list_ref(const enkf_config_type * config , int *Nwells) {
  *Nwells = config->Nwells;
  return (const char **) config->well_list;
}


void enkf_config_add_well(enkf_config_type * enkf_config , const char *well_name , int size, const char ** var_list) {
  enkf_config_add_type(enkf_config , well_name , ecl_summary , WELL,
		       well_config_alloc(well_name , well_name , size , var_list));


  enkf_config->Nwells++;
  enkf_config_realloc_well_list(enkf_config);
  enkf_config->well_list[enkf_config->Nwells - 1] = util_alloc_string_copy(well_name);
}




void enkf_config_add_type(enkf_config_type * enkf_config, 
			  const char * key , 
			  enkf_var_type enkf_type , 
			  enkf_impl_type impl_type , 
			  const void *data) {
  if (enkf_config_has_key(enkf_config , key)) {
    fprintf(stderr,"%s: a configuration object:%s has already been added - aborting \n",__func__ , key);
    abort();
  }

  {
    config_free_ftype              * freef;
    config_get_serial_size_ftype   * get_serial_size;
    config_set_serial_offset_ftype * set_serial_offset;
    switch(impl_type) {
    case(FIELD):
      freef             = field_config_free__;
      get_serial_size   = field_config_get_serial_size__;
      set_serial_offset = field_config_set_serial_offset__;
      break;
    case(MULTZ):
      freef             = multz_config_free__;
      get_serial_size   = multz_config_get_serial_size__;
      set_serial_offset = multz_config_set_serial_offset__;
      break;
    case(WELL):
      freef             = well_config_free__;
      get_serial_size   = well_config_get_serial_size__;
      set_serial_offset = well_config_set_serial_offset__;
      break;
    case(MULTFLT):
      freef             = multflt_config_free__;
      get_serial_size   = multflt_config_get_serial_size__;
      set_serial_offset = multflt_config_set_serial_offset__;
      break;
    case(EQUIL):
      freef             = equil_config_free__;
      get_serial_size   = equil_config_get_serial_size__;
      set_serial_offset = equil_config_set_serial_offset__;
      break;
    case(STATIC):
      freef             = ecl_static_kw_config_free__;
      get_serial_size   = NULL;
      set_serial_offset = NULL;
      break;
    default:
      fprintf(stderr,"%s : invalid implementation type: %d - aborting \n",__func__ , impl_type);
      abort();
    }
    {
      enkf_config_node_type * node = enkf_config_node_alloc(enkf_type , impl_type , data , freef , get_serial_size , set_serial_offset);
      hash_insert_hash_owned_ref(enkf_config->config_hash , key , node , enkf_config_node_free__);
    }
  }
}



void enkf_config_add_type0(enkf_config_type * enkf_config , const char *key , int size, enkf_var_type enkf_type , enkf_impl_type impl_type) {
  switch(impl_type) {
  case(STATIC):
    enkf_config_add_type(enkf_config , key , enkf_type , impl_type , ecl_static_kw_config_alloc(size , key , key));
    break;
  case(FIELD):
    /*
      enkf_config_add_type(enkf_config , key , enkf_type , impl_type , field_config_alloc(size , key , key)   , field_config_free__ , field_config_get_size__);
    */
    fprintf(stderr,"%s: Can not add FIELD config objects like:%s on the run - these must be from the main program with enkf_config_add_type - sorry.\n",__func__ , key);
    abort();
    break;
  default:
    fprintf(stderr,"%s only STATIC and FIELD types are implemented - aborting \n",__func__);
    abort();
  }
}



void enkf_config_free(enkf_config_type * enkf_config) {  
  hash_free(enkf_config->config_hash);
  hash_free(enkf_config->obs_hash);
  {
    int i;
    for (i=0; i < enkf_config->Nwells; i++)
      free(enkf_config->well_list[i]);
    free(enkf_config->well_list);
  }
  free(enkf_config);
}



const enkf_config_node_type * enkf_config_get_ref(const enkf_config_type * config, const char * key) {
  if (hash_has_key(config->config_hash , key)) {
    enkf_config_node_type * node = hash_get(config->config_hash , key);
    return node;
  } else {
    fprintf(stderr,"%s: config node:%s does not exist \n",__func__ , key);
    abort();
  }
}






