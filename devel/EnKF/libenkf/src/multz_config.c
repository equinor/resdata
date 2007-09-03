#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <enkf_util.h>
#include <util.h>
#include <ens_config.h>
#include <config.h>
#include <multz_config.h>
#include <logmode.h>



/*
  WARNING: Return the multz_config object in a completely unitialized state.
*/
static multz_config_type * __multz_config_alloc_empty(int size , const char * eclfile , const char * ensfile) {

  multz_config_type *config = malloc(sizeof *config);
  config->ecl_kw_name = NULL;
  config->data_size   = size;
  config->var_type    = parameter;
  
  config->eclfile = util_alloc_string_copy(eclfile);
  config->ensfile = util_alloc_string_copy(ensfile);

  config->mean    = enkf_util_malloc(config->data_size * sizeof *config->mean    , __func__);
  config->std     = enkf_util_malloc(config->data_size * sizeof *config->std     , __func__);
  config->active  = enkf_util_malloc(config->data_size * sizeof *config->active  , __func__);
  config->i1      = enkf_util_malloc(config->data_size * sizeof *config->i1      , __func__);
  config->i2      = enkf_util_malloc(config->data_size * sizeof *config->i2      , __func__);
  config->j1      = enkf_util_malloc(config->data_size * sizeof *config->j1      , __func__);
  config->j2      = enkf_util_malloc(config->data_size * sizeof *config->j2      , __func__);
  config->k       = enkf_util_malloc(config->data_size * sizeof *config->k       , __func__);
  config->area    = enkf_util_malloc(config->data_size * sizeof *config->area    , __func__);
  config->logmode = enkf_util_malloc(config->data_size * sizeof *config->logmode , __func__);
  config->serial_size = 0;
  
  return config;
}



/*multz_config_type * multz_config_alloc(const int *i1, const int *i2 , const int *j1 , const int *j2, const char * eclfile , const char * ensfile) {
  multz_config_type *config = __multz_config_alloc_empty(size , eclfile , ensfile);
  { 
    int i;
    for (i = 0; i < config->data_size; i++) {
      config->mean[i]   = 1.0;
      config->std[i]    = 1.0;
      config->active[i] = true;

      config->i1[i]     = i1[i];
      config->i2[i]     = i2[i];

      config->j1[i]     = j1[i];
      config->j2[i]     = j2[i];
      
      config->k[i]      = i+1;

      if (config->active[i])
	config->serial_size++;
    }
  }

  { 
    int i;
    for (i = 0; i < config->data_size; i++) 
      config->area[i] = (config->i2[i]- config->i1[i] + 1) * (config->j2[i]- config->j1[i] + 1);
  }
  
  return config;
}
*/



multz_config_type * multz_config_fscanf_alloc(const char * filename , int nx , int ny , int nz , const char * eclfile , const char * ensfile) {
  multz_config_type * config;
  FILE * stream = util_fopen(filename , "r");
  char * line   = NULL;
  int size , line_nr;
  bool at_eof;

  size = util_count_file_lines(stream);
  fseek(stream , 0L , SEEK_SET);
  config  = __multz_config_alloc_empty(size , eclfile , ensfile);
  line_nr = -1;
  do {
    int k , logmode;
    int i1,i2,j1,j2;
    double   mu , sigma;
    
    line = util_fscanf_realloc_line(stream , &at_eof , line);
    if (!at_eof) {
      int scan_count = sscanf(line , "%d  %lg  %lg  %d  %d  %d  %d  %d" , &k , &mu , &sigma , &logmode , &i1 , &i2 , &j1 , &j2);
      switch(scan_count) {
      case(4):
	line_nr++;
	i1 = 1; 
	i2 = nx;
	j1 = 1;
	j2 = ny;
	break;
      case(8):
	line_nr++;
	i1 = util_int_max(i1 , 1);
	i2 = util_int_min(i2 , nx);
	j1 = util_int_max(j1 , 1);
	j2 = util_int_min(j1 , ny);
	break;
      default:
	fprintf(stderr,"%s: line number %d in config file %s not recognized as valid format - aborting\n",__func__ , line_nr + 2 , filename);
	abort();
      }

      if (logmode < 0 || logmode > log_all) {
	fprintf(stderr,"%s: line number %d in config file: %s  logmode=%d is invalid - aborting \n",__func__ , line_nr + 1 , filename , logmode);
	abort();
      }

      config->mean[line_nr]    = mu;
      config->std[line_nr]     = sigma;
      config->active[line_nr]  = true;
      config->logmode[line_nr] = logmode_alloc(10.0 , logmode);
      config->k[line_nr]       = k;
      config->i1[line_nr]      = i1;
      config->i2[line_nr]      = i2;
      config->j1[line_nr]      = j1;
      config->j2[line_nr]      = j2;
    }
  } while (! at_eof);
    
  {
    int i;
    for (i = 0; i < config->data_size; i++) {
      double mu , std;
      logmode_transform_input_distribution(config->logmode[i], config->mean[i] , config->std[i] , &mu , &std); 
      config->mean[i] = mu;  config->std[i] = std;
      
      config->area[i] = (config->i2[i]- config->i1[i] + 1) * (config->j2[i]- config->j1[i] + 1);
      if (config->active[i])
	config->serial_size++;
    }
  }
  fclose(stream);
  free(line);
  return config;
}



const char * multz_config_get_ensfile_ref(const multz_config_type * config) {
  return config->ensfile;
}

const char * multz_config_get_eclfile_ref(const multz_config_type * config) {
  return config->eclfile;
}


void multz_config_fprintf_layer(const multz_config_type * config , int ik , double multz_value , FILE *stream) {
  fprintf(stream,"BOX\n   %5d %5d %5d %5d %5d %5d / \nMULTZ\n%d*%g /\nENDBOX\n\n" , 
	  config->i1[ik]   , config->i2[ik] , 
	  config->j1[ik]   , config->j2[ik] , 
	  config->k[ik]    , config->k[ik]  , 
	  config->area[ik] , multz_value);
}



void multz_config_free(multz_config_type * config) {
  int i;
  free(config->std);
  free(config->mean);
  free(config->active);
  free(config->j1);
  free(config->j2);
  free(config->i1);
  free(config->i2);
  free(config->k);
  free(config->area);
  for (i=0; i < config->data_size; i++)
    logmode_free(config->logmode[i]);
  free(config->logmode);
  
  CONFIG_FREE_STD_FIELDS;
  free(config);
}


/*****************************************************************/
CONFIG_SET_ECLFILE(multz);
CONFIG_SET_ENSFILE(multz);
CONFIG_SET_ECLFILE_VOID(multz);
CONFIG_SET_ENSFILE_VOID(multz);
GET_SERIAL_SIZE(multz)
GET_DATA_SIZE(multz)
VOID_GET_SERIAL_SIZE(multz)
SET_SERIAL_OFFSET(multz);
VOID_SET_SERIAL_OFFSET(multz);


VOID_FUNC(multz_config_free , multz_config_type);

							 

