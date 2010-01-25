#include <ecl_util.h>
#include <util.h>
#include <string.h>
#include <ecl_smspec.h>
#include <ecl_sum_data.h>
#include <ecl_kw.h>
#include <vector.h>
#include <ecl_file.h>
#include <ecl_endian_flip.h>
#include <time_t_vector.h>
#include <stringlist.h>


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

     * There is no BASE.SOOOO file



   About MINISTEP, REPORTSTEP, rates and continous sim_time/sim_days:
   ------------------------------------------------------------------

   For ECLIPSE summary files the smallest unit of time resolution is
   called the ministep - a ministep corresponds to a time step in the
   underlying partial differential equation, i.e. the length of the
   timesteps is controlled by the simulator itself - there is no finer
   temporal resolution.
   
   The user has told the simulator to store (i.e. save to file
   results) the results at reportsteps. A reportstep will typically
   consist of several ministeps. The timeline below shows a simulation
   consisting of two reportsteps:


   S0000                                          S0001                                          S0002
   ||------|------|------------|------------------||----------------------|----------------------||
          M1     M2           M3                 M4                      M5                     M6     
   
   The first reportstep consist of four ministeps, the second
   reportstep consits of only two ministeps. As a user you have no
   control over the length/number of ministeps apart from:

      1. Indirectly through the TUNING keywords.
      2. A ministep will always end at a report step.


   RPTONLY: In conjunction with enkf it has been customary to use the
   keyword RPTONLY. This is purely a storage directive, the effect is
   that only the ministep ending at the REPORT step is reported,
   i.e. in the case above we would get the ministeps [M4 , M6], where
   the ministeps M4 and M6 will be unchanged, and there will be many
   'holes' in the timeline.

   About truetime: The ministeps have a finite length; this implies
   that

     [rates]: The ministep value is NOT actually an instantaneous
        value, it is the total production during the ministepd period
        - divided by the length of the ministep. I.e. it is an average
        value. (I.e. the differential time element dt is actually quite
        looong).

     [state]: For state variables (this will include total production
        of various phases), the ministep value corresponds to the
        reservoir state at THE END OF THE MINISTEP.

   This difference between state variables and rates implies a
   difference in how continous time-variables (in the middle of a
   ministep) are reported, i.e.


   S0000                                                      S0001
   ||--------------|---------------|------------X-------------||
                  M1              M2           /|\            M3
                                                | 
                                                |

   We have enteeed the sim_days/sim_time cooresponding to the location
   of 'X' on the timeline, i.e. in the middle of ministep M3. If we
   are interested in the rate at this time the function:

             ecl_sum_data_get_from_sim_time()

   will just return the M3 value, whereas if you are interested in
   e.g. pressure at this time the function will return a weighted
   average of the M2 and M3 values. Whether a variable in question is
   interpreted as a 'rate' is effectively determined by the
   ecl_smspec_set_rate_var() function in ecl_smspec.c.
*/   



#include <int_vector.h>

#define ECL_SUM_MINISTEP_ID 88631


typedef struct ecl_sum_ministep_struct ecl_sum_ministep_type;


struct ecl_sum_ministep_struct {
  UTIL_TYPE_ID_DECLARATION;
  float       		 * data;            /* A memcpy copy of the PARAMS vector in ecl_kw instance - the raw data. */
  time_t      		   sim_time;      
  int         		   ministep;      
  int         		   report_step;
  double      		   sim_days;        /* Accumulated simulation time up to this ministep. */
  int         		   data_size;       /* Number of elements in data - only used for checking indices. */
  int                      internal_index;  /* Used for lookups of the next / previous ministep based on an existing ministep. */
};




struct ecl_sum_data_struct {
  vector_type 	         * data;                   /* Vector of ecl_sum_ministep_type instances. */
  const ecl_smspec_type  * smspec;                 /* A shared reference - only used for providing good error messages. */
  time_t                   sim_end;
  double                   sim_length;
  int                	   first_ministep;
  int                	   last_ministep; 
  int_vector_type  	 * report_first_ministep ; /* Indexed by report_step - giving first ministep in report_step.   */
  int_vector_type  	 * report_last_ministep;   /* Indexed by report_step - giving last ministep in report_step.    */   
  int_vector_type  	 * ministep_index;         /* Indexed by ministep - gives index in data - 
						      observe that we make no assumtpitons of time-ordering of the input - flexible ehh !? */
  int                      first_report_step;
  int                      last_report_step;
};


