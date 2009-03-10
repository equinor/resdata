#include <ecl_util.h>
#include <util.h>
#include <string.h>
#include <ecl_smspec.h>
#include <ecl_sum_data.h>
#include <ecl_kw.h>
#include <vector.h>
#include <ecl_file.h>



/*
  This file implements the type ecl_sum_data_type. The data structure 
  is involved with holding all the actual summary data (i.e. the
  PARAMS vectors in ECLIPSE speak), in addition the time-information
  with MINISTEPS / REPORT_STEPS and so on is implemented here. 

  This file has no information about how to index into the PARAMS
  vector, i.e. at which location can the WWCT for well P6 be found,
  that is responsability of the ecl_smspec_type.

  The time direction in this system is implemented in terms of
  ministeps. There are some query / convert functons based on report
  steps.
*/


/*****************************************************************/
/*
   About ministeps and report steps.
   ---------------------------------

   A sequence of summary data will typically look like this:

   ------------------
   SEQHDR            \
   MINISTEP  0        |     
   PARAMS    .....    |
   MINISTEP  1        |==> This is REPORT STEP 1, in file BASE.S00001
   PARAMS    .....    |
   MINISTEP  2        |
   PARAMS    .....   /
   ------------------
   SEQHDR            \
   MINISTEP  3        |
   PARAMS    .....    |
   MINISTEP  4	      |
   PARAMS    .....    |
   MINISTEP  5	      |==> This is REPORT STEP 2, in file BASE.S0002
   PARAMS    .....    |
   MINISTEP  6	      |
   PARAMS    .....    |
   SEQHDR	      |
   MINISTEP  7	      |
   PARAMS    .....   /
   ------------------


   Observe the following:

     * The MINISTEP counter runs continously, and does not
       differentiate between unified files and not unified files.

     * When using multiple files we can read off the report number
       from the filename, for unified files this is IMPOSSIBLE, and we
       just have to assume that the first block corresponds to
       report_step 1 and then count afterwards. 

     * When asking for a summary variable at a particular REPORT STEP
       (as we do in enkf) it is ambigous as to which ministep within
       the block one should use. The convention we have employed
       (which corresponds to the old RPTONLY based behaviour) is to
       use the last ministep in the block.

     * There is no BASE.SOOOO file.

*/   



#include <int_vector.h>

#define ECL_SUM_MINISTEP_ID 88631


typedef struct ecl_sum_ministep_struct ecl_sum_ministep_type;


struct ecl_sum_ministep_struct {
  int         		   __id;
  float       		 * data;          /* A memcpy copy of the PARAMS vector in ecl_kw instance - the raw data. */
  time_t      		   sim_time;      
  int         		   ministep;      
  int         		   report_step;
  double      		   sim_days;
  int         		   data_size;     /* NUmber of elements in data - only used for checking indices. */
};




struct ecl_sum_data_struct {
  vector_type 	         * data;                   /* Vector of ecl_sum_ministep_type instances. */
  const ecl_smspec_type  * smspec;                 /* A shared reference - only used for providing good error messages. */
  int                	   first_ministep;
  int                	   last_ministep; 
  int_vector_type  	 * report_first_ministep ; /* Indexed by report_step - giving first ministep in report_step.   */
  int_vector_type  	 * report_last_ministep;   /* Indexed by report_step - giving last ministep in report_step.    */   
  int_vector_type  	 * ministep_index;         /* Indexed by ministep - gives index in data - 
						      observe that we make no assumtpitons of time-ordering of the input - flexible ehh !? */
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
  ministep->data_size   = ecl_kw_get_size( param_kw );
  ministep->report_step = report_step;
  ministep->ministep    = ministep_nr;
  ecl_smspec_set_time_info( smspec , ministep->data , &ministep->sim_days , &ministep->sim_time);

  return ministep;
}


static double ecl_sum_ministep_iget(const ecl_sum_ministep_type * ministep , int index) {
  if ((index >= 0) && (index < ministep->data_size))
    return ministep->data[index];
  else {
    util_abort("%s: invalid index:%d Valid range: [0,%d) \n",__func__ , index , ministep->data_size);
    return -1;
  }
}

static time_t ecl_sum_ministep_get_sim_time(const ecl_sum_ministep_type * ministep) {
  return ministep->sim_time;
}


static double ecl_sum_ministep_get_sim_days(const ecl_sum_ministep_type * ministep) {
  return ministep->sim_days;
}



/*****************************************************************/


 void ecl_sum_data_free( ecl_sum_data_type * data ) {
  vector_free( data->data );
  int_vector_free( data->report_first_ministep );
  int_vector_free( data->report_last_ministep  );
  int_vector_free( data->ministep_index );
  free(data);
}



static ecl_sum_data_type * ecl_sum_data_alloc(const ecl_smspec_type * smspec) {
  ecl_sum_data_type * data = util_malloc( sizeof * data , __func__);
  data->data         = vector_alloc_new();
  data->first_ministep = -1;
  data->last_ministep  = -1;
  data->report_first_ministep = int_vector_alloc(10 , -1);
  data->report_last_ministep  = int_vector_alloc(10 , -1);
  data->ministep_index        = int_vector_alloc(10 , -1);
  data->smspec                = smspec;
  return data;
}


