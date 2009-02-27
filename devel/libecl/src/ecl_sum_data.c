

#include <ecl_util.h>
#include <util.h>
#include <ecl_smspec.h>
#include <string.h>
#include <ecl_sum_data.h>
#include <ecl_kw.h>
#include <ecl_block.h>
#include <ecl_fstate.h>
#include <vector.h>
#include <int_vector.h>

#define ECL_SUM_MINISTEP_ID 88631

typedef struct ecl_sum_ministep_struct ecl_sum_ministep_type;


struct ecl_sum_ministep_struct {
  int     __id;
  float * data;
  time_t  sim_time;
  int     ministep;
  int     report_step;
  double  sim_days;
};




struct ecl_sum_data_struct {
  vector_type 	   * data;                   /* Vector of ecl_sum_ministep_type instances. */
  int                first_report;
  int                last_report; 
  int_vector_type  * report_first_ministep ; /* Indexed by report_step - giving first ministep in report_step.   */
  int_vector_type  * report_last_ministep;   /* Indexed by report_step - giving last ministep in report_step.    */   
  int_vector_type  * ministep_index;         /* Indexed by ministep - gives index in data. */
};




static void ecl_sum_ministep_free( ecl_sum_ministep_type * ministep ) {
  free( ministep->data );
  free( ministep );
}




static ecl_sum_ministep_type * ecl_sum_ministep_safe_cast( void * __ministep ) {
  ecl_sum_ministep_type * ministep = ( ecl_sum_ministep_type * )  __ministep;
  if (ministep->__id == ECL_SUM_MINISTEP_ID)
    return ministep;
  else {
    util_abort("%s: run_time cast failed - aborting \n",__func__);
    return NULL;
  }
}



static void ecl_sum_ministep_free__( void * __ministep) {
  ecl_sum_ministep_type * ministep = ecl_sum_ministep_safe_cast( __ministep );
  ecl_sum_ministep_free( ministep );
}




static ecl_sum_ministep_type * ecl_sum_ministep_alloc( int ministep_nr    	  ,
						       int report_step 	  ,
						       const ecl_kw_type * param_kw , 
						       const ecl_smspec_type * smspec) {
  
  ecl_sum_ministep_type * ministep = util_malloc( sizeof * ministep , __func__);
  
  ministep->__id        = ECL_SUM_MINISTEP_ID;
  ministep->data        = ecl_kw_alloc_data_copy( param_kw );
  ministep->report_step = report_step;
  ministep->ministep    = ministep_nr;
  ecl_smspec_set_time_info( smspec , ministep->data , &ministep->sim_days , &ministep->sim_time);

  return ministep;
}


static double ecl_sum_ministep_iget(const ecl_sum_ministep_type * ministep , int index) {
  return ministep->data[index];
}



/*****************************************************************/


 void ecl_sum_data_free( ecl_sum_data_type * data ) {
  vector_free( data->data );
  int_vector_free( data->report_first_ministep );
  int_vector_free( data->report_last_ministep  );
  int_vector_free( data->ministep_index );
  free(data);
}



static ecl_sum_data_type * ecl_sum_data_alloc() {
  ecl_sum_data_type * data = util_malloc( sizeof * data , __func__);
  data->data         = vector_alloc_new();
  data->first_report = -1;
  data->last_report  = -1;
  data->report_first_ministep = int_vector_alloc(10 , -1);
  data->report_last_ministep  = int_vector_alloc(10 , -1);
  data->ministep_index        = int_vector_alloc(10 , -1);
  return data;
}


/* 
   One ecl_block corresponds to one report_step (limited by SEQHDR). 
*/
static void ecl_sum_data_add_block(ecl_sum_data_type * data         , 
			    int   report_step , 
				 const ecl_block_type * ecl_block , 
			    const ecl_smspec_type * smspec) {

  int block_size  = ecl_block_get_kw_size( ecl_block , "PARAMS");
  int ikw;
  for (ikw = 0; ikw < block_size; ikw++) {
    ecl_kw_type * ministep_kw = ecl_block_iget_kw( ecl_block , "MINISTEP" , ikw);
    ecl_kw_type * param_kw    = ecl_block_iget_kw( ecl_block , "PARAMS"    , ikw);
    ecl_sum_ministep_type * ministep;

    ministep = ecl_sum_ministep_alloc( ecl_kw_iget_int( ministep_kw , 0 ),
				       report_step , 
				       param_kw , 
				       smspec);

    {
      int index = vector_append_owned_ref( data->data , ministep , ecl_sum_ministep_free__);
      int_vector_iset( data->ministep_index , ministep->ministep , index );
    }
  }
}


static void ecl_sum_data_add_file(ecl_sum_data_type * data , const ecl_smspec_type * smspec , const char * file  , ecl_file_type target_type , bool endian_convert ) {
  ecl_file_type file_type;
  ecl_util_get_file_type( file , &file_type , NULL , NULL);
  if (file_type != target_type)
    util_abort("%s: file:%s has wrong type \n",__func__ , file);
  
  {
    ecl_fstate_type * fstate = ecl_fstate_fread_alloc( 1 , (const char **) &file , file_type , endian_convert , false);
    int report_step = 0;
    int ib;
    for (ib=0; ib < ecl_fstate_get_size( fstate ); ib++) {
      if (file_type == ecl_summary_file)
	ecl_util_get_file_type( file , NULL , NULL , &report_step );

      ecl_sum_data_add_block(  data , report_step , ecl_fstate_iget_block( fstate , ib ) , smspec);
      report_step++;  /* For unified files. */
    }
    
    ecl_fstate_free( fstate );
  }
}


/*static*/ ecl_sum_data_type * ecl_sum_data_fread_alloc(const ecl_smspec_type * smspec , int files , const char ** filelist , bool endian_convert) {
  ecl_file_type file_type;
  ecl_util_get_file_type( filelist[0] , &file_type , NULL , NULL);
  if ((files > 1) && (file_type != ecl_summary_file))
    util_abort("%s: internal error - when calling with more than one file - you can not supply a unified file - come on?! \n",__func__);
  {
    int filenr;
    ecl_sum_data_type * data = ecl_sum_data_alloc();
    for (filenr = 0; filenr < files; filenr++)
      ecl_sum_data_add_file( data , smspec , filelist[filenr] , file_type , endian_convert);
    return data;
  }
}


bool ecl_sum_data_has_ministep(const ecl_sum_data_type * data , int ministep) {
  if (int_vector_safe_iget( data->ministep_index , ministep ) >= 0)
    return true;
  else
    return false;
}

