#include <string.h>
#include <ecl_kw.h>
#include <ecl_fstate.h>
#include <ecl_sum.h>
#include <ext_job.h>
#include <lsf_jobs.h>


static ecl_fstate_type * ECL_FSTATE     = NULL;
static bool              ENDIAN_CONVERT = true;
static ecl_sum_type    * ECL_SUM        = NULL;
static lsf_pool_type   * LSF_POOL       = NULL;

/******************************************************************/

static char * alloc_cstring(const char *fort_string , int strlen) {
  const char null_char = '\0';
  char *new_string = malloc(strlen + 1);
  strncpy(new_string , fort_string , strlen);
  new_string[strlen] = null_char;
  return new_string;
}


void ecl_inter_load_file__(const char *__filename , const int *strlen) {
  char *filename = alloc_cstring(__filename , *strlen);
  ECL_FSTATE = ecl_fstate_load_unified(filename , ECL_FMT_AUTO , ENDIAN_CONVERT);
  free(filename);
}


void ecl_inter_free__(void) {
  if (ECL_FSTATE != NULL) {
    ecl_fstate_free(ECL_FSTATE);
    ECL_FSTATE = NULL;
  }
  if (ECL_SUM != NULL) {
    ecl_sum_free(ECL_SUM);
    ECL_SUM = NULL;
  }
}


void ecl_inter_get_nstep__(int *Nstep) {
  *Nstep = ecl_fstate_get_Nstep(ECL_FSTATE);
}


/*
  Fortran index conventions in the call: These functions are called
  with 1-based indexes.
*/

void ecl_inter_kw_iget__(const char *kw , const int *istep , const int *iw , void *value) {
  if (!ecl_fstate_kw_iget(ECL_FSTATE , (*istep) - 1 , kw  , (*iw) - 1 , value)) {
    fprintf(stderr,"%s - failed to load kw:%s  timestep:%d  index:%d - aborting \n",__func__ , kw , *istep , *iw);
    abort();
  }
}


void ecl_inter_get_kw_size__(const char *kw, const int *istep, int *size) {
  *size = ecl_fstate_kw_get_size(ECL_FSTATE , (*istep) - 1 , kw);
}
  


void ecl_inter_kw_get_data__(const char *kw , const int *istep , void *value) {
  if (!ecl_fstate_kw_get_memcpy_data(ECL_FSTATE , (*istep) - 1, kw , value)) {
    fprintf(stderr,"%s: failed to load kw:%s  timestep:%d - aborting.\n",__func__ , kw , *istep);
    abort();
  }
}


void ecl_inter_kw_exists__(const char *kw , const int *istep , int *int_ex) {
  if (ecl_fstate_kw_exists(ECL_FSTATE ,  (*istep) - 1 , kw))
    *int_ex = 1;
  else
    *int_ex =  0;
}



void ecl_inter_load_summary__(const char *__header_file , const int *header_len , const char *__data_file , const int *data_len) {
  char * header_file = alloc_cstring(__header_file , *header_len);
  char * data_file   = alloc_cstring(__data_file   , *data_len);

  ECL_SUM = ecl_sum_load_unified(header_file , data_file , ECL_FMT_AUTO , ENDIAN_CONVERT);
  free(header_file);  
  free(data_file);
}

void ecl_inter_get_nwells__(int *Nwells) {
  *Nwells = ecl_sum_get_Nwells(ECL_SUM);
}

void ecl_inter_copy_well_names__(char **well_list) {
  ecl_sum_copy_well_names(ECL_SUM , well_list);
}

void ecl_inter_sum_get__(const char *_well_name , const int *well_len, 
			 const char *_var_name  , const int *var_len, 
			 void *value) {
  char *well = alloc_cstring(_well_name , *well_len);
  char *var  = alloc_cstring(_var_name  , *var_len);
  ecl_sum_iget1(ECL_SUM , 0 , well , var , value);
  free(well);
  free(var);
}
    


/******************************************************************/