/* 
   One ecl_file corresponds to one report_step (limited by SEQHDR). 
*/
static void ecl_sum_data_add_ecl_file(ecl_sum_data_type * data         , 
				      int   report_step                , 
				      const ecl_file_type   * ecl_file , 
				      const ecl_smspec_type * smspec) {
  
  int num_ministep  = ecl_file_get_num_named_kw( ecl_file , "PARAMS");
  int ikw;
  for (ikw = 0; ikw < num_ministep; ikw++) {
    ecl_kw_type * ministep_kw = ecl_file_iget_named_kw( ecl_file , "MINISTEP" , ikw);
    ecl_kw_type * param_kw    = ecl_file_iget_named_kw( ecl_file , "PARAMS"   , ikw);

    ecl_sum_ministep_type * ministep;
    int ministep_nr = ecl_kw_iget_int( ministep_kw , 0 );
    ministep = ecl_sum_ministep_alloc( ministep_nr,
				       report_step , 
				       param_kw , 
				       smspec);

    
    if (data->first_ministep < 0) 
      data->first_ministep = ministep_nr;
    data->first_ministep = util_int_min( data->first_ministep , ministep_nr);
    
    if (data->last_ministep < 0) 
      data->last_ministep = ministep_nr;
    data->last_ministep = util_int_max( data->last_ministep , ministep_nr);
    
    {
      int index = vector_append_owned_ref( data->data , ministep , ecl_sum_ministep_free__);
      int_vector_iset( data->ministep_index , ministep->ministep , index );
    }
  }
}



ecl_sum_data_type * ecl_sum_data_fread_alloc(const ecl_smspec_type * smspec , int files , const char ** filelist , bool endian_convert) {
  ecl_file_enum file_type;
  ecl_util_get_file_type( filelist[0] , &file_type , NULL , NULL);
  if ((files > 1) && (file_type != ecl_summary_file))
    util_abort("%s: internal error - when calling with more than one file - you can not supply a unified file - come on?! \n",__func__);
  {
    int filenr;
    ecl_sum_data_type * data = ecl_sum_data_alloc(smspec);
    if (file_type == ecl_summary_file) {

      /* Not unified. */
      for (filenr = 0; filenr < files; filenr++) {
	ecl_file_enum file_type;
	int report_step;
	ecl_util_get_file_type( filelist[filenr] , &file_type , NULL , &report_step);
	if (file_type != ecl_summary_file)
	  util_abort("%s: file:%s has wrong type \n",__func__ , filelist[filenr]);
	{
	  ecl_file_type * ecl_file = ecl_file_fread_alloc( filelist[filenr] , endian_convert );
	  ecl_sum_data_add_ecl_file( data , report_step , ecl_file , smspec);
	  ecl_file_free( ecl_file );
	}
      }
    } else if (file_type == ecl_unified_summary_file) {
      /* Loading a unified summary file. */
      bool fmt_file = ecl_util_fmt_file( filelist[0] );
      fortio_type * fortio = fortio_fopen( filelist[0] , "r" , endian_convert , fmt_file);
      bool complete = false;
      int report_step = 1; /* Corresponding to the first report_step in unified files - by assumption. */
      do {
	ecl_file_type * ecl_file = ecl_file_fread_alloc_summary_section( fortio );
	if (ecl_file != NULL) {
	  ecl_sum_data_add_ecl_file( data , report_step , ecl_file , smspec);
	  ecl_file_free( ecl_file );
	  report_step++;
	} else complete = true;
      } while ( !complete );
      fortio_fclose(fortio);
    } else
      util_abort("%s: invalid file type \n",__func__);

    /* OK - now we have loaded all the actual data. Must build up the report -> ministep mapping. */

    {
      int internal_index;
      for (internal_index = 0; internal_index < vector_get_size( data->data ); internal_index++) {
	const ecl_sum_ministep_type * ministep = vector_iget_const( data->data , internal_index );
	int report_step = ministep->report_step;
	int ministep_nr = ministep->ministep;
	
	{
	  int current_first_ministep =  int_vector_safe_iget( data->report_first_ministep , report_step );
	  if (current_first_ministep == -1) 
	    int_vector_iset( data->report_first_ministep , report_step , ministep_nr);
	  else
	    if (ministep_nr  < current_first_ministep)
	      int_vector_iset( data->report_first_ministep , report_step , ministep_nr);
	}


	{
	  int current_last_ministep =  int_vector_safe_iget( data->report_last_ministep , report_step );
	  if (current_last_ministep == -1)
	    int_vector_iset( data->report_last_ministep , report_step ,  ministep_nr);
	  else
	    if (ministep_nr > current_last_ministep)
	      int_vector_iset( data->report_last_ministep , report_step , ministep_nr);
	}
      }
    }
    return data;
  }
}