static const ecl_sum_ministep_type * ecl_sum_data_get_ministep( const ecl_sum_data_type * data , int ministep_nr);

static void ecl_sum_ministep_free( ecl_sum_ministep_type * ministep ) {
  free( ministep->data );
  free( ministep );
}



static UTIL_SAFE_CAST_FUNCTION( ecl_sum_ministep , ECL_SUM_MINISTEP_ID)


static void ecl_sum_ministep_free__( void * __ministep) {
  ecl_sum_ministep_type * ministep = ecl_sum_ministep_safe_cast( __ministep );
  ecl_sum_ministep_free( ministep );
}




static ecl_sum_ministep_type * ecl_sum_ministep_alloc( int ministep_nr    	  ,
						       int report_step 	  ,
						       const ecl_kw_type * param_kw , 
						       const ecl_smspec_type * smspec) {
  
  ecl_sum_ministep_type * ministep = util_malloc( sizeof * ministep , __func__);
  UTIL_TYPE_ID_INIT( ministep , ECL_SUM_MINISTEP_ID);
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
  data->report_first_ministep = int_vector_alloc( 0 , -1 );  /* This -1 value is hard-wired around in the place - not good. */
  data->report_last_ministep  = int_vector_alloc( 0 , -1 );
  data->ministep_index        = int_vector_alloc( 0 , -1 );
  data->smspec                = smspec;
  data->first_report_step     =  1024 * 1024;
  data->last_report_step      = -1024 * 1024;
  data->sim_end               = -1;
  data->sim_length            = -1;
  return data;
}


time_t ecl_sum_data_get_sim_end   ( const ecl_sum_data_type * data ) { return data->sim_end; }


/**
   Returns the number of simulations days from the start of the
   simulation (irrespective of whether the that summary data has
   actually been loaded) to the last loaded simulation step.
*/

double ecl_sum_data_get_sim_length( const ecl_sum_data_type * data ) { 
  return data->sim_length; 
}




/**
   This function will return the ministep corresponding to a time_t
   instance 'sim_time'. The function will fail hard if the time_t is
   before the simulation start, or after the end of the
   simulation. Check with

       ecl_smspec_get_start_time() and ecl_sum_data_get_sim_end()

   first.

   See the documentation about report steps, ministeps and rates at
   the top of this file for how the sim_time relates to to the
   returned ministep_nr.

   The indices used in this function are the internal indices, and not
   ministep numbers. Observe that there are holes in the time-domain,
   i.e. if RPTONLY has been used, the function can return a ministep
   index which does NOT cover the input time:

     The 'X' should represent report times - the dashed lines
     represent the temporal extent of two ministeps. Outside the '--'
     area we do not have any results. The two ministeps we actually
     have are M15 and M25, i.e. there is a hole.


      X      .      +-----X            +----X
            /|\        M15               M25
             |
             |

     When asking for the ministep number at the location of the arrow,
     the function will return '15', i.e. the valid ministep following
     the sim_time. Of course - the ideal situation is if the time
     sequence has no holes.
*/



int ecl_sum_data_get_ministep_from_sim_time( const ecl_sum_data_type * data , time_t sim_time) {
  time_t sim_start = ecl_smspec_get_start_time( data->smspec );

  if ((sim_time < sim_start) || (sim_time > data->sim_end))
    util_abort("%s: invalid time_t instance:%d  interval:  [%d,%d]\n",__func__, sim_time , sim_start , data->sim_end);
  
  /* 
     The moment we have passed the intial test we MUST find a valid
     ministep index, however care should be taken that there can
     perfectly well be 'holes' in the time domain, because of e.g. the
     RPTONLY keyword.
  */
  {
    int  low_index      = 0;
    int  high_index     = vector_get_size( data->data );
    int  internal_index = -1;
    

    while (internal_index < 0) {
      if (low_index == high_index)
        internal_index = low_index;
      else {
        int center_index = 0.5*( low_index + high_index );
        const ecl_sum_ministep_type * ministep = vector_iget_const(data->data , center_index);
        
        if ((high_index - low_index) == 1) {
          /* Degenerate special case. */
          if (sim_time < ministep->sim_time)
            internal_index = low_index;
          else
            internal_index = high_index;
        } else {
          if (sim_time > ministep->sim_time)    /*     Low-----Center---X---High */
            low_index = center_index;
          else {
            time_t prev_time = sim_start;
            if (center_index > 0) {
              const ecl_sum_ministep_type * prev_step = vector_iget_const( data->data , center_index - 1 );
              prev_time = prev_step->sim_time;
            }
            
            if (prev_time < sim_time)
              internal_index = center_index; /* Found it */
            else
              high_index = center_index;
          }
        }
      }
    }

    /* Translation from internal indexing to ministep nr. */
    {
      const ecl_sum_ministep_type * ministep = vector_iget_const( data->data , internal_index );
      return ministep->ministep;
    }
  }
}

