#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <enkf_types.h>
#include <enkf_util.h>
#include <well_config.h>
#include <config.h>
#include <util.h>




int well_config_get_var_index(const well_config_type * config , const char * var) {
  int index , i;
  index = -1;
  i = 0;
  while (index < 0 && i < config->data_size) {
    if (strcmp(var , config->var_list[i]) == 0) index = i;
    i++;
  }
  return index;
}


const char ** well_config_get_var_list_ref(const well_config_type * config) { return (const char **) config->var_list; }


void well_config_add_var(well_config_type * config , const char * var) {
  int index = well_config_get_var_index(config , var);
  if (index >= 0) {
    fprintf(stderr,"%s: well variable:%s already added to well:%s - nothing done \n",__func__ , var , config->well_name);
    return;
  }
  /*
    All variables are (currently) active.
  */
  config->serial_size++;   
  config->data_size++;
  config->var_list = realloc(config->var_list , config->data_size * sizeof * config->var_list);
  config->var_list[config->data_size - 1] = util_alloc_string_copy(var);
}



well_config_type * well_config_alloc(const char * well_name , const char * ensfile , int size , const char ** var_list) {
  well_config_type * config = malloc(sizeof *config);

  config->eclfile     = NULL;
  config->ensfile     = util_alloc_string_copy(ensfile);
  config->well_name   = util_alloc_string_copy(well_name);
  config->data_size   = 0;
  config->serial_size = 0;
  config->var_list    = NULL;
  {
    int i;
    for (i=0; i < size; i++) 
      printf("var[%d] = %s \n",i , var_list[i]);

    for (i=0; i < size; i++) 
      well_config_add_var(config , var_list[i]);
    
  }
  return config;
}




void well_config_free(well_config_type * config) {
  CONFIG_FREE_STD_FIELDS;
  { 
    int i;
    for (i = 0; i < config->data_size; i++)
      free(config->var_list[i]);
    free(config->var_list);
  }
  free(config);
}


const char * well_config_get_ensfile_ref(const well_config_type * config) {
  return config->ensfile;
}

const char * well_config_get_well_name_ref(const well_config_type * config) {
  return config->well_name;
}




/*****************************************************************/
CONFIG_SET_ENSFILE(well);
CONFIG_SET_ENSFILE_VOID(well);
GET_SERIAL_SIZE(well)
GET_DATA_SIZE(well)
VOID_GET_SERIAL_SIZE(well)
VOID_CONFIG_FREE(well)
