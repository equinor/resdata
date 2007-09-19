#include <stdlib.h>
#include <string.h>
#include <util.h>
#include <ens_config.h>
#include <mult_config.h>
#include <enkf_util.h>
#include <config.h>
#include <logmode.h>
#include <trans_func.h>



static void mult_config_set_output_transform(mult_config_type * config) {
  int i;
  for (i=0; i < config->data_size; i++) 
    config->output_transform[i] = trans_func_lookup(config->output_transform_name[i]);
}



mult_config_type * mult_config_alloc_empty(int size, const char * eclfile , const char * ensfile) {
  mult_config_type *mult_config = malloc(sizeof *mult_config);
  mult_config->data_size   = size;
  mult_config->ecl_kw_name = NULL;
  mult_config->var_type    = parameter;
  
  mult_config->eclfile     = util_alloc_string_copy(eclfile);
  mult_config->ensfile     = util_alloc_string_copy(ensfile);
  mult_config->mean        = enkf_util_malloc(size * sizeof *mult_config->mean        , __func__);
  mult_config->std         = enkf_util_malloc(size * sizeof *mult_config->std         ,  __func__);
  mult_config->active      = enkf_util_malloc(size * sizeof *mult_config->active      , __func__);
  mult_config->logmode     = enkf_util_malloc(size * sizeof *mult_config->logmode      , __func__);
  mult_config->output_transform      = enkf_util_malloc(mult_config->data_size * sizeof * mult_config->output_transform      , __func__);
  mult_config->output_transform_name = enkf_util_malloc(mult_config->data_size * sizeof * mult_config->output_transform_name , __func__);
  mult_config->serial_size           = 0;

  return mult_config;
}


/*
mult_config_type * mult_config_alloc(int size, const char * eclfile , const char * ensfile) {
  mult_config_type *mult_config = __mult_config_alloc_empty(size , eclfile , ensfile);
  { 
    int i;
    for (i = 0; i < size; i++) {
      mult_config->output_transform_name = NULL;
      mult_config->mean[i]   = 1.0;
      mult_config->std[i]    = 0.25;
      mult_config->active[i] = true;
      if (mult_config->active[i])
	mult_config->serial_size++;
    }
  }
  
  mult_config_set_output_transform(mult_config);
  return mult_config;
}
*/


double mult_config_transform(const mult_config_type * config , int index , double value) {
  if (config->output_transform[index] == NULL)
    return value;
  else
    return config->output_transform[index](value);
}



void mult_config_fscanf_line(mult_config_type * config , int line_nr , FILE * stream) {
  char output_transform[128];
  int logmode    = 0;

  
  double   mu , sigma;
  int scan_count = fscanf(stream , "%lg  %lg  %d  %s" , &mu , &sigma , &logmode , output_transform);

  
  if (scan_count < 2 || scan_count > 4) {
    util_rewind_line(stream);
    fprintf(stderr,"%s error when loading line: %s - aborting \n",__func__ , util_fscanf_alloc_line(stream , NULL));
    abort();
  }

  config->active[line_nr]  		 = true;
  config->mean[line_nr]    		 = mu;
  config->std[line_nr]     		 = sigma;
  config->logmode[line_nr] 		 = logmode_alloc(10.0 , logmode);
  if (scan_count == 4)
    config->output_transform_name[line_nr] = util_alloc_string_copy(output_transform);
  else
    config->output_transform_name[line_nr] = NULL;
  
}


double mult_config_truncate(const mult_config_type * config , int i, double org_value) {
  double new_value = org_value;
  if (config->active[i]) 
    if (!logmode_logEnKF(config->logmode[i]))
      if (config->output_transform[i] == NULL)
	new_value = util_double_max(0.0 , org_value);
  
  return new_value;
}




const char * mult_config_get_ensfile_ref(const mult_config_type * mult_config) {
  return mult_config->ensfile;
}

const char * mult_config_get_eclfile_ref(const mult_config_type * mult_config) {
  return mult_config->eclfile;
}


void mult_config_free(mult_config_type * mult_config) {
  free(mult_config->eclfile);
  free(mult_config->ensfile);
  free(mult_config->mean);
  free(mult_config->std);
  free(mult_config->active);
  free(mult_config->logmode);
  util_free_string_list(mult_config->output_transform_name , mult_config->data_size);
  free(mult_config->output_transform);
  free(mult_config);
}



/*****************************************************************/

CONFIG_SET_ECLFILE(mult);
CONFIG_SET_ENSFILE(mult);
CONFIG_SET_ECLFILE_VOID(mult);
CONFIG_SET_ENSFILE_VOID(mult);
GET_SERIAL_SIZE(mult);
VOID_GET_SERIAL_SIZE(mult);
GET_DATA_SIZE(mult);
VOID_FUNC(mult_config_free , mult_config_type);
SET_SERIAL_OFFSET(mult);
VOID_SET_SERIAL_OFFSET(mult);