static void ecl_inter_run_eclipse_static(int jobs , int max_running , int max_restart , int *submit_list , const char *base_run_path , const char *eclipse_base , int time_step , int fmt_out , int exit_on_submit_int , int use_lsf_int) {
  const int sleep_time  = 2;
  bool exit_on_submit , use_lsf;
  int job , i , submit_jobs;
  ext_job_type ** jobList;
  char run_file[256] , complete_file[256] , run_path[256] , id[64], summary_file[64];
  
  submit_jobs = 0;
  if (exit_on_submit_int)
    exit_on_submit = true;
  else
    exit_on_submit = false;
  if (use_lsf_int)
    use_lsf = true;
  else
    use_lsf = false;
  
  for (job = 0; job < jobs; job++) 
    submit_jobs += submit_list[job];
  jobList = calloc(submit_jobs , sizeof *jobList);
  
  i = 0;
  for (job = 0; job < jobs; job++) {
    if (submit_list[job]) {
      sprintf(run_path , "%s%04d" , base_run_path , job + 1); 
      /*
	For som fxxxing reason the *.run_lock file is not
	created when the simulation is submitted from 
	Geir's linux computer???

	sprintf(run_file , "%s.run_lock" , eclipse_base);
      */
      sprintf(run_file , "%s.PRT" , eclipse_base);
      if (fmt_out)
	sprintf(complete_file , "%s.F%04d" , eclipse_base , time_step);
      else
	sprintf(complete_file , "%s.X%04d" , eclipse_base , time_step);
      sprintf(id,"Job: %04d" , job + 1);
      jobList[i] = ext_job_alloc(id , "@eclipse < eclipse.in 2> /dev/null | grep filterXX" , NULL , run_path  , run_file , complete_file , max_restart , sleep_time , true , use_lsf);
      i++;
    }
  }
  sprintf(summary_file , "Jobsummary/summary_%04d", time_step);
  ext_job_run_pool(submit_jobs , jobList , max_running , 10 , summary_file , exit_on_submit);
}


void ecl_inter_run_eclipse__(const char * __basedir , int *basedir_length, 
			     const char * __eclbase , int *eclbase_length , 
			     int *jobs , int *max_running , int *max_restart, 
			     int *submit_list, int *time_step , int *fmt_out,
			     int *exit_on_submit) {
  char *basedir = alloc_cstring(__basedir , *basedir_length);
  char *eclbase = alloc_cstring(__eclbase , *eclbase_length);
  
  printf("*****************************************************************\n");
  printf("* Skal kjore eclipse jobber .... \n");
  printf("*****************************************************************\n");
  ecl_inter_run_eclipse_static(*jobs , *max_running , *max_restart , submit_list , basedir , eclbase , *time_step , *fmt_out , *exit_on_submit , 0);
  free(basedir);
  free(eclbase);
}


/*****************************************************************/

void ecl_inter_init_lsf__(const int  * sleep_time , const int *max_running, 
			  const char * _summary_file , const int * summary_file_len) {
  char *summary_file = alloc_cstring(_summary_file , *summary_file_len);
  LSF_POOL = lsf_pool_alloc(*sleep_time , *max_running , summary_file , "bjobs -a" , "/tmp");
  free(summary_file);
}



void ecl_inter_add_lsf_job__(const int *iens, 
			     const char *_run_path      , const int *run_path_len , 
			     const char *_complete_file , const int *complete_file_len,
			     const int  *max_resubmit) {
  if (LSF_POOL == NULL) {
    fprintf(stderr,"%s - must call xxxx_lsf_init first - aborting \n",__func__);
    abort();
  }
  {
    char *complete_file = alloc_cstring(_complete_file , *complete_file_len); 
    char *run_path      = alloc_cstring(_run_path      , *run_path_len);
    
    lsf_pool_add_job(LSF_POOL , NULL , run_path , complete_file , *max_resubmit);
    free(run_path);
    free(complete_file);
    printf("%4d" , *iens); fflush(stdout);
  }
}


void ecl_inter_run_lsf__(const int *_sub_exit, int *exit_count) {
  bool sub_exit;
  if (LSF_POOL == NULL) {
    fprintf(stderr,"%s - must call xxxx_lsf_init first - aborting \n",__func__);
    abort();
  }
  if (*_sub_exit == 1)
    sub_exit = true;
  else
    sub_exit = false;
  
  *exit_count = lsf_pool_run_jobs(LSF_POOL , sub_exit);
}


void ecl_inter_free_lsf__() {
  if (LSF_POOL != NULL) 
    lsf_pool_free(LSF_POOL);
  LSF_POOL = NULL;
}


/*   void ecl_inter_new_file(const char * filename , int fmt_mode) { */
/*   ECL_FSTATE = ecl_fstate_alloc(filename , 10 , fmt_mode , ENDIAN_CONVERT); */
/*   } */


/* void ecl_inter_get_kw_data(const char *header , char *buffer) { */
/*   ecl_kw_type * ecl_kw = ecl_fstate_get_kw(ECL_FSTATE , header); */
/*   ecl_kw_get_memcpy_data(ecl_kw , buffer); */
/*   ecl_kw_free(ecl_kw); */
/* } */


/* void ecl_inter_add_kwc(const char *header  , const char *ecl_str_type ,  const char *buffer) { */
/*   ecl_kw_type *ecl_kw = ecl_kw_alloc_complete(true , endian_convert , header , size , ecl_str_type ,  buffer); */
/*   ecl_fstate_add_kw_copy(ECL_FSTATE , ecl_kw); */
/*   ecl_kw_free(ecl_kw); */
/* } */



/* void ecl_inter_write_file() { */
/*   ecl_fstate_fwrite(ECL_FSTATE); */
/* } */

