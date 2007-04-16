#include <string.h>
#include <ecl_kw.h>
#include <ecl_fstate.h>
#include <ecl_sum.h>
#include <ext_job.h>
#include <lsf_jobs.h>
#include <ecl_parse.h>
#include <ecl_diag.h>
#include <util.h>


static ecl_fstate_type * ECL_FSTATE     = NULL;
static bool              ENDIAN_CONVERT = true;
static ecl_sum_type    * ECL_SUM        = NULL;
static lsf_pool_type   * LSF_POOL       = NULL;

/******************************************************************/



void ecl_inter_load_file__(const char *__filename , const int *strlen) {
  char *filename = util_alloc_cstring(__filename , strlen);
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


void ecl_inter_del_kw(const char *kw, const int *istep) {
  ecl_block_type *ecl_block = ecl_fstate_get_block(ECL_FSTATE , (*istep) - 1);
  ecl_block_free_kw(ecl_block , kw);
}


void ecl_inter_kw_exists__(const char *kw , const int *istep , int *int_ex) {
  if (ecl_fstate_kw_exists(ECL_FSTATE ,  (*istep) - 1 , kw))
    *int_ex = 1;
  else
    *int_ex =  0;
}


void ecl_inter_load_summary__(const char *__header_file , const int *header_len , const char *__data_file , const int *data_len) {
  char * header_file = util_alloc_cstring(__header_file , header_len);
  char * data_file   = util_alloc_cstring(__data_file   , data_len);

  ECL_SUM = ecl_sum_load_unified(header_file , data_file , ECL_FMT_AUTO , ENDIAN_CONVERT);
  free(header_file);  
  free(data_file);
}

void ecl_inter_get_nwells__(int *Nwells) {
  *Nwells = ecl_sum_get_Nwells(ECL_SUM);
}

void ecl_inter_copy_well_names__(char *well_string) {
  char **tmp_well_list = ecl_sum_alloc_well_names_copy(ECL_SUM);
  int  N = ecl_sum_get_Nwells(ECL_SUM);
  int i;
  
  well_string[0] = '|';
  well_string[1] = '\0';
  for (i=0; i < N; i++) {
    strcat(well_string , tmp_well_list[i]);
    strcat(well_string , "|");
  }
  for (i=0; i < N; i++) 
    free(tmp_well_list[i]);
  free(tmp_well_list);
}



void ecl_inter_sum_get__(const char *_well_name , const int *well_len, 
			 const char *_var_name  , const int *var_len, 
			 void *value, int *index) {
  char *well = util_alloc_cstring(_well_name , well_len);
  char *var  = util_alloc_cstring(_var_name  , var_len);
  *index = ecl_sum_iget1(ECL_SUM , 0 , well , var , value);
  free(well);
  free(var);
}
    


/******************************************************************/

/* static void ecl_inter_run_eclipse_static(int jobs , int max_running , int max_restart , int *submit_list , const char *base_run_path , const char *eclipse_base , int time_step , int fmt_out , int *exit_on_submit_int , int *use_lsf_int) { */
/*   const int sleep_time  = 2; */
/*   int job , i , submit_jobs; */
/*   ext_job_type ** jobList; */
/*   char run_file[256] , restart_file[256] , run_path[256] , id[64], summary_file[64]; */
  
/*   submit_jobs = 0; */
/*   if (exit_on_submit_int) */
/*     exit_on_submit = true; */
/*   else */
/*     exit_on_submit = false; */

/*   if (use_lsf_int) */
/*     use_lsf = true; */
/*   else */
/*     use_lsf = false; */
  
/*   for (job = 0; job < jobs; job++)  */
/*     submit_jobs += submit_list[job]; */
/*   jobList = calloc(submit_jobs , sizeof *jobList); */
  
/*   i = 0; */
/*   for (job = 0; job < jobs; job++) { */
/*     if (submit_list[job]) { */
/*       sprintf(run_path , "%s%04d" , base_run_path , job + 1);  */
/*       /\* */
/* 	For som fxxxing reason the *.run_lock file is not */
/* 	created when the simulation is submitted from  */
/* 	Geir's linux computer??? */

/* 	sprintf(run_file , "%s.run_lock" , eclipse_base); */
/*       *\/ */
/*       sprintf(run_file , "%s.PRT" , eclipse_base); */
/*       if (fmt_out) */
/* 	sprintf(restart_file , "%s.F%04d" , eclipse_base , time_step); */
/*       else */
/* 	sprintf(restart_file , "%s.X%04d" , eclipse_base , time_step); */
/*       sprintf(id,"Job: %04d" , job + 1); */
/*       jobList[i] = ext_job_alloc(id , "@eclipse < eclipse.in 2> /dev/null | grep filterXX" , NULL , run_path  , run_file , restart_file , max_restart , sleep_time , true , use_lsf); */
/*       i++; */
/*     } */
/*   } */
/*   sprintf(summary_file , "Jobsummary/summary_%04d", time_step); */
/*   ext_job_run_pool(submit_jobs , jobList , max_running , 10 , summary_file , exit_on_submit); */
/* } */


/* void ecl_inter_run_eclipse__(const char * __basedir , int *basedir_length,  */
/* 			     const char * __eclbase , int *eclbase_length ,  */
/* 			     int *jobs , int *max_running , int *max_restart,  */
/* 			     int *submit_list, int *time_step , int *fmt_out, */
/* 			     int *exit_on_submit) { */
/*   char *basedir = util_alloc_cstring(__basedir , *basedir_length); */
/*   char *eclbase = util_alloc_cstring(__eclbase , *eclbase_length); */
  
/*   printf("*****************************************************************\n"); */
/*   printf("* Skal kjore eclipse jobber .... \n"); */
/*   printf("*****************************************************************\n"); */
/*   ecl_inter_run_eclipse_static(*jobs , *max_running , *max_restart , submit_list , basedir , eclbase , *time_step , *fmt_out , exit_on_submit , 0); */
/*   free(basedir); */
/*   free(eclbase); */
/* } */


/*****************************************************************/

void ecl_inter_init_lsf__(const int  * sleep_time , const int *max_running,  const int *subexit_int, 
			  const char * _summary_path , const int * summary_path_len,
			  const char * _summary_file , const int * summary_file_len) {
  
  char *summary_file = util_alloc_cstring(_summary_file , summary_file_len);
  char *summary_path = util_alloc_cstring(_summary_path , summary_path_len);
  LSF_POOL = lsf_pool_alloc(*sleep_time , *max_running , util_intptr_2bool(subexit_int) , summary_path , summary_file , "bjobs -a" , "/tmp");
  free(summary_path);
  free(summary_file);
}



void ecl_inter_add_lsf_job__(const int  *iens, 
			     const char *_id            , const int *id_len,
			     const char *_run_path      , const int *run_path_len , 
			     const char *_restart_file  , const int *restart_file_len,
			     const char *_OK_file       , const int *OK_file_len,
			     const char *_fail_file     , const int *fail_file_len,
			     const int  *max_resubmit) {
  if (LSF_POOL == NULL) {
    fprintf(stderr,"%s - must call xxxx_lsf_init first - aborting \n",__func__);
    abort();
  }
  {
    char *restart_file  = util_alloc_cstring(_restart_file , restart_file_len); 
    char *run_path      = util_alloc_cstring(_run_path      , run_path_len);
    char *id            = util_alloc_cstring(_id            , id_len);
    char *fail_file     = util_alloc_cstring(_fail_file     , fail_file_len);
    char *OK_file       = util_alloc_cstring(_OK_file       , OK_file_len);
      
    lsf_pool_add_job(LSF_POOL , id, run_path , restart_file , OK_file , fail_file , *max_resubmit);
    free(run_path);
    free(restart_file);
    free(id);
    free(fail_file);
    free(OK_file);
    printf("%4d" , *iens); fflush(stdout);
  }
}


void ecl_inter_run_lsf__(int *exit_count) {
  if (LSF_POOL == NULL) {
    fprintf(stderr,"%s - must call xxxx_lsf_init first - aborting \n",__func__);
    abort();
  }
  
  *exit_count = lsf_pool_run_jobs(LSF_POOL);
}


void ecl_inter_free_lsf__() {
  if (LSF_POOL != NULL) 
    lsf_pool_free(LSF_POOL);
  LSF_POOL = NULL;
}


/*****************************************************************/

void ecl_inter_parse__(const char *_refcase_path , const int * refcase_len,
		       const char *_eclbase      , const int * eclbase_len, 
		       const char *_include_path , const int * include_len, 
		       const int  *fmt_file_int  , const int * unified_int) {
  bool fmt_file = util_intptr_2bool(fmt_file_int);
  bool unified  = util_intptr_2bool(unified_int);
  char *refcase_path = util_alloc_cstring(_refcase_path , refcase_len);
  char *eclbase      = util_alloc_cstring(_eclbase      , eclbase_len);
  char *include_path = util_alloc_cstring(_include_path , include_len);
  
  ecl_parse(refcase_path , eclbase , include_path , fmt_file , unified , ENDIAN_CONVERT);

  free(refcase_path);
  free(eclbase);
  free(include_path);
}


void ecl_inter_parse_grid__(const char *_refcase_path , const int * refcase_len,
			    const char *_eclbase      , const int * eclbase_len, 
			    const char *_include_path , const int * include_len, 
			    const int  *fmt_file_int ) {
  bool fmt_file = util_intptr_2bool(fmt_file_int);
  char *refcase_path = util_alloc_cstring(_refcase_path , refcase_len);
  char *eclbase      = util_alloc_cstring(_eclbase      , eclbase_len);
  char *include_path = util_alloc_cstring(_include_path , include_len);
  
  ecl_parse_grid(refcase_path , eclbase , include_path , fmt_file , ENDIAN_CONVERT);

  free(refcase_path);
  free(eclbase);
  free(include_path);
}




void ecl_inter_diag_ens_interactive__(const char *_eclbase_dir  , const int *dir_len,
				      const char *_eclbase_name , const int *name_len, 
				      int *fmt_file_int , int *unified_int) {
  char *eclbase_dir  = util_alloc_cstring(_eclbase_dir  , dir_len);
  char *eclbase_name = util_alloc_cstring(_eclbase_name , name_len);

  bool fmt_file = util_intptr_2bool(fmt_file_int);
  bool unified  = util_intptr_2bool(unified_int);
  
  ecl_diag_ens_interactive(eclbase_dir , eclbase_name , fmt_file , unified);
  free(eclbase_dir);
  free(eclbase_name);
}
		       
void ecl_inter_diag_make_gnuplot_interactive__() {
  ecl_diag_make_gnuplot_interactive();
}


void ecl_inter_unlink_path__(const char *_path , const int *path_len) {
  char *path = util_alloc_cstring(_path , path_len);
  
  util_unlink_path(path);
  free(path);

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

