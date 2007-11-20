#include <stdlib.h>
#include <enkf_util.h>
#include <enkf_state.h>
#include <enkf_node.h>
#include <enkf_config.h>
#include <multz_config.h>
#include <multz.h>
#include <equil_config.h>
#include <equil.h>
#include <ecl_kw.h>
#include <ecl_block.h>
#include <enkf_path.h>
#include <enkf_types.h>
#include <field_config.h>
#include <well_config.h>
#include <thread_pool.h>
#include <void_arg.h>
#include <well_obs.h>
#include <history.h>
#include <sched_file.h>
#include <obs_data.h>
#include <meas_data.h>
#include <enkf_obs.h>
#include <multflt_config.h>
#include <multflt.h>


void TEST() {
  multz_TEST();
  multflt_TEST();
}



int main(void) {

  enkf_obs_type      * enkf_obs;
  obs_data_type      * obs_data;
  meas_data_type     * meas_data;
  enkf_config_type   * config;
  enkf_state_type   ** state;

  const int *index_map;
  int nx,ny,nz,active_size;
  thread_pool_type * tp;

  sched_file_type    * sched;
  history_type          * hist;

  meas_data = meas_data_alloc();
  obs_data  = obs_data_alloc();


  sched     = sched_file_alloc((const int [3]) {1 , 1 , 1999});
  sched_file_parse(sched , "SCHEDULE.INC");
  hist      = history_alloc_from_schedule(sched);
 
  index_map = field_config_alloc_index_map1("ECLIPSE.EGRID" , true , &nx , &ny , &nz , &active_size);
  config = enkf_config_alloc(4, 2 , true);
  enkf_config_add_type(config , "MULTZ" , 
		       parameter , MULTZ, 
		       multz_config_fscanf_alloc("Config/multz" , 100 , 100 , 100 , "MULTZ.INC" , "multz"));

  enkf_config_add_type(config , "EQUIL" , 
		       parameter , EQUIL, 
		       equil_config_alloc(10 , true , true , "EQUIL.INC" , "equil") );
  
  
  enkf_config_add_type(config , "SWAT"  , ecl_restart , FIELD , 
		       field_config_alloc("SWAT" , ecl_float_type   , nx , ny , nz , active_size , index_map , 1 , NULL , "SWAT"));
  
  enkf_config_add_type(config , "PRESSURE" , ecl_restart , FIELD , 
		       field_config_alloc("PRESSURE"  , ecl_float_type , nx , ny , nz , active_size , index_map , 1 , NULL , "Pressure"));

  enkf_config_add_type(config , "SGAS"  , ecl_restart , FIELD , 
		       field_config_alloc("SGAS" , ecl_float_type    , nx , ny , nz , active_size , index_map , 1 , NULL , "SGAS"));


  enkf_config_add_type(config , "RS"     , ecl_restart , FIELD , 
		       field_config_alloc("RS"  , ecl_float_type        , nx , ny , nz , active_size , index_map , 1 , NULL , "RS"));
  
  enkf_config_add_type(config , "RV"    , ecl_restart , FIELD , 
		       field_config_alloc("RV"    , ecl_float_type       , nx , ny , nz , active_size , index_map , 1 , NULL , "RV"));
  
  enkf_config_add_well(config , "B-37T2" ,  4 , (const char *[4]) {"WGPR" , "WWPR" , "WOPR" , "WBHP"});
  enkf_config_add_well(config , "B-33A"  ,  4 , (const char *[4]) {"WGPR" , "WWPR" , "WOPR" , "WBHP"});
  enkf_config_add_well(config , "B-43A"  ,  4 , (const char *[4]) {"WGPR" , "WWPR" , "WOPR" , "WBHP"});

  enkf_obs = enkf_obs_alloc(config , sched);
  enkf_obs_add_well_obs(enkf_obs , "B-33A"  , NULL, "Config/B-33A");
  enkf_obs_add_well_obs(enkf_obs , "B-37T2" , NULL, "Config/B-43A");
  enkf_obs_add_well_obs(enkf_obs , "B-43A"  , NULL, "Config/B-37T2");
  
  /*
    enkf_obs_add_field_obs(enkf_obs , "PRES"  , 4 , (const char *[4]) {"WGPR" , "WWPR" , "WOPR" , "WBHP"});
  */



  tp = thread_pool_alloc(10);
  state = malloc(100 * sizeof *state);
  {
    void_arg_type ** load_arg = calloc(100 , sizeof * load_arg);
    int i;
    char path[100];
    
    for (i=0; i < 100; i++) {
      
      state[i] = enkf_state_alloc(config , "ECLIPSE" , false);
      enkf_state_add_node(state[i] , "MULTZ"); 
      enkf_state_add_node(state[i] , "EQUIL");

      enkf_state_iset_eclpath(state[i] , 0 , "RunPATH");
      sprintf(path , "tmpdir_%04d" , i);
      enkf_state_iset_eclpath(state[i] , 1 , path);
      
      {
	bool unified    = false;
	int report_step = 51;

	load_arg[i] = void_arg_alloc3(sizeof state[i] , sizeof unified, sizeof report_step);
	
	void_arg_pack_ptr(load_arg[i] , 0 , &state[i]);
	void_arg_pack_ptr(load_arg[i] , 1 , &unified);
	void_arg_pack_ptr(load_arg[i] , 2 , &report_step);
	
	thread_pool_add_job(tp , &enkf_state_load_ecl_summary_void , load_arg[i]);
	thread_pool_add_job(tp , &enkf_state_load_ecl_restart_void , load_arg[i]);
      }
      
      
      /*
	enkf_state_load_ecl_summary(state[i] , false , 51);
	enkf_state_load_ecl_restart(state[i] , false , 51);
      */

      
      /*
	enkf_state_iset_enspath(state[i] , 0 , "Ensemble");

	sprintf(path , "%04d" , i);
	enkf_state_iset_enspath(state[i] , 1 , "1");
	enkf_state_iset_enspath(state[i] , 2 , path);
	enkf_state_iset_enspath(state[i] , 3 , "Forecast");
	enkf_state_ens_write(state[i] , all_types - ecl_static );
	
	enkf_state_iset_enspath(state[i] , 3 , "Static");
	enkf_state_swapout(state[i] , ecl_static);
	
	sprintf(path , "tmpdir_%04d" , i);
	enkf_state_iset_eclpath(state[i] , 1 , path);
	enkf_state_ecl_write(state[i] , all_types , 51);
	
	enkf_state_swapin(state[i] , ecl_static);
      */
    }
    thread_pool_join(tp);
    {
      double *X = calloc(100 * 100 , sizeof *X);
      enkf_ensemble_update(state , 100 , 100 * 4096 , X); 
      free(X);
    }
    enkf_obs_get_observations(enkf_obs , 51 , obs_data);
    obs_data_fprintf(obs_data   , stdout);
    for (i = 0; i < 100; i++) 
      enkf_obs_measure(enkf_obs   , 51 , state[i] , meas_data);
    meas_data_fprintf(meas_data , stdout);
    

    for (i=0; i < 100; i++)
      enkf_state_free(state[i]);
    
  }    
  obs_data_reset(obs_data);
  
  obs_data_fprintf(obs_data , stdout);
  enkf_config_free(config);
  enkf_obs_free(enkf_obs);
  history_free(hist);
}