int ecl_sum_data_get_ministep_from_sim_days( const ecl_sum_data_type * data , double sim_days) {
  time_t sim_time = ecl_smspec_get_start_time( data->smspec );
  util_inplace_forward_days( &sim_time , sim_days );
  return ecl_sum_data_get_ministep_from_sim_time(data , sim_time );
}


/**
   This function will take a true time 'sim_time' as input. The
   ministep indices bracketing this sim_time is identified, and the
   corresponding weights are calculated. 

   The actual value we are interested in can then be computed with the
   ecl_sum_data_interp_get() function:


   int    param_index;
   time_t sim_time;
   {
      int    ministep1 , ministep2;
      double weight1   , weight2;

      ecl_sum_data_init_interp_from_sim_time( data , sim_time , &ministep1 , &ministep2 , &weight1 , &weight2);
      return ecl_sum_data_interp_get( data , ministep1 , ministep2 , weight1 , weight2 , param_index );
   }
   
   
   For further explanation (in particular for which keywords the
   function should be used), consult documentation at the top of
   this file.
*/



void ecl_sum_data_init_interp_from_sim_time( const ecl_sum_data_type * data , time_t sim_time, int *_step1, int *_step2 , double * _weight1 , double *_weight2) {
  int     step2                           = ecl_sum_data_get_ministep_from_sim_time( data , sim_time);
  const ecl_sum_ministep_type * ministep2 = ecl_sum_data_get_ministep( data , step2 );
  int   step1;
  const ecl_sum_ministep_type * ministep1;
  {
    int     internal_index1 = ministep2->internal_index - 1; /* We want the previous timestep - not actually considering if there is hole on the temporal series. */
    ministep1 = vector_iget_const( data->data , internal_index1);
    step1 = ministep1->ministep;
  }
    
  
  double  weight2    =  (sim_time - ecl_sum_ministep_get_sim_time( ministep1 ));
  double  weight1    = -(sim_time - ecl_sum_ministep_get_sim_time( ministep2 ));

  
  *_step1   = step1;
  *_step2   = step2;
  *_weight1 = weight1 / ( weight1 + weight2 );
  *_weight2 = weight2 / ( weight1 + weight2 );
}



void ecl_sum_data_init_interp_from_sim_days( const ecl_sum_data_type * data , double sim_days, int *step1, int *step2 , double * weight1 , double *weight2) {
  time_t sim_time = ecl_smspec_get_start_time( data->smspec );
  util_inplace_forward_days( &sim_time , sim_days );
  ecl_sum_data_init_interp_from_sim_time( data , sim_time , step1 , step2 , weight1 , weight2);
}





static void ecl_sum_data_append_ministep( ecl_sum_data_type * data , int ministep_nr , ecl_sum_ministep_type * ministep) {
  if (data->first_ministep < 0) 
    data->first_ministep = ministep_nr;
  data->first_ministep = util_int_min( data->first_ministep , ministep_nr);
    
  if (data->last_ministep < 0) 
    data->last_ministep = ministep_nr;
  data->last_ministep = util_int_max( data->last_ministep , ministep_nr);
  
  {
    int index = vector_append_owned_ref( data->data , ministep , ecl_sum_ministep_free__);
    ministep->internal_index = index;
    int_vector_iset( data->ministep_index , ministep->ministep , index );
  }
  
  data->sim_length = util_double_max( data->sim_length , ministep->sim_days );
  data->sim_end    = util_time_t_max( data->sim_end    , ministep->sim_time );
}



