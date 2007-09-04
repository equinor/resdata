#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <enkf_util.h>
#include <util.h>
#include <ens_config.h>
#include <config.h>
#include <multz_config.h>
#include <logmode.h>
#include <util.h>


/*
  WARNING: Returns the multz_config object in a completely unitialized state.
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





void multz_config_fwrite(const multz_config_type * config , FILE * stream) {
  /* Bootstrap is based on the three first fields. */
  UTIL_FWRITE_SCALAR(config->data_size   , stream);
  util_fwrite_string(config->ensfile     , stream);
  util_fwrite_string(config->eclfile     , stream);
  
  UTIL_FWRITE_SCALAR(config->serial_size , stream);
  util_fwrite_string(config->ecl_kw_name , stream);
  UTIL_FWRITE_SCALAR(config->var_type    , stream);
  UTIL_FWRITE_VECTOR(config->i1          , config->data_size , stream);
  UTIL_FWRITE_VECTOR(config->i2          , config->data_size , stream);
  UTIL_FWRITE_VECTOR(config->j1          , config->data_size , stream);
  UTIL_FWRITE_VECTOR(config->j2          , config->data_size , stream);
  UTIL_FWRITE_VECTOR(config->k           , config->data_size , stream);
  UTIL_FWRITE_VECTOR(config->area        , config->data_size , stream);
  UTIL_FWRITE_VECTOR(config->mean        , config->data_size , stream);
  UTIL_FWRITE_VECTOR(config->std         , config->data_size , stream);
  UTIL_FWRITE_VECTOR(config->active      , config->data_size , stream);
  {
    int i;
    for (i=0; i < config->data_size; i++) 
      logmode_fwrite(config->logmode[i] , stream);
  }
}




multz_config_type * multz_config_fread_alloc(FILE * stream) {
  int size;
  multz_config_type * config;
  UTIL_FREAD_SCALAR(size , stream);
  {
    char * ensfile , *eclfile;
    ensfile = util_fread_alloc_string(stream);
    eclfile = util_fread_alloc_string(stream);
    config = __multz_config_alloc_empty(size , eclfile , ensfile);
  }
  
  UTIL_FREAD_SCALAR(config->serial_size , stream);
  config->ecl_kw_name = util_fread_alloc_string(stream);
  UTIL_FREAD_SCALAR(config->var_type    , stream);
  UTIL_FREAD_VECTOR(config->i1          , config->data_size , stream);
  UTIL_FREAD_VECTOR(config->i2          , config->data_size , stream);
  UTIL_FREAD_VECTOR(config->j1          , config->data_size , stream);
  UTIL_FREAD_VECTOR(config->j2          , config->data_size , stream);
  UTIL_FREAD_VECTOR(config->k           , config->data_size , stream);
  UTIL_FREAD_VECTOR(config->area        , config->data_size , stream);
  UTIL_FREAD_VECTOR(config->mean        , config->data_size , stream);
  UTIL_FREAD_VECTOR(config->std         , config->data_size , stream);
  UTIL_FREAD_VECTOR(config->active      , config->data_size , stream);
  {
    int i;
    for (i=0; i < config->data_size; i++) 
      config->logmode[i] = logmode_fread_alloc(stream);
  }
  return config;
}




multz_config_type * multz_config_fscanf_alloc(const char * filename , int nx , int ny , int nz , const char * eclfile , const char * ensfile) {
  multz_config_type * config;
  FILE * stream = util_fopen(filename , "r");
  char * line   = NULL;
  int size , line_nr;
  bool at_eof;

  size = util_count_content_file_lines(stream);
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
      if (scan_count == EOF) 
	at_eof = true;
      else {
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
	  j2 = util_int_min(j2 , ny);
	  break;
	default:
	  fprintf(stderr,"%s: line number %d in config file %s not recognized as valid format - aborting\n",__func__ , line_nr + 2 , filename);
	  abort();
	}
	
	if (logmode < 0 || logmode > log_all) {
	  fprintf(stderr,"%s: line number %d in config file: %s  logmode=%d is invalid - aborting \n",__func__ , line_nr + 1 , filename , logmode);
	  abort();
	}
	
	if (k <= 0 || k > nz) {
	  fprintf(stderr,"%s: invalid k:%d \n",__func__ , k);
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
	config->area[line_nr]    = (config->i2[line_nr]- config->i1[line_nr] + 1) * (config->j2[line_nr]- config->j1[line_nr] + 1);
	logmode_transform_input_distribution(config->logmode[line_nr], &config->mean[line_nr] , &config->std[line_nr]);
	if (config->active[line_nr])
	  config->serial_size++;
      }
    }
  } while (! at_eof);
  
  fclose(stream);
  free(line);
  return config;
}



const char * multz_config_get_ensfile_ref(const multz_config_type * config) {
  if (config->ensfile == NULL) {
    fprintf(stderr,"%s: ensfile == NULL - aborting \n",__func__);
    abort();
  }
  return config->ensfile;
}

const char * multz_config_get_eclfile_ref(const multz_config_type * config) {
  if (config->eclfile == NULL) {
    fprintf(stderr,"%s: eclfile == NULL - aborting \n",__func__);
    abort();
  }
  return config->eclfile;
}


void multz_config_fprintf_layer(const multz_config_type * config , int ik , double multz_value , FILE *stream) {
  fprintf(stream,"BOX\n   %5d %5d %5d %5d %5d %5d / \nMULTZ\n%d*%g /\nENDBOX\n\n\n" , 
	  config->i1[ik]   , config->i2[ik] , 
	  config->j1[ik]   , config->j2[ik] , 
	  config->k[ik]    , config->k[ik]  , 
	  config->area[ik] , logmode_transform_output_scalar(config->logmode[ik] , multz_value));
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

							 

