#include <stdlib.h>
#include <stdio.h>
#include <enkf_macros.h>
#include <obs_node.h>
#include <enkf_types.h>
#include <time.h>
#include <stdbool.h>



struct obs_node_struct {
  const void       *obs;
  obs_free_ftype   *freef;
  obs_get_ftype    *get_obs;
  obs_meas_ftype   *measure;
  enkf_active_type  active_mode;
  time_t            active_time;
  int               obs_time;
  bool              current_active;
};




obs_node_type * obs_node_alloc(const void      * obs,
			       enkf_active_type  active_mode,
			       time_t            active_time, 
			       obs_get_ftype   * get_obs,
			       obs_meas_ftype  * measure,
			       obs_free_ftype  * freef) {
  
  obs_node_type * node = malloc( sizeof *node);
  node->active_mode        = active_mode;
  node->active_time        = active_time;
  node->obs                = obs;
  node->freef              = freef;
  node->measure            = measure;
  node->get_obs            = get_obs;
  node->obs_time           = -1;
  return node;
}



void obs_node_free(obs_node_type * node) {
  if (node->freef != NULL) node->freef( (void *) node->obs);
  free(node);
}


void obs_node_get_observations(obs_node_type * node , int report_step, obs_data_type * obs_data) {
  if (node->active_mode == active_on) 
    node->current_active = true;
  else
    node->current_active = false;

  if (node->current_active) {
    node->get_obs(node->obs , report_step , obs_data);
    node->obs_time = report_step;
  }
}


void obs_node_measure(const obs_node_type * node , int report_step , const void * enkf_node , meas_data_type * meas_data) {
  if (node->current_active) {
    if (report_step != node->obs_time) {
      fprintf(stderr,"%s: measuring at time:%d observations loaded at time:%d - aborting \n",__func__ , report_step , node->obs_time);
      abort();
    }
    node->measure(node->obs , enkf_node , meas_data);
  }
}


const void *  obs_node_get_ref(const obs_node_type * node) { 
  return node->obs; 
}




VOID_FREE(obs_node)
