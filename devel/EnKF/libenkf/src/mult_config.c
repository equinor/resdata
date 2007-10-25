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


void mult_config_finalize_init(mult_config_type *config) {
  int i;

  config->serial_size = 0;
  for (i=0; i < config->data_size; i++)
    if (config->active[i])
      config->serial_size++;

  mult_config_set_output_transform(config);
}


mult_config_type * mult_config_alloc_empty(int size) {
  mult_config_type *mult_config = malloc(sizeof *mult_config);

  mult_config->data_size   	     = size;
  mult_config->mean        	     = enkf_util_malloc(size * sizeof *mult_config->mean        , __func__);
  mult_config->std         	     = enkf_util_malloc(size * sizeof *mult_config->std         ,  __func__);
  mult_config->active      	     = enkf_util_malloc(size * sizeof *mult_config->active      , __func__);
  mult_config->logmode     	     = enkf_util_malloc(size * sizeof *mult_config->logmode      , __func__);
  mult_config->output_transform      = enkf_util_malloc(mult_config->data_size * sizeof * mult_config->output_transform      , __func__);
  mult_config->output_transform_name = enkf_util_malloc(mult_config->data_size * sizeof * mult_config->output_transform_name , __func__);
  mult_config->serial_size           = 0;
  mult_config->internal_offset       = 0;
  
  return mult_config;
}



void mult_config_transform(const mult_config_type * config , const double * input_data , double *output_data) {
  int index;

  /*
    Both log and another transform will induce chaos ...
  */

  for (index = 0; index < config->data_size; index++) {

    if (config->output_transform[index] == NULL)
      output_data[index] = input_data[index];
    else
      output_data[index] = config->output_transform[index](input_data[index]);
    
    output_data[index] = logmode_transform_output_scalar( config->logmode[index] , output_data[index]);
  }
}


void mult_config_truncate(const mult_config_type * config , double *data) {
  int i;
  for (i=0; i < config->data_size; i++) {
    if (config->active[i]) 
      if (!logmode_logEnKF(config->logmode[i]))
	if (config->output_transform[i] == NULL)
	  data[i] = util_double_max(0.0 , data[i]);
  }
}





void mult_config_fscanf_line(mult_config_type * config , int line_nr , FILE * stream) {
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
  
  config->active[line_nr]  		 = true;
  config->mean[line_nr]    		 = mu;
  config->std[line_nr]     		 = sigma;
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
      }

    }
  }
  
  config->logmode[line_nr] 		 = logmode_alloc(10.0 , logmode);
}





void mult_config_free(mult_config_type * mult_config) {
  free(mult_config->mean);
  free(mult_config->std);
  free(mult_config->active);
  free(mult_config->logmode);
  util_free_string_list(mult_config->output_transform_name , mult_config->data_size);
  free(mult_config->output_transform);
  free(mult_config);
}



/*****************************************************************/

GET_SERIAL_SIZE(mult);
VOID_GET_SERIAL_SIZE(mult);
GET_DATA_SIZE(mult);
VOID_FUNC(mult_config_free , mult_config_type);
SET_SERIAL_OFFSET(mult);
GET_SERIAL_OFFSET(mult);
VOID_SET_SERIAL_OFFSET(mult);
