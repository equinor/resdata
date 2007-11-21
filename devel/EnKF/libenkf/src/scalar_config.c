#include <stdlib.h>
#include <string.h>
#include <util.h>
#include <ens_config.h>
#include <scalar_config.h>
#include <enkf_util.h>
#include <enkf_macros.h>
#include <logmode.h>
#include <trans_func.h>



/*
static void scalar_config_set_output_transform(scalar_config_type * config) {
  int i;
  for (i=0; i < config->data_size; i++) 
    config->output_transform[i] = trans_func_lookup(config->output_transform_name[i] , &config->void_arg[i]);
}
*/


void scalar_config_finalize_init(scalar_config_type *config) {
  /*
    scalar_config_set_output_transform(config);
  */
}


scalar_config_type * scalar_config_alloc_empty(int size) {
  scalar_config_type *scalar_config = malloc(sizeof *scalar_config);

  scalar_config->data_size   	       = size;
  scalar_config->mean        	       = enkf_util_malloc(size * sizeof *scalar_config->mean        , __func__);
  scalar_config->std         	       = enkf_util_malloc(size * sizeof *scalar_config->std         ,  __func__);
  scalar_config->active      	       = enkf_util_malloc(size * sizeof *scalar_config->active      , __func__);
  scalar_config->logmode       	       = enkf_util_malloc(size * sizeof *scalar_config->logmode      , __func__);
  scalar_config->output_transform      = enkf_util_malloc(scalar_config->data_size * sizeof * scalar_config->output_transform      , __func__);
  scalar_config->output_transform_name = enkf_util_malloc(scalar_config->data_size * sizeof * scalar_config->output_transform_name , __func__);
  scalar_config->internal_offset       = 0;
  scalar_config->void_arg              = enkf_util_malloc(scalar_config->data_size * sizeof * scalar_config->void_arg , __func__);

  {
    int i;
    for (i=0; i < size; i++) {
      scalar_config->output_transform_name[i] = NULL;
      scalar_config->void_arg[i]              = NULL;
      scalar_config->active[i]                = false;
    }
  }
  return scalar_config;
}



void scalar_config_transform(const scalar_config_type * config , const double * input_data , double *output_data) {
  int index;

  /*
    Both log and another transform will induce chaos ...
  */

  for (index = 0; index < config->data_size; index++) {

    if (config->output_transform[index] == NULL)
      output_data[index] = input_data[index];
    else
      output_data[index] = config->output_transform[index](input_data[index] , config->void_arg[index]);
    
    output_data[index] = logmode_transform_output_scalar( config->logmode[index] , output_data[index]);
  }
}


void scalar_config_truncate(const scalar_config_type * config , double *data) {
  int i;
  for (i=0; i < config->data_size; i++) {
    if (config->active[i]) 
      if (!logmode_logEnKF(config->logmode[i]))
	if (config->output_transform[i] == NULL)
	  data[i] = util_double_max(0.0 , data[i]);
  }
}





void scalar_config_fscanf_line(scalar_config_type * config , int line_nr , FILE * stream) {
  int logmode    = 0;
  double   mu , sigma;
  int scan_count;
  scan_count = fscanf(stream , "%lg  %lg",&mu,&sigma);
  /*scan_count = fscanf(stream , "%lg  %lg  %d  %s" , &mu , &sigma , &logmode , output_transform);*/
  
  
  if (scan_count != 2) {
    util_rewind_line(stream);
    fprintf(stderr,"%s error when loading line: %s - aborting \n",__func__ , util_fscanf_alloc_line(stream , NULL));
    abort();
  }
  
  config->active[line_nr]  = true;
  config->mean[line_nr]    = mu;
  config->std[line_nr]     = sigma;
  {
    long int current_pos;
    char * token;

    current_pos = ftell(stream);
    token       = util_fscanf_alloc_token(stream);
    if (token != NULL) {

      if (sscanf(token , "%d" , &logmode) != 1)
	fseek(stream , current_pos , SEEK_SET);
      else {
	current_pos = ftell(stream);
	config->output_transform_name[line_nr] = util_fscanf_alloc_token(stream);
	config->output_transform[line_nr]      = trans_func_lookup(config->output_transform_name[line_nr] , stream , &config->void_arg[line_nr]);
      }
    }
  }
  config->logmode[line_nr] 		 = logmode_alloc(10.0 , logmode);
}





void scalar_config_free(scalar_config_type * scalar_config) {
  int i;
  free(scalar_config->mean);
  free(scalar_config->std);
  free(scalar_config->active);
  free(scalar_config->logmode);
  util_free_string_list(scalar_config->output_transform_name , scalar_config->data_size);
  for (i=0; i < scalar_config->data_size; i++)
    if (scalar_config->void_arg[i] != NULL) void_arg_free(scalar_config->void_arg[i]);

  free(scalar_config->void_arg);
  free(scalar_config->output_transform);
  free(scalar_config);
}



/*****************************************************************/

GET_DATA_SIZE(scalar);
VOID_FUNC(scalar_config_free , scalar_config_type);
SET_SERIAL_OFFSET(scalar);
GET_SERIAL_OFFSET(scalar);
VOID_SET_SERIAL_OFFSET(scalar);
