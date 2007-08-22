#include <stdlib.h>
#include <stdio.h>
#include <util.h>
#include <well_obs.h>
#include <meas_op.h>
#include <meas_data.h>
#include <hash.h>
#include <list.h>
#include <hist.h>


typedef enum {abs_error = 0 , rel_error = 1} well_obs_error_type;


struct well_obs_struct {
  int    	          size;
  const hist_type      *  hist;
  char   	       *  well_name;
  char   	       ** var_list;
  double               *  abs_std;
  double 	       *  rel_std;   
  meas_op_type         ** meas_op;
  well_obs_error_type     error_type;
};









well_obs_type * well_obs_alloc(const char * well_name , int NVar , const char ** var_list) {
  well_obs_type * well_obs = malloc(sizeof * well_obs);
  
  well_obs->size      = NVar;
  well_obs->well_name = util_alloc_string_copy(well_name);
  well_obs->var_list  = util_alloc_stringlist_copy(var_list , NVar);
  well_obs->meas_op   = malloc(5 * sizeof * well_obs->meas_op);
  {
    int i;
    for (i=0; i < NVar; i++)
      well_obs->meas_op[i] = meas_op_alloc(1);
  }
  
  return well_obs;
}



void well_obs_measure(const well_obs_type * well_obs , const double * serial_data , meas_data_type * meas_data) {
  int i;
  
  for (i=0; i < well_obs->size; i++)
    meas_data_add(meas_data , meas_op_eval(well_obs->meas_op[i] , serial_data));
  
}


