#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <field_config.h>
#include <layer_config.h>
#include <util.h>


/*
  data_owner = false: In this case you must allocate with a
                      field_config_type object, and index_map and data
                      pointers are extracted from this. If you want to
                      use the layer object to update a field instance
                      you must do it this way.

   
  data_owner = true: In this case the layer itself owns the index_map
                     and data. In this case it is *NOT* possible to
                     inject into the layer of a field. The layer can
                     be both compact and sparce.
		     
  		     
   
*/

static layer_config_type * __layer_config_alloc(const char * ecl_kw , const char * eclfile , const char * ensfile , const field_config_type * target_config , const int * index_map , int nx , int ny) {
  layer_config_type * config = malloc(sizeof * config);
  config->eclfile     = util_alloc_string_copy(eclfile);
  config->ensfile     = util_alloc_string_copy(ensfile);
  config->ecl_kw_name = util_alloc_string_copy(ecl_kw);

  config->serial_size = -1;
  
  if (target_config != NULL) {
    config->data_owner 	  = false;
    config->compact    	  = false;
    config->target_config = target_config;
    {
      int nz;
      field_config_get_dims(target_config , &config->nx , &config->ny , &nz);
    }
  } else {
    config->nx = nx;
    config->ny = ny;
    config->data_owner 	  = true;
    if (index_map == NULL) {
      config->compact = true;
      config->index_map = NULL;
      config->serial_size = nx * ny;
      config->data_size = nx * ny;
    } else {
      config->index_map = index_map;
      config->compact   = false;
    }
  }

  /*
    Must determine size from index_map
  */
  if (config->serial_size < 0) {
    int index;
    config->serial_size = 0;
    for (index=0; index < config->nx*config->ny; index++)
      if (config->index_map[index] >= 0)
	config->serial_size++;
  }
  return config;
}


layer_config_type * layer_config_alloc_compact()     { return NULL; }
layer_config_type * layer_config_alloc_sparse()      { return NULL; }
layer_config_type * layer_config_alloc_field_layer() { return NULL; }



