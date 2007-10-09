#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <hash.h>
#include <util.h>
#include <enkf_obs.h>
#include <enkf_config.h>
#include <well_obs.h>
#include <obs_node.h>
#include <history.h>
#include <enkf_util.h>

struct enkf_obs_struct {
  const enkf_config_type * config;
  const history_type        * hist;
  hash_type * obs_hash;
};




enkf_obs_type * enkf_obs_alloc(const enkf_config_type * config , const history_type * hist) {
  enkf_obs_type * enkf_obs = malloc(sizeof * enkf_obs);
  enkf_obs->config   = config;
  enkf_obs->obs_hash = hash_alloc(10);
  enkf_obs->hist     = hist;
  return enkf_obs;
}


const void * __enkf_obs_config_get(const enkf_obs_type * enkf_obs , const char * key , enkf_impl_type impl_type) {
  if (enkf_config_has_key(enkf_obs->config , key)) {
    const enkf_config_node_type * node = enkf_config_get_ref(enkf_obs->config , key );
    if (enkf_config_node_get_impl_type(node) == impl_type) 
      return enkf_config_node_get_ref(node);      
    else {
      fprintf(stderr,"%s: obse node:%s is not of correct type - aborting \n",__func__ , key);
      abort();
    }
  } else {
    fprintf(stderr,"%s: can not find config key for observation:%s - aborting \n",__func__ , key);
    abort();
  }
}

void enkf_obs_add_obs(enkf_obs_type * enkf_obs, const char * key , const obs_node_type * node) {
  if (hash_has_key(enkf_obs->obs_hash , key)) {
    fprintf(stderr,"%s: observation with key:%s already added - aborting \n",__func__ , key);
    abort();
  }
  hash_insert_hash_owned_ref(enkf_obs->obs_hash , key , node , obs_node_free__);
}



enkf_obs_type * enkf_obs_fscanf_alloc(const char * filename , const enkf_config_type * config , const history_type * hist) {
  enkf_obs_type * enkf_obs = enkf_obs_alloc(config , hist);
  FILE * stream = enkf_util_fopen_r(filename , __func__);
  char *path;
  
  util_alloc_file_components(filename , &path , NULL , NULL);
  do {
    char key[64];
    char obs_type[64];
    int scan_count = fscanf(stream , "%s  %s" , key , obs_type);
    if (scan_count != EOF) {
      char file[64];
      time_t obs_time = -1;
      int    active   = -1;
      char   cmp;
      obs_node_type * obs_node;

      if (scan_count != 2) {
	fprintf(stderr,"%s: fatal error when loading: %s - aborting \n",__func__ , filename);
	abort();
      }
      if (fscanf(stream , "%d" , &active) != 1) {
	fscanf(stream , "%c" , &cmp);
	obs_time = util_fscanf_date(stream);
      }
      if (fscanf(stream ,"%s" , file) == 1) {
	/*
	  Now we have scanned the full line sucessfully ... 
	*/
	char * config_file = util_alloc_full_path(path , file);
	enkf_active_type     active_mode;
	const void * config;
	void       * obs;

	if (active == 1) 
	  active_mode = active_on;
	else if (active == 0)
	  active_mode = active_off;
	else if (active == -1) {
	  switch(cmp) {
	  case('='):
	    active_mode = active_at;
	    break; 
	  case('>'):
	    active_mode = active_after;
	    break;
	  case('<'):
	    active_mode = active_before;
	    break;
	  default:
	    fprintf(stderr,"%s: comparison operator:%c in file:%s not recognized - alternatives are: = < > \n",__func__ , cmp , filename);
	    abort();
	  }
	} else {
	  fprintf(stderr,"%s: variable active:%d is invalid - aborting \n",__func__ , active);
	  abort();
	}

	if (strcmp(obs_type , WELL_OBS_TYPE_STRING) == 0) {      
	  config = __enkf_obs_config_get(enkf_obs , key , WELL);
	  obs = well_obs_fscanf_alloc(config_file , config , hist);
	} else if (strcmp(obs_type , POINT_OBS_TYPE_STRING) == 0) {
	  config = __enkf_obs_config_get(enkf_obs , key , FIELD);
	  obs = /*well_obs_fscanf_alloc(well_config , hist);*/ NULL;
	} else {
	  fprintf(stderr,"%s: observation type: %s is not recognized - aborting \n",__func__ , obs_type);
	  abort();
	}
      
	obs_node = obs_node_alloc(obs , active_mode , obs_time , well_obs_get_observations__ , well_obs_measure__ , well_obs_free__);
	enkf_obs_add_obs(enkf_obs , key , obs_node);
	free(config_file);
      }
    }
  } while ( !feof(stream) );
  
  free(path);
  fclose(stream);
  return enkf_obs;
}



void enkf_obs_free(enkf_obs_type * enkf_obs) {
  hash_free(enkf_obs->obs_hash);
  free(enkf_obs);
}





/*
void enkf_obs_add_well_obs(enkf_obs_type * enkf_obs, const char * well_name , const well_obs_type * obs) {
  if (enkf_config_has_key(enkf_obs->config , well_name)) {
    const enkf_config_node_type * node = enkf_config_get_ref(enkf_obs->config , well_name);
    if (enkf_config_node_get_impl_type(node) == WELL) 
      enkf_obs_add_obs(enkf_obs , well_name , obs_node_alloc(obs , well_obs_get_observations__ , well_obs_measure__ , well_obs_free__));
    else {
      fprintf(stderr,"%s config object:%s exists - but it is not of well type - aborting \n",__func__ , well_name );
      abort();
    }
  } else {
    fprintf(stderr,"%s: must add well:%s _before_ it can be used as an observation. \n",__func__ , well_name);
    abort();
  }
}
*/


void enkf_obs_get_observations(enkf_obs_type * enkf_obs , int report_step , obs_data_type * obs_data) {
  char ** kw_list = hash_alloc_keylist(enkf_obs->obs_hash);
  int iobs;
  for (iobs = 0; iobs < hash_get_size(enkf_obs->obs_hash); iobs++) {
    obs_node_type * obs_node = hash_get(enkf_obs->obs_hash , kw_list[iobs]);
    obs_node_get_observations(obs_node , report_step , obs_data);
  }
}

void enkf_obs_measure(const enkf_obs_type * enkf_obs , int report_step , const double * serial_data , meas_data_type * meas_data) {
  char ** kw_list = hash_alloc_keylist(enkf_obs->obs_hash);
  int iobs;
  for (iobs = 0; iobs < hash_get_size(enkf_obs->obs_hash); iobs++) {
    obs_node_type * obs_node = hash_get(enkf_obs->obs_hash , kw_list[iobs]);
    obs_node_measure(obs_node , report_step , serial_data , meas_data);
  }
}