static void ecl_sum_data_summarize(const ecl_sum_data_type * data) { 
  printf("REPORT         MINISTEP               DATE                 DAYS\n");
  printf("---------------------------------------------------------------\n");
  {
    int ministep;
    for (ministep = 0; ministep < vector_get_size( data->data ); ministep++) {
      int internal_index = int_vector_safe_iget( data->ministep_index , ministep );
      if (internal_index >= 0) {
	const ecl_sum_ministep_type * ministep = vector_iget_const( data->data , internal_index);
	int day,month,year;
	util_set_date_values( ministep->sim_time , &day, &month , &year);
	printf("%04d          %6d               %02d/%02d/%4d           %7.2f \n", ministep->report_step , ministep->ministep , day,month,year, ministep->sim_days);
      }
    }
  }
  printf("---------------------------------------------------------------\n");
}


static const ecl_sum_ministep_type * ecl_sum_data_get_ministep( const ecl_sum_data_type * data , int ministep_nr) {
  int internal_index = int_vector_safe_iget( data->ministep_index , ministep_nr );
  if (internal_index < 0) {
    ecl_sum_data_summarize( data );
    
    util_abort("%s: Summary:%s object has no data for MINISTEP:%d - abortng \n",__func__ , ecl_smspec_get_simulation_case( data->smspec ) , ministep_nr);
    return NULL;
  }
  return vector_iget_const( data->data , internal_index );
}


/*****************************************************************/

bool  ecl_sum_data_has_report_step(const ecl_sum_data_type * data , int report_step ) {
  if (int_vector_safe_iget( data->report_first_ministep , report_step) >= 0)
    return true;
  else
    return false;
}


bool ecl_sum_data_has_ministep(const ecl_sum_data_type * data , int ministep) {
  if (int_vector_safe_iget( data->ministep_index , ministep ) >= 0)
    return true;
  else
    return false;
}


/**
   This function will update the pointers ministep1 and ministep2 with
   the first and last active ministep (inclusive range) for the
   current ecl_sum_data instance. Observe that in principle there can
   be holes in the ministep range, i.e. the follwoing example:

      ....
      sum_data = ecl_sum_data_fread_alloc(smspec , 2 , ["BASE.S0001 , "BASE.S0003"] , true);
      ecl_sum_get_ministep_range(sum_data , &ministep1 , &ministep2);
      for (ministep = ministep1; ministep <= ministep2; ministep++) 
           ecl_sum_data_get(sum_data , ministep , params_index); 

   will fail - because the ministeps corresponding to the file
   BASE.S0002 will be missing. To guard against this you must either
   be certain that the full set of summary data is read in (which will
   probably be the case in 99% of the cases anyway) ... or call the
   ecl_sum_data_has_ministep function to check explicitly.
*/

	   
  
void ecl_sum_data_get_ministep_range(const ecl_sum_data_type * data , int * ministep1, int * ministep2) {
  *ministep1 = data->first_ministep;
  *ministep2 = data->last_ministep;
}


/**
   This function will take a report as input , and update the two
   pointers ministep1 and ministep2 with the range of the report step
   (in terms of ministeps). 

   Calling this function with report_step == 2 for the example
   documented at the top of the file will yield: *ministep1 = 3 and
   *ministep2 = 7. If you are only interested in one of the limits you
   can pass in NULL for the other limit, i.e. 

      xxx(data , report_step , NULL , &ministep2); 

   to get the last step.
   
   If the supplied report_step is invalid the function will set both
   return values to -1 (the return value from safe_iget). In that case
   it is the responsability of the calling scope to check the return
   values, alternatively one can use the query function
   ecl_sum_data_has_report_step() first.
*/


void ecl_sum_data_report2ministep_range(const ecl_sum_data_type * data , int report_step , int * ministep1 , int * ministep2 ){
  if (ministep1 != NULL)
    *ministep1 = int_vector_safe_iget( data->report_first_ministep , report_step );
  
  if (ministep2 != NULL)
    *ministep2 = int_vector_safe_iget( data->report_last_ministep  , report_step ); 
}


/**
   This function will check that the ministep, and params_index are
   valid, and ABORT if that is not the case. There are several query
   functions which can be used to check prior to calling this
   function.
*/

double ecl_sum_data_get(const ecl_sum_data_type * data , int ministep , int params_index) {
  const ecl_sum_ministep_type * ministep_data = ecl_sum_data_get_ministep( data , ministep );  
  return ecl_sum_ministep_iget( ministep_data , params_index);
}



time_t ecl_sum_data_get_sim_time( const ecl_sum_data_type * data , int ministep ) {
  const ecl_sum_ministep_type * ministep_data = ecl_sum_data_get_ministep( data , ministep );
  return ecl_sum_ministep_get_sim_time( ministep_data );
}


double ecl_sum_data_get_sim_days( const ecl_sum_data_type * data , int ministep ) {
  const ecl_sum_ministep_type * ministep_data = ecl_sum_data_get_ministep( data , ministep );
  return ecl_sum_ministep_get_sim_days( ministep_data );
}