/**
   Malformed/incopmplete files:
   ----------------------------
   Observe that ECLIPSE works in the following way:

     1. At the start of a report step a summary data section
        containing only the 'SEQHDR' keyword is written - this is
        currently a an 'invalid' summary section.

     2. ECLIPSE simulates as best it can.

     3. When the time step is complete data is written to the summary
        file.

   Now - if ECLIPSE goes down in flames during step 2 a malformed
   summary file will be left around, to handle this situation
   reasonably gracefully we check that the ecl_file instance has at
   least one "PARAMS" keyword.

   One ecl_file corresponds to one report_step (limited by SEQHDR). 
*/
static void ecl_sum_data_add_ecl_file(ecl_sum_data_type * data         , 
                                      int   report_step                , 
                                      const ecl_file_type   * ecl_file , 
                                      const ecl_smspec_type * smspec) {
  
  
  int num_ministep  = ecl_file_get_num_named_kw( ecl_file , "PARAMS");
  if (num_ministep > 0) {
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
      
      ecl_sum_data_append_ministep( data , ministep_nr , ministep );
      
    }
    data->first_report_step = util_int_min( data->first_report_step , report_step );
    data->last_report_step  = util_int_max( data->last_report_step  , report_step );
  }
}



ecl_sum_data_type * ecl_sum_data_fread_alloc(const ecl_smspec_type * smspec , const stringlist_type * filelist ) {
  ecl_file_enum file_type;
  ecl_util_get_file_type( stringlist_iget( filelist , 0 ) , &file_type , NULL , NULL);
  if ((stringlist_get_size( filelist ) > 1) && (file_type != ECL_SUMMARY_FILE))
    util_abort("%s: internal error - when calling with more than one file - you can not supply a unified file - come on?! \n",__func__);
  {
    int filenr;
    ecl_sum_data_type * data = ecl_sum_data_alloc(smspec);
    if (file_type == ECL_SUMMARY_FILE) {

      /* Not unified. */
      for (filenr = 0; filenr < stringlist_get_size( filelist ); filenr++) {
        const char * data_file = stringlist_iget( filelist , filenr );
	ecl_file_enum file_type;
	int report_step;
	ecl_util_get_file_type( data_file , &file_type , NULL , &report_step);

	/** 
	    ECLIPSE starts a report step by writing an empty summary
	    file, therefor we must verify that the ecl_file instance
	    returned by ecl_file_fread_alloc() is different from NULL
	    before adding it to the ecl_sum_data instance.
	*/
        if (file_type != ECL_SUMMARY_FILE)
          util_abort("%s: file:%s has wrong type \n",__func__ , data_file);
        {
          ecl_file_type * ecl_file = ecl_file_fread_alloc( data_file );
          if (ecl_file != NULL) {
            ecl_sum_data_add_ecl_file( data , report_step , ecl_file , smspec);
            ecl_file_free( ecl_file );
          } 
	}
      }
    } else if (file_type == ECL_UNIFIED_SUMMARY_FILE) {
      /* Loading a unified summary file. */
      bool fmt_file = ecl_util_fmt_file( stringlist_iget(filelist ,0 ) );
      fortio_type * fortio = fortio_fopen( stringlist_iget(filelist , 0) , "r" , ECL_ENDIAN_FLIP , fmt_file);
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
      util_abort("%s: invalid file type:%s  \n",__func__ , ecl_util_file_type_name( file_type ));


    /* OK - now we have loaded all the actual data. Must build up the report -> ministep mapping. */
    {
      int internal_index;
      for (internal_index = 0; internal_index < vector_get_size( data->data ); internal_index++) {
	const ecl_sum_ministep_type * ministep = vector_iget_const( data->data , internal_index );
	int report_step = ministep->report_step;
	int ministep_nr = ministep->ministep;
	{
	  int current_first_ministep = int_vector_safe_iget( data->report_first_ministep , report_step );
	  if (current_first_ministep < 0) /* i.e. currently not set. */
	    int_vector_iset( data->report_first_ministep , report_step , ministep_nr);
	  else
	    if (ministep_nr  < current_first_ministep)
	      int_vector_iset( data->report_first_ministep , report_step , ministep_nr);
	}


	{
	  int current_last_ministep =  int_vector_safe_iget( data->report_last_ministep , report_step );
	  if (current_last_ministep < 0)
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


void ecl_sum_data_summarize(const ecl_sum_data_type * data , FILE * stream) { 
  fprintf(stream , "REPORT         MINISTEP               DATE                 DAYS\n");
  fprintf(stream , "---------------------------------------------------------------\n");
  {
    int ministep;
    for (ministep = 0; ministep < vector_get_size( data->data ); ministep++) {
      int internal_index = int_vector_safe_iget( data->ministep_index , ministep );
      if (internal_index >= 0) {
	const ecl_sum_ministep_type * ministep = vector_iget_const( data->data , internal_index);
	int day,month,year;
	util_set_date_values( ministep->sim_time , &day, &month , &year);
	fprintf(stream , "%04d          %6d               %02d/%02d/%4d           %7.2f \n", ministep->report_step , ministep->ministep , day,month,year, ministep->sim_days);
      }
    }
  }
  fprintf(stream , "---------------------------------------------------------------\n");
}


static const ecl_sum_ministep_type * ecl_sum_data_get_ministep( const ecl_sum_data_type * data , int ministep_nr) {
  int internal_index = int_vector_safe_iget( data->ministep_index , ministep_nr );
  if (internal_index < 0) {
    ecl_sum_data_summarize( data , stderr);
    
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


/**
   This function will form a weight average of the two ministeps
   @ministep1 and @ministep2. The weights and the ministep indices
   should (typically) be determined by the 

      ecl_sum_data_init_interp_from_sim_xxx() 

   functions. The function will typically the last function called
   when we seek a reservoir state variable at an intermediate time
   between two ministeps.
*/

double ecl_sum_data_interp_get(const ecl_sum_data_type * data , int ministep1 , int ministep2 , double weight1 , double weight2 , int params_index) {
  const ecl_sum_ministep_type * ministep_data1 = ecl_sum_data_get_ministep( data , ministep1 );  
  const ecl_sum_ministep_type * ministep_data2 = ecl_sum_data_get_ministep( data , ministep2 );  
  
  return ecl_sum_ministep_iget( ministep_data1 , params_index ) * weight1 + ecl_sum_ministep_iget( ministep_data2 , params_index ) * weight2;
}



double ecl_sum_data_get_from_sim_time( const ecl_sum_data_type * data , time_t sim_time , int params_index) {
  if (ecl_smspec_is_rate( data->smspec , params_index)) {
    int ministep = ecl_sum_data_get_ministep_from_sim_time( data , sim_time );
    return ecl_sum_data_get( data , ministep , params_index);
  } else {
    /* Interpolated lookup based on two (hopefully) consecutive ministeps. */
    double weight1 , weight2;
    int    ministep1 , ministep2;
    

    ecl_sum_data_init_interp_from_sim_time( data , sim_time , &ministep1 , &ministep2 , &weight1 , &weight2);
    return ecl_sum_data_interp_get( data , ministep1 , ministep2 , weight1 , weight2 , params_index);
  }
}


double ecl_sum_data_get_from_sim_days( const ecl_sum_data_type * data , double sim_days , int params_index) {
  time_t sim_time = ecl_smspec_get_start_time( data->smspec );
  util_inplace_forward_days( &sim_time , sim_days );
  return ecl_sum_data_get_from_sim_time( data , sim_time , params_index );
}


time_t ecl_sum_data_get_sim_time( const ecl_sum_data_type * data , int ministep ) {
  const ecl_sum_ministep_type * ministep_data = ecl_sum_data_get_ministep( data , ministep );
  return ecl_sum_ministep_get_sim_time( ministep_data );
}


double ecl_sum_data_get_sim_days( const ecl_sum_data_type * data , int ministep ) {
  const ecl_sum_ministep_type * ministep_data = ecl_sum_data_get_ministep( data , ministep );
  return ecl_sum_ministep_get_sim_days( ministep_data );
}


int ecl_sum_data_get_first_report_step( const ecl_sum_data_type * data ) {
  return data->first_report_step;
}


int ecl_sum_data_get_last_report_step( const ecl_sum_data_type * data ) {
  return data->last_report_step;
}


int ecl_sum_data_get_first_ministep( const ecl_sum_data_type * data ) {
  return data->first_ministep;
}

int ecl_sum_data_get_last_ministep( const ecl_sum_data_type * data ) {
  return data->last_ministep;
}



time_t_vector_type * ecl_sum_data_alloc_time_vector( const ecl_sum_data_type * data , bool report_only) {
  time_t_vector_type * time_vector = time_t_vector_alloc(0,0);
  time_t_vector_append( time_vector , ecl_smspec_get_start_time( data->smspec ));
  if (report_only) {
    int report_step;
    for (report_step = data->first_report_step; report_step < data->last_report_step; report_step++) {
      const ecl_sum_ministep_type * ministep = ecl_sum_data_get_ministep( data , int_vector_iget(data->report_last_ministep , report_step));
      time_t_vector_append( time_vector , ministep->sim_time );
    }
  } else {
    int i;
    for (i = 0; i < vector_get_size(data->data); i++) {
      const ecl_sum_ministep_type * ministep = vector_iget_const( data->data , i );
      time_t_vector_append( time_vector , ministep->sim_time );
    }
  }
    
  
  return time_vector;
}
