#include <string.h>
#include <ecl_kw.h>
#include <ecl_fstate.h>
#include <ecl_sum.h>
#include <ext_job.h>
#include <lsf_jobs.h>
#include <ecl_parse.h>
#include <ecl_diag.h>
#include <util.h>
#include <sched.h>


static ecl_fstate_type * ECL_FSTATE     = NULL;
static bool              ENDIAN_CONVERT = true;
static ecl_sum_type    * ECL_SUM        = NULL;
static lsf_pool_type   * LSF_POOL       = NULL;

/******************************************************************/



void ecl_inter_load_file__(const char *__filename , const int *strlen) {
  char *filename = util_alloc_cstring(__filename , strlen);
  ECL_FSTATE = ecl_fstate_fread_alloc(1 , (const char **) &filename , ecl_other_file , false , ENDIAN_CONVERT);
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
  *Nstep = ecl_fstate_get_size(ECL_FSTATE);
}


/*
  Fortran index conventions in the call: These functions are called
  with 1-based indexes.
*/

void ecl_inter_kw_iget__(const char *_kw , const int * kw_len , const int *istep , const int *iw , void *value) {
  char *kw = util_alloc_cstring(_kw , kw_len);
  ecl_block_type * block   =  ecl_fstate_iget_block(ECL_FSTATE , (*istep) - 1);
  ecl_kw_type    * ecl_kw  =  ecl_block_get_kw(block , kw);
  ecl_kw_iget(ecl_kw , (*iw) - 1 , value);
  free(kw);
}


void ecl_inter_get_kw_size__(const char *_kw, const int *kw_len , const int *istep, int *size) {
  char *kw = util_alloc_cstring(_kw , kw_len);
  ecl_block_type * block  = ecl_fstate_iget_block(ECL_FSTATE , (*istep) - 1);
  ecl_kw_type    * ecl_kw = ecl_block_get_kw(block , kw);
  *size = ecl_kw_get_size(ecl_kw);
  free(kw);
}
  


void ecl_inter_kw_get_data__(const char *_kw , const int *kw_len , const int *istep , void *value) {
  char *kw = util_alloc_cstring(_kw , kw_len);
  ecl_block_type * block  = ecl_fstate_iget_block(ECL_FSTATE , (*istep) - 1);
  ecl_kw_type    * ecl_kw = ecl_block_get_kw(block , kw);
  ecl_kw_get_memcpy_data(ecl_kw , value);    
  free(kw);
}


/*
void ecl_inter_del_kw(const char *_kw, const int *kw_len , const int *istep) {
  char *kw = util_alloc_cstring(_kw , kw_len);
  ecl_block_type *ecl_block = ecl_fstate_iget_block(ECL_FSTATE , (*istep) - 1);
  ecl_block_free_kw(ecl_block , kw);
  free(kw);
}
*/

void ecl_inter_kw_exists__(const char *_kw , const int *kw_len , const int *istep , int *int_ex) {
  char *kw = util_alloc_cstring(_kw , kw_len);
  ecl_block_type *ecl_block = ecl_fstate_iget_block(ECL_FSTATE , (*istep) - 1);

  if (ecl_block_has_kw(ecl_block , kw))
    *int_ex = 1;
  else
    *int_ex =  0;

  free(kw);
}


void ecl_inter_load_summary__(const char *__header_file , const int *header_len , const char *__data_file , const int *data_len , const int *report_mode_int) {
  char * header_file = util_alloc_cstring(__header_file , header_len);
  char * data_file   = util_alloc_cstring(__data_file   , data_len);
  bool   report_mode = util_intptr_2bool(report_mode_int);
  ECL_SUM = ecl_sum_fread_alloc(header_file , 1 , (const char **) &data_file , report_mode , ENDIAN_CONVERT);
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
			 double *_value, int *_index) {
  char *well = util_alloc_cstring(_well_name , well_len);
  char *var  = util_alloc_cstring(_var_name  , var_len);

  if (ecl_sum_has_well_var(ECL_SUM , well , var)) {
    int index    = ecl_sum_get_index(ECL_SUM , well , var);
    double value = ecl_sum_iget(ECL_SUM , 0 , well , var);

    *_index = index;
    *_value = value;
  } else
    *_index = -1;
  
  free(well);
  free(var);
}
    

void ecl_inter_fwrite_param__(const char *_filename    , const int *filename_len,
			      const int  *fmt_file_int , 
			      const char *_header      , const int *header_len, 
			      const int  *size,          void *data) {
  char *filename = util_alloc_cstring(_filename , filename_len);
  char *header   = util_alloc_cstring(_header   , header_len);
  bool fmt_file  = util_intptr_2bool(fmt_file_int);
  ecl_kw_fwrite_param(filename , fmt_file , ENDIAN_CONVERT , header , ecl_float_type , *size , data);
  free(filename);
  free(header);
}




void ecl_inter_fread_param__(const char *_filename    , const int *filename_len,
			     const int  *fmt_file_int , double *data) {
  
  char *filename = util_alloc_cstring(_filename , filename_len);
  bool fmt_file  = util_intptr_2bool(fmt_file_int);
  ecl_kw_fread_double_param(filename , fmt_file , ENDIAN_CONVERT , data);
  free(filename);

}


void ecl_inter_fscanf_grdecl_param__(const char *_filename , const int * filename_len, const int * size, double * data) {
  char *filename = util_alloc_cstring(_filename , filename_len);
  ecl_kw_type * ecl_kw;
  {
    FILE * stream = util_fopen(filename , "r"); 
    ecl_kw = ecl_kw_fscanf_alloc_parameter(stream , *size , ENDIAN_CONVERT);
    fclose(stream);
  }
  ecl_kw_get_data_as_double(ecl_kw , data);
  ecl_kw_free(ecl_kw);
  free(filename);
}

