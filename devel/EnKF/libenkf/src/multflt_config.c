#include <stdlib.h>
#include <string.h>
#include <util.h>
#include <ens_config.h>
#include <multflt_config.h>
#include <enkf_util.h>
#include <config.h>
#include <logmode.h>
#include <trans_func.h>



static void multflt_config_set_output_transform(multflt_config_type * config) {
  int i;
  for (i=0; i < config->data_size; i++) 
    config->output_transform[i] = trans_func_lookup(config->output_transform_name[i]);
}



static multflt_config_type * __multflt_config_alloc_empty(int size, const char * eclfile , const char * ensfile) {
  multflt_config_type *multflt_config = malloc(sizeof *multflt_config);
  multflt_config->data_size   = size;
  multflt_config->ecl_kw_name = NULL;
  multflt_config->var_type    = parameter;
  
  multflt_config->eclfile     = util_alloc_string_copy(eclfile);
  multflt_config->ensfile     = util_alloc_string_copy(ensfile);
  multflt_config->mean        = enkf_util_malloc(size * sizeof *multflt_config->mean        , __func__);
  multflt_config->std         = enkf_util_malloc(size * sizeof *multflt_config->std         ,  __func__);
  multflt_config->active      = enkf_util_malloc(size * sizeof *multflt_config->active      , __func__);
  multflt_config->logmode     = enkf_util_malloc(size * sizeof *multflt_config->logmode      , __func__);
  multflt_config->fault_names = enkf_util_malloc(size * sizeof *multflt_config->fault_names , __func__);
  multflt_config->output_transform 	= enkf_util_malloc(multflt_config->data_size * sizeof * multflt_config->output_transform      , __func__);
  multflt_config->output_transform_name = enkf_util_malloc(multflt_config->data_size * sizeof * multflt_config->output_transform_name , __func__);
  multflt_config->serial_size = 0;

  return multflt_config;
}



multflt_config_type * multflt_config_alloc(int size, const char * eclfile , const char * ensfile) {
  multflt_config_type *multflt_config = __multflt_config_alloc_empty(size , eclfile , ensfile);
  { 
    int i;
    for (i = 0; i < size; i++) {
      multflt_config->output_transform_name = NULL;
      multflt_config->mean[i]   = 1.0;
      multflt_config->std[i]    = 0.25;
      multflt_config->active[i] = true;
      multflt_config->fault_names[i] = util_alloc_string_copy("FAULT");
      if (multflt_config->active[i])
	multflt_config->serial_size++;
    }
  }

  multflt_config_set_output_transform(multflt_config);
  return multflt_config;
}



double multflt_config_transform(const multflt_config_type * config , int index , double value) {
  if (config->output_transform[index] == NULL)
    return value;
  else
    return config->output_transform[index](value);
}



multflt_config_type * multflt_config_fscanf_alloc(const char * filename , const char * eclfile , const char * ensfile) {
  multflt_config_type * config;
  FILE * stream = util_fopen(filename , "r");
  char * line   = NULL;
  int line_nr = 0;
  int size;
  bool at_eof;

  size = util_count_file_lines(stream);
  fseek(stream , 0L , SEEK_SET);
  config = __multflt_config_alloc_empty(size , eclfile , ensfile);
  do {
    line = util_fscanf_realloc_line(stream , &at_eof , line);
    if (!at_eof) {
      
      char name[128];  /* UGGLY HARD CODED LIMIT */
      char output_transform[128];
      double   mu , sigma;
      int logmode;
      int scan_count = sscanf(line , "%s  %lg  %lg  %d %s" , name , &mu , &sigma , &logmode , output_transform);
      if (scan_count == 5) {
	config->mean[line_nr]    	       = mu;
	config->std[line_nr]     	       = sigma;
	config->active[line_nr]  	       = true;
	config->logmode[line_nr] 	       = logmode_alloc(10.0 , logmode);
	config->fault_names[line_nr]           = util_alloc_string_copy(name);
	config->output_transform_name[line_nr] = util_alloc_string_copy(output_transform);
	if (config->active[line_nr])
	  config->serial_size++;
	line_nr++;
      } else {
	fprintf(stderr,"%s: line %d in config file %s not recognized as valid format - aborting\n",__func__ , line_nr + 1 , filename);
	abort();
      }
      
    }
  } while (! at_eof);
  free(line);
  fclose(stream);
  multflt_config_set_output_transform(config);
  return config;
}


double multflt_config_truncate(const multflt_config_type * config , int i, double org_value) {
  double new_value = org_value;
  if (config->active[i]) 
    if (!logmode_logEnKF(config->logmode[i]))
      if (config->output_transform[i] == NULL)
	new_value = util_double_max(0.0 , org_value);
  
  return new_value;
}




const char * multflt_config_get_ensfile_ref(const multflt_config_type * multflt_config) {
  return multflt_config->ensfile;
}

const char * multflt_config_get_eclfile_ref(const multflt_config_type * multflt_config) {
  return multflt_config->eclfile;
}


void multflt_config_free(multflt_config_type * multflt_config) {
  free(multflt_config->eclfile);
  free(multflt_config->ensfile);
  free(multflt_config->mean);
  free(multflt_config->std);
  free(multflt_config->active);
  free(multflt_config->logmode);
  util_free_string_list(multflt_config->fault_names           , multflt_config->data_size);
  util_free_string_list(multflt_config->output_transform_name , multflt_config->data_size);
  free(multflt_config->output_transform);
  free(multflt_config);
}



/*****************************************************************/

CONFIG_SET_ECLFILE(multflt);
CONFIG_SET_ENSFILE(multflt);
CONFIG_SET_ECLFILE_VOID(multflt);
CONFIG_SET_ENSFILE_VOID(multflt);
GET_SERIAL_SIZE(multflt);
VOID_GET_SERIAL_SIZE(multflt);
GET_DATA_SIZE(multflt);
VOID_FUNC(multflt_config_free , multflt_config_type);
SET_SERIAL_OFFSET(multflt);
VOID_SET_SERIAL_OFFSET(multflt);
