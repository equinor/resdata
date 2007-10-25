#include <stdlib.h>
#include <string.h>
#include <util.h>
#include <ens_config.h>
#include <multflt_config.h>
#include <enkf_util.h>
#include <config.h>
#include <logmode.h>
#include <trans_func.h>
#include <mult_config.h>




static multflt_config_type * __multflt_config_alloc_empty(int size, const char * eclfile , const char * ensfile) {
  multflt_config_type *multflt_config = malloc(sizeof *multflt_config);
  multflt_config->fault_names = enkf_util_malloc(size * sizeof *multflt_config->fault_names , __func__);
  multflt_config->mult_config = mult_config_alloc_empty(size);

  multflt_config->ecl_kw_name = NULL;
  multflt_config->var_type    = parameter;

  multflt_config->eclfile = NULL;
  multflt_config->ensfile = NULL;
  multflt_config_set_eclfile(multflt_config , eclfile);
  multflt_config_set_ensfile(multflt_config , ensfile);
  
  return multflt_config;
}



/*multflt_config_type * multflt_config_alloc(int size, const char * eclfile , const char * ensfile) {
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
*/


void multflt_config_transform(const multflt_config_type * config , const double * input_data , double * output_data) {
  mult_config_transform(config->mult_config , input_data , output_data);
}



multflt_config_type * multflt_config_fscanf_alloc(const char * filename , const char * eclfile , const char * ensfile) {
  multflt_config_type * config;
  FILE * stream = util_fopen(filename , "r");
  int line_nr = 0;
  int size;

  size = util_count_file_lines(stream);
  fseek(stream , 0L , SEEK_SET);
  config = __multflt_config_alloc_empty(size , eclfile , ensfile);
  do {
    char name[128];  /* UGGLY HARD CODED LIMIT */
    if (fscanf(stream , "%s" , name) != 1) {
      fprintf(stderr,"%s: something wrong when reading: %s - aborting \n",__func__ , filename);
      abort();
    }
    config->fault_names[line_nr] = util_alloc_string_copy(name);
    mult_config_fscanf_line(config->mult_config , line_nr , stream);
    line_nr++;
  } while ( line_nr < size );
  fclose(stream);
  mult_config_finalize_init(config->mult_config);
  
  return config;
}

void multflt_config_free(multflt_config_type * multflt_config) {
  mult_config_free(multflt_config->mult_config);
  util_free_string_list(multflt_config->fault_names , mult_config_get_data_size(multflt_config->mult_config));
  free(multflt_config);
}


int multflt_config_get_data_size(const multflt_config_type * multflt_config) {
  return mult_config_get_data_size(multflt_config->mult_config);
}

void multflt_config_set_serial_offset(multflt_config_type * multflt_config, int serial_offset ) {
  mult_config_set_serial_offset(multflt_config->mult_config , serial_offset);
}


/*****************************************************************/

CONFIG_GET_ENSFILE(multflt);
CONFIG_GET_ECLFILE(multflt);
CONFIG_SET_ECLFILE(multflt);
CONFIG_SET_ENSFILE(multflt);
CONFIG_SET_ECLFILE_VOID(multflt);
CONFIG_SET_ENSFILE_VOID(multflt);
VOID_FUNC(multflt_config_free , multflt_config_type);
VOID_SET_SERIAL_OFFSET(multflt);