/******************************************************************/

/* static*  void ecl_inter_run_eclipse_static(int jobs , int max_running , int max_restart , int *submit_list , const char *base_run_path , const char *eclipse_base , int time_step , int fmt_out , int *exit_on_submit_int , int *use_lsf_int) { */
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

void ecl_inter_init_lsf__(const int  * sleep_time    , const int *max_running,  const int *subexit_int, 
			  const char * _summary_path , const int * summary_path_len,
			  const char * _summary_file , const int * summary_file_len,
			  const int  * version_nr    , 
			  const char * _queu         , const int * queu_len,
			  const char * _request      , const int * request_len , 
			  const char * _submit_cmd   , const int * submit_cmd_len) {
  
  char *summary_file = util_alloc_cstring(_summary_file , summary_file_len);
  char *summary_path = util_alloc_cstring(_summary_path , summary_path_len);
  char *submit_cmd     = util_alloc_cstring(_submit_cmd , submit_cmd_len);
  char *request      = util_alloc_cstring(_request  , request_len);
  char *queu         = util_alloc_cstring(_queu     , queu_len);
  LSF_POOL = lsf_pool_alloc(*sleep_time , *max_running , util_intptr_2bool(subexit_int) , *version_nr , queu , request , summary_path , summary_file , "bjobs -a" , submit_cmd , "/tmp");
  free(summary_path);
  free(summary_file);
  free(submit_cmd);
  free(request);
  free(queu);
  printf("Submitting: "); fflush(stdout);
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
    printf("%04d" , *iens); fflush(stdout);
    printf("\b\b\b\b");
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
  bool fmt_file      = util_intptr_2bool(fmt_file_int);
  bool unified       = util_intptr_2bool(unified_int);
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
  
  ecl_parse_egrid(refcase_path , eclbase , include_path , fmt_file , ENDIAN_CONVERT);

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

  ecl_diag_ens_interactive(eclbase_dir , eclbase_name , fmt_file , unified , ENDIAN_CONVERT);
  free(eclbase_dir);
  free(eclbase_name);
}


void ecl_inter_avg_prod__(const char *_out_path     , const int *out_len,
			  const char *_eclbase_dir  , const int *dir_len,
			  const char *_avg_name     , const int *avg_len, 
			  const char *_std_name     , const int *std_len, 
			  int *fmt_file_int , int *unified_int) {
  char *eclbase_dir  = util_alloc_cstring(_eclbase_dir  , dir_len);
  char *avg_name = util_alloc_cstring(_avg_name , avg_len);
  char *std_name = util_alloc_cstring(_std_name , std_len);
  char *out_path = util_alloc_cstring(_out_path , out_len);
  

  /*
    bool fmt_file = util_intptr_2bool(fmt_file_int);
    bool unified  = util_intptr_2bool(unified_int);
    ecl_diag_avg_production_interactive(out_path , eclbase_dir , avg_name , std_name , fmt_file , unified);
  */
  fprintf(stderr,"%s: sorry calling unimplented function : ecl_diag_avg_production_interactive \n",__func__);
  abort();
  free(eclbase_dir);
  free(avg_name);
  free(std_name);
  free(out_path);
}
	



void ecl_inter_unlink_path__(const char *_path , const int *path_len) {
  char *path = util_alloc_cstring(_path , path_len);
  
  util_unlink_path(path);
  free(path);

}



void ecl_inter_update_init_file__(const char   *_src_file    , const int * src_file_len,
				  const char   *_target_file , const int * target_file_len,
				  const int    *_nactive,
				  const double *poro , const double * permx , const double *permz) {
  const int nactive  = *_nactive;
  bool   fmt_file    = false; /* Will follow src_file anyhow */
  char * src_file    = util_alloc_cstring(_src_file    , src_file_len);
  char * target_file = util_alloc_cstring(_target_file , target_file_len);
  float * float_data = util_malloc(nactive * sizeof * float_data , __func__);
  hash_type * kw_hash = hash_alloc(10);

  
  util_double_to_float(float_data , poro  , nactive); hash_insert_hash_owned_ref(kw_hash , "PORO"  , ecl_kw_alloc_complete(fmt_file , ENDIAN_CONVERT , "PORO"  , nactive , ecl_float_type , float_data) , ecl_kw_free__);
  util_double_to_float(float_data , permz , nactive); hash_insert_hash_owned_ref(kw_hash , "PERMZ" , ecl_kw_alloc_complete(fmt_file , ENDIAN_CONVERT , "PERMZ" , nactive , ecl_float_type , float_data) , ecl_kw_free__);
  util_double_to_float(float_data , permx , nactive); hash_insert_hash_owned_ref(kw_hash , "PERMX" , ecl_kw_alloc_complete(fmt_file , ENDIAN_CONVERT , "PERMX" , nactive , ecl_float_type , float_data) , ecl_kw_free__);
  util_double_to_float(float_data , permx , nactive); hash_insert_hash_owned_ref(kw_hash , "PERMY" , ecl_kw_alloc_complete(fmt_file , ENDIAN_CONVERT , "PERMY" , nactive , ecl_float_type , float_data) , ecl_kw_free__);
  
  ecl_fstate_filter_file(src_file , target_file , kw_hash , ENDIAN_CONVERT);

  hash_free(kw_hash);
  free(float_data);
  free(src_file);
  free(target_file);
}




