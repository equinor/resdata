#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <util.h>
#include <enkf_util.h>
#include <well_obs.h>
#include <meas_op.h>
#include <meas_data.h>
#include <hash.h>
#include <list.h>
#include <history.h>
#include <well_config.h>




struct well_obs_struct {
  const well_config_type * config;
  int    	           size;
  const history_type      	 *  hist;
  char   	       	 ** var_list;
  double               	 *  abs_std;
  double 	       	 *  rel_std;   
  bool                 	 *  active;
  bool                 	 *  current_active;
  meas_op_type         	 ** meas_op;
  enkf_obs_err_type      *  error_mode;
  int                    *  internal_offset;
};





static well_obs_type * __well_obs_alloc(const well_config_type * config , int size, const history_type * hist) {
  
  well_obs_type * well_obs = malloc(sizeof * well_obs);
  well_obs->size      	    = size;
  well_obs->meas_op   	    = enkf_util_malloc(size * sizeof * well_obs->meas_op , __func__);
  well_obs->var_list  	    = enkf_util_malloc(size * sizeof * well_obs->var_list, __func__);
  well_obs->active    	    = enkf_util_malloc(size * sizeof * well_obs->active, __func__);
  well_obs->current_active  = enkf_util_malloc(size * sizeof * well_obs->active, __func__);
  well_obs->internal_offset = enkf_util_malloc(size * sizeof * well_obs->internal_offset , __func__);
  well_obs->error_mode      = enkf_util_malloc(size * sizeof * well_obs->error_mode , __func__);
  well_obs->abs_std         = enkf_util_malloc(size * sizeof * well_obs->abs_std , __func__);
  well_obs->rel_std         = enkf_util_malloc(size * sizeof * well_obs->rel_std , __func__);
  well_obs->hist            = hist;
  well_obs->config          = config;

  return well_obs;
}



well_obs_type * well_obs_fscanf_alloc(const char * filename , const well_config_type * config , const history_type * hist) {
  FILE * stream = enkf_util_fopen_r(filename , __func__);
  int size      = util_count_file_lines(stream);
  well_obs_type * well_obs = __well_obs_alloc(config , size , hist);
  char * line = NULL;
  bool at_eof;
  int ivar;

  fseek(stream , 0L , SEEK_SET);
  for (ivar = 0; ivar < size; ivar++) {
    line = util_fscanf_realloc_line(stream , &at_eof , line);
    if (!at_eof) {
      double abs_std, rel_std;
      int active , error_mode;
      char var[32];
      int scan_count = sscanf(line , "%s  %d  %d  %lg  %lg" , var , &active , &error_mode , &abs_std , &rel_std);
      if (scan_count == 5) {
	if (well_config_has_var(config , var)) {
	  well_obs->var_list[ivar]   = util_alloc_string_copy(var);
	  well_obs->error_mode[ivar] = error_mode;  /* MUST TYPE CHECK THIS ... */
	  well_obs->abs_std[ivar]    = abs_std;
	  well_obs->rel_std[ivar]    = rel_std;
	  if (active == 1)
	    well_obs->active[ivar]   = true;
	  else if (active == 0)
	    well_obs->active[ivar]   = false;
	  else {
	    fprintf(stderr,"%s: active:%d invalid must be 0 or 1 - aborting \n",__func__ , active);
	    abort();
	  }
	} else {
	  fprintf(stderr,"%s: attempt to observe %s/%s but the variable:%s is not added to the state vector - aborting.\n",__func__ , well_config_get_well_name_ref(config) , var , var);
	  abort();
	}
      } else {
	fprintf(stderr,"%s: invalid input line %d :\"%s\" - aborting \n",__func__ , ivar + 1, line);
	abort();
      }
    }
    well_obs->meas_op[ivar] = meas_op_alloc(1);
    well_obs->internal_offset[ivar] = well_config_get_var_index(config , well_obs->var_list[ivar]);
  } 

  free(line);
  fclose(stream);
  return well_obs;
}





well_obs_type * well_obs_alloc(const well_config_type * config , int NVar , const char ** var_list , const history_type * hist) {
  int i;
  well_obs_type * well_obs  = __well_obs_alloc(config , NVar , hist);

  for (i=0; i < NVar; i++) {
    if (well_config_has_var(config , var_list[i])) {
      well_obs->meas_op[i] = meas_op_alloc(1);
      well_obs->active[i]  = true;
      well_obs->internal_offset[i] = well_config_get_var_index(config , var_list[i]);
    } else {
      fprintf(stderr,"%s: attempt to observe %s/%s but the variable:%s is not added to the state vector - aborting.\n",__func__ , well_config_get_well_name_ref(config) , var_list[i] , var_list[i]);
      abort();
    }
  }
  return well_obs;
}




void well_obs_get_observations(const well_obs_type * well_obs , int report_step, obs_data_type * obs_data) {
  const char *well_name = well_config_get_well_name_ref(well_obs->config);
  const int kw_len = 16;
  char kw[kw_len+1];
  int serial_offset = well_config_get_serial_offset(well_obs->config);
  int i;
  memcpy(well_obs->current_active , well_obs->active , well_obs->size * sizeof * well_obs->active);
  for (i=0; i < well_obs->size; i++) 
    if (well_obs->current_active[i]) {
      bool default_used;
      double d   = history_get2(well_obs->hist , report_step , well_name , well_obs->var_list[i] , &default_used);
      if (default_used) 
	well_obs->current_active[i] = false;
      else {
	double std = 1.0;
	strncpy(kw , well_name   , kw_len);
	strcat(kw , "/");
	strncat(kw , well_obs->var_list[i] , kw_len - 1 - (strlen(well_name)));
	obs_data_add(obs_data , d , std , kw);
      }
    }
  
  
  /*
    Initialize the measurement operator.
  */
  for (i = 0; i < well_obs->size; i++) 
    if (well_obs->current_active[i]) 
      meas_op_set_scalar(well_obs->meas_op[i] , serial_offset + well_obs->internal_offset[i]);

}




void well_obs_measure(const well_obs_type * well_obs , const double * serial_data , meas_data_type * meas_data) {
  int i;

  for (i=0; i < well_obs->size; i++) 
    if (well_obs->current_active[i])
      meas_data_add(meas_data , meas_op_eval(well_obs->meas_op[i] , serial_data));
}



void well_obs_free(well_obs_type * well_obs) {
  printf("********** WARNING:%s() does not free internal fields ********** \n",__func__);
  free(well_obs);
}




VOID_FREE(well_obs)
VOID_GET_OBS(well_obs)
VOID_MEASURE(well_obs)
