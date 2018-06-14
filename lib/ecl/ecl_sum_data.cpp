/*
   Copyright (C) 2011  Statoil ASA, Norway.

   The file 'ecl_sum_data.c' is part of ERT - Ensemble based Reservoir Tool.

   ERT is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   ERT is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.

   See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html>
   for more details.
*/

#include <string.h>
#include <math.h>
#include <vector>
#include <stdexcept>
#include <string>

#include <ert/util/util.hpp>
#include <ert/util/vector.hpp>
#include <ert/util/time_t_vector.hpp>
#include <ert/util/int_vector.hpp>
#include <ert/util/stringlist.hpp>
#include <ert/util/test_util.hpp>

#include <ert/ecl/ecl_util.hpp>
#include <ert/ecl/ecl_smspec.hpp>
#include "ert/ecl/ecl_sum_file_data.hpp"
#include <ert/ecl/ecl_sum_data.hpp>
#include <ert/ecl/ecl_sum_tstep.hpp>
#include <ert/ecl/smspec_node.hpp>
#include <ert/ecl/ecl_kw.hpp>
#include <ert/ecl/ecl_file.hpp>
#include <ert/ecl/ecl_endian_flip.hpp>
#include <ert/ecl/ecl_kw_magic.hpp>
#include <ert/ecl/ecl_sum_vector.hpp>



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
   MINISTEP  4        |
   PARAMS    .....    |
   MINISTEP  5        |==> This is REPORT STEP 2, in file BASE.S0002
   PARAMS    .....    |
   MINISTEP  6        |
   PARAMS    .....    |
   SEQHDR             |
   MINISTEP  7        |
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

     * The report steps are halfopen intervals in the "wrong way":
       (....]




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


                                                 S0001                                          S0002
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



   Indexing and _get() versus _iget()
   ----------------------------------
   As already mentionded the set of ministeps is not necessarrily a
   continous series, we can easily have a series of ministeps with
   "holes" in it, and the series can also start on a non-zero
   value. Internally all the ministeps are stored in a dense, zero
   offset vector instance; and we must be able to translate back and
   forth between ministep_nr and internal index.

   Partly due to EnKF heritage the MINISTEP nr has been the main
   method to access the time dimension of the data, i.e. all the
   functions like ecl_sum_get_general_var() expect the time direction
   to be given as a ministep; however it is also possible to get the
   data by giving an invector_typeternal (not that internal ...) index. In
   ecl_sum_data.c the latter functions have _iget():


      ecl_sum_data_get_xxx : Expects the time direction given as a ministep_nr.
      ecl_sum_data_iget_xxx: Expects the time direction given as an internal index.

*/



struct ecl_sum_data_struct {
  ecl_smspec_type        * smspec;                 /* A shared reference - only used for providing good error messages. */
  double                   days_start;
  double                   sim_length;
  int_vector_type        * report_first_index ;    /* Indexed by report_step - giving first internal_index in report_step.   */
  int_vector_type        * report_last_index;      /* Indexed by report_step - giving last internal_index in report_step.    */
  int                      first_report_step;
  int                      last_report_step;
  bool                     index_valid;
  time_t                   start_time;             /* In the case of restarts the start might disagree with the value reported
                                                      in the smspec file. */
  time_t                   end_time;
  std::vector<ecl::ecl_sum_file_data_type*> data_list;              // List of ecl_sum_file_data instances

  size_t                   data_list_size;         //number of ecl_sum_file_data objects contained
  std::vector<size_t>      data_list_first_index;
  std::vector<size_t>      data_list_last_index;     

  std::vector<int *>       param_map; //maps each underlying ecl_sum_file_data_type into current file_data
};





/*****************************************************************/

 void ecl_sum_data_free( ecl_sum_data_type * data ) { 
  if (!data)
    throw std::invalid_argument(__func__ + std::string(": invalid delete") );
  if (data->data_list.size() > 0)
    delete data->data_list.back();

  for (size_t index = 0; index < data->param_map.size(); index++)
    free(data->param_map[index]);

  int_vector_free( data->report_first_index );
  int_vector_free( data->report_last_index  );
  delete data;
}

/*
  This function will clear/initialize all the mapping between
  ministep, report step and internal index. This function should be
  called before (re)building the indexes.
*/


static void ecl_sum_data_clear_index( ecl_sum_data_type * data ) {
  int_vector_reset( data->report_first_index);
  int_vector_reset( data->report_last_index);

  data->first_report_step     =  1024 * 1024;
  data->last_report_step      = -1024 * 1024;
  data->days_start            = 0;
  data->sim_length            = -1;
  data->index_valid           = false;
  data->start_time            = INVALID_TIME_T;
  data->end_time              = INVALID_TIME_T;
}


ecl_sum_data_type * ecl_sum_data_alloc(ecl_smspec_type * smspec) {
  ecl_sum_data_type * data =  new ecl_sum_data_type;
  data->report_first_index    = int_vector_alloc( 0 , INVALID_MINISTEP_NR );
  data->report_last_index     = int_vector_alloc( 0 , INVALID_MINISTEP_NR );
  data->data_list   = {};
  data->smspec      = smspec;
  data->data_list_size    = 0;
  data->data_list_first_index = {};
  data->data_list_last_index = {};
  data->param_map = { ecl_smspec_alloc_self_mapping( data->smspec ) };
  ecl_sum_data_clear_index( data );
  return data;
}


void ecl_sum_data_reset_self_map( ecl_sum_data_type * data ) {
  if (data->param_map.size() > 0) {
    for (size_t i = 0; i < data->param_map.size(); i++)
      free( data->param_map[i] );
  }
  data->param_map = { ecl_smspec_alloc_self_mapping( data->smspec ) };;
}

static void ecl_sum_data_append_file_data( ecl_sum_data_type * sum_data, ecl::ecl_sum_file_data_type * file_data) {

  sum_data->data_list.push_back( file_data );
  sum_data->data_list_first_index.push_back(0);
  sum_data->data_list_last_index.push_back( file_data->get_length() - 1 );

  sum_data->start_time = file_data->get_data_start();
  sum_data->end_time   = file_data->get_sim_end();
}


static int ecl_sum_data_get_list_index_from_ministep_index( const ecl_sum_data_type * data, size_t ministep_index ) {
  size_t index = 0;
  int data_list_index = -1;
  while (index < data->data_list.size()) {
    if (ministep_index >= data->data_list_first_index[index] && ministep_index <= data->data_list_last_index[index]) {
      data_list_index = index;
      break;
    }
    index++;
  }
  if (data_list_index < 0)
    throw std::invalid_argument(__func__ + std::string(": arg ministep index too high: ") + std::to_string(ministep_index) 
                                         + std::string(" >= ") + std::to_string( ecl_sum_data_get_length(data)));
  return data_list_index;
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


static ecl_sum_tstep_type * ecl_sum_data_iget_ministep( const ecl_sum_data_type * data , int internal_index ) {
  int list_index = ecl_sum_data_get_list_index_from_ministep_index( data, internal_index );
  return data->data_list[list_index]->iget_ministep( internal_index - data->data_list_first_index[list_index] );
}



void ecl_sum_data_report2internal_range(const ecl_sum_data_type * data , int report_step , int * index1 , int * index2 ){
  if (index1 != NULL)
    *index1 = int_vector_safe_iget( data->report_first_index , report_step );

  if (index2 != NULL)
    *index2 = int_vector_safe_iget( data->report_last_index  , report_step );
}



ecl_sum_data_type * ecl_sum_data_alloc_writer( ecl_smspec_type * smspec ) {
  ecl_sum_data_type * data = ecl_sum_data_alloc( smspec );
  ecl::ecl_sum_file_data_type * file_data = new ecl::ecl_sum_file_data_type( smspec );
  ecl_sum_data_append_file_data( data, file_data );
  return data;
}


static void ecl_sum_data_fwrite_report__( const ecl_sum_data_type * data , int report_step , fortio_type * fortio) {
  {
    ecl_kw_type * seqhdr_kw = ecl_kw_alloc( SEQHDR_KW , SEQHDR_SIZE , ECL_INT );
    ecl_kw_iset_int( seqhdr_kw , 0 , 0 );
    ecl_kw_fwrite( seqhdr_kw , fortio );
    ecl_kw_free( seqhdr_kw );
  }

  {
    int index , index1 , index2;

    ecl_sum_data_report2internal_range( data , report_step , &index1 , &index2);
    for (index = index1; index <= index2; index++) {
      const ecl_sum_tstep_type * tstep = ecl_sum_data_iget_ministep( data , index );
      ecl_sum_tstep_fwrite( tstep , ecl_smspec_get_index_map( data->smspec ) , fortio );
    }
  }
}



static void ecl_sum_data_fwrite_multiple_step( const ecl_sum_data_type * data , const char * ecl_case , bool fmt_case , int report_step) {
  char * filename = ecl_util_alloc_filename( NULL , ecl_case , ECL_UNIFIED_SUMMARY_FILE , fmt_case , 0 );
  fortio_type * fortio = fortio_open_readwrite( filename , fmt_case , ECL_ENDIAN_FLIP );

  ecl_sum_data_fwrite_report__( data , report_step , fortio );

  fortio_fclose( fortio );
  free(filename);
}


static void ecl_sum_data_fwrite_unified_step( const ecl_sum_data_type * data , const char * ecl_case , bool fmt_case , int report_step) {
  char * filename = ecl_util_alloc_filename( NULL , ecl_case , ECL_UNIFIED_SUMMARY_FILE , fmt_case , 0 );
  fortio_type * fortio = fortio_open_readwrite( filename , fmt_case , ECL_ENDIAN_FLIP );

  int current_step = 1;
  if (report_step > 1) {
    while (true) {
      if (ecl_kw_fseek_kw( SEQHDR_KW , false , false , fortio )) {
        if (current_step == report_step)
          break;
        current_step++;
      } else {
        current_step++;
        break;
      }
    }
  }

  if (current_step == report_step) { // We found the position:
    long size = fortio_ftell( fortio );

    util_ftruncate( fortio_get_FILE( fortio ) , size );
    ecl_sum_data_fwrite_report__( data , report_step , fortio );
  } else
    util_abort("%s: hmm could not locate the position for report step:%d in summary file:%s \n",__func__ , report_step , filename);

  fortio_fclose( fortio );
  free( filename );
}



static void ecl_sum_data_fwrite_unified( const ecl_sum_data_type * data , const char * ecl_case , bool fmt_case ) {
  char * filename = ecl_util_alloc_filename( NULL , ecl_case , ECL_UNIFIED_SUMMARY_FILE , fmt_case , 0 );
  fortio_type * fortio = fortio_open_writer( filename , fmt_case , ECL_ENDIAN_FLIP );

  for (size_t index = 0; index < data->data_list.size(); index++)
    data->data_list[index]->fwrite_unified( fortio );

  fortio_fclose( fortio );
  free( filename );
}


static void ecl_sum_data_fwrite_multiple( const ecl_sum_data_type * data , const char * ecl_case , bool fmt_case ) {
  
  for (size_t index = 0; index < data->data_list.size(); index++) 
    data->data_list[index]->fwrite_multiple(ecl_case, fmt_case);
  
}



void ecl_sum_data_fwrite_step( const ecl_sum_data_type * data , const char * ecl_case , bool fmt_case , bool unified, int report_step) {
  if (unified)
    ecl_sum_data_fwrite_unified_step( data , ecl_case , fmt_case , report_step);
  else
    ecl_sum_data_fwrite_multiple_step( data , ecl_case , fmt_case , report_step);
}


void ecl_sum_data_fwrite( const ecl_sum_data_type * data , const char * ecl_case , bool fmt_case , bool unified) {
  if (unified)
    ecl_sum_data_fwrite_unified( data , ecl_case , fmt_case );
  else
    ecl_sum_data_fwrite_multiple( data , ecl_case , fmt_case );
}





time_t ecl_sum_data_get_sim_end   (const ecl_sum_data_type * data ) { 
  return data->end_time;
}

time_t ecl_sum_data_get_data_start   ( const ecl_sum_data_type * data ) { 
  return data->start_time; 
}

double ecl_sum_data_get_first_day( const ecl_sum_data_type * data) { return data->days_start; }

/**
   Returns the number of simulations days from the start of the
   simulation (irrespective of whether the that summary data has
   actually been loaded) to the last loaded simulation step.
*/

double ecl_sum_data_get_sim_length( const ecl_sum_data_type * data ) {
  return data->sim_length;
}







/**
   The check_sim_time() and check_sim_days() routines check if you
   have summary data for the requested date/days value. In the case of
   a restarted case, where the original case is missing - this will
   return false if the input values are in the region after simulation
   start with no data.
*/

bool ecl_sum_data_check_sim_time( const ecl_sum_data_type * data , time_t sim_time) {
  if (sim_time < data->start_time)
    return false;

  if (sim_time > data->end_time)
    return false;

  return true;
}


bool ecl_sum_data_check_sim_days( const ecl_sum_data_type * data , double sim_days) {
  return sim_days >= data->days_start && sim_days <= data->sim_length;
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
   ministep numbers. Observe that if there are holes in the
   time-domain, i.e. if RPTONLY has been used, the function can return
   a ministep index which does NOT cover the input time:

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


static int ecl_sum_data_get_index_from_sim_time( const ecl_sum_data_type * data , time_t sim_time) {
  if (!ecl_sum_data_check_sim_time(data, sim_time)) {
    fprintf(stderr , "Simulation start: "); util_fprintf_date_utc( ecl_smspec_get_start_time( data->smspec ) , stderr );
    fprintf(stderr , "Data start......: "); util_fprintf_date_utc( data->start_time , stderr );
    fprintf(stderr , "Simulation end .: "); util_fprintf_date_utc( data->end_time , stderr );
    fprintf(stderr , "Requested date .: "); util_fprintf_date_utc( sim_time , stderr );
    util_abort("%s: invalid time_t instance:%d  interval:  [%d,%d]\n",__func__, sim_time , data->start_time , data->end_time);
  }

  /*
     The moment we have passed the intial test we MUST find a valid
     ministep index, however care should be taken that there can
     perfectly well be 'holes' in the time domain, because of e.g. the
     RPTONLY keyword.
  */

  int low_index = 0;
  int high_index = ecl_sum_data_get_length(data) - 1;

  // perform binary search
  while (low_index+1 < high_index) {
    int center_index = (low_index + high_index) / 2;
    const ecl_sum_tstep_type * center_step = ecl_sum_data_iget_ministep(data, center_index);
    const time_t center_time = ecl_sum_tstep_get_sim_time(center_step);

    if (sim_time > center_time)
      low_index = center_index;
    else
      high_index = center_index;
  }

  const ecl_sum_tstep_type * low_step = ecl_sum_data_iget_ministep(data, low_index);
  return sim_time <= ecl_sum_tstep_get_sim_time(low_step) ? low_index : high_index;
}

int ecl_sum_data_get_index_from_sim_days( const ecl_sum_data_type * data , double sim_days) {
  time_t sim_time = ecl_smspec_get_start_time( data->smspec );
  util_inplace_forward_days_utc( &sim_time , sim_days );
  return ecl_sum_data_get_index_from_sim_time(data , sim_time );
}


/**
   This function will take a true time 'sim_time' as input. The
   ministep indices bracketing this sim_time are identified, and the
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
   function should be used), consult documentation at the top of this
   file.
*/
void ecl_sum_data_init_interp_from_sim_time(const ecl_sum_data_type* data,
                                            time_t sim_time,
                                            int* index1,
                                            int* index2,
                                            double* weight1,
                                            double* weight2) {
  int idx = ecl_sum_data_get_index_from_sim_time(data, sim_time);

  // if sim_time is first date, idx=0 and then we cannot interpolate, so we give
  // weight 1 to index1=index2=0.
  if (idx == 0) {
    *index1 = 0;
    *index2 = 0;
    *weight1 = 1;
    *weight2 = 0;
    return;
  }

  const ecl_sum_tstep_type * ministep1 = ecl_sum_data_iget_ministep(data, idx-1);
  const ecl_sum_tstep_type * ministep2 = ecl_sum_data_iget_ministep(data, idx);

  time_t sim_time1 = ecl_sum_tstep_get_sim_time(ministep1);
  time_t sim_time2 = ecl_sum_tstep_get_sim_time(ministep2);

  *index1 = idx-1;
  *index2 = idx;

  // weights the interpolation each of the ministeps according to distance from sim_time
  double time_diff  = sim_time2 - sim_time1;
  double time_dist1 =  (sim_time - sim_time1);
  double time_dist2 = -(sim_time - sim_time2);

  *weight1 = time_dist2 / time_diff;
  *weight2 = time_dist1 / time_diff;
}



void ecl_sum_data_init_interp_from_sim_days( const ecl_sum_data_type * data , double sim_days, int *step1, int *step2 , double * weight1 , double *weight2) {
  time_t sim_time = ecl_smspec_get_start_time( data->smspec );
  util_inplace_forward_days_utc( &sim_time , sim_days );
  ecl_sum_data_init_interp_from_sim_time( data , sim_time , step1 , step2 , weight1 , weight2);
}


double_vector_type * ecl_sum_data_alloc_seconds_solution(const ecl_sum_data_type * data, const smspec_node_type * node, double cmp_value, bool rates_clamp_lower) {
  double_vector_type * solution = double_vector_alloc(0, 0);
  const int param_index = smspec_node_get_params_index(node);
  const int size = ecl_sum_data_get_length(data);

  if (size <= 1)
    return solution;

  for (int index = 0; index < size; ++index) {
    int prev_index = util_int_max(0, index-1);

    const ecl_sum_tstep_type * ministep = ecl_sum_data_iget_ministep(data, index);
    const ecl_sum_tstep_type * prev_ministep = ecl_sum_data_iget_ministep(data, prev_index);
    double value      = ecl_sum_data_iget(data, index, param_index);
    double prev_value = ecl_sum_data_iget(data, prev_index, param_index);

    // cmp_value in interval value (closed) and prev_value (open)
    bool contained = (value == cmp_value);
    contained |= (util_double_min(prev_value, value) < cmp_value) &&
            (cmp_value < util_double_max(prev_value, value));

    if (!contained)
      continue;

    double prev_time = ecl_sum_tstep_get_sim_seconds(prev_ministep);
    double time = ecl_sum_tstep_get_sim_seconds(ministep);

    if (smspec_node_is_rate(node)) {
      double_vector_append(solution, rates_clamp_lower ? prev_time + 1 : time);
    } else {
      double slope = (value - prev_value) / (time - prev_time);
      double seconds = (cmp_value - prev_value)/slope + prev_time;
      double_vector_append(solution, seconds);
    }
  }
  return solution;
}


static void ecl_sum_data_build_index( ecl_sum_data_type * sum_data ) {
  int_vector_reset( sum_data->report_first_index);
  int_vector_reset( sum_data->report_last_index); 
  for (size_t list_index = 0; list_index < sum_data->data_list.size(); list_index++) {
    sum_data->data_list[list_index]->update_report_vectors( sum_data->report_first_index, sum_data->report_last_index , 
                                                            sum_data->data_list_first_index[list_index], sum_data->data_list_last_index[list_index]);
  }
  if (sum_data->data_list.size() > 0) {
    const ecl_sum_tstep_type * first_ministep = ecl_sum_data_iget_ministep(sum_data, 0);
    const ecl_sum_tstep_type * last_ministep  = ecl_sum_data_iget_ministep(sum_data, ecl_sum_data_get_length(sum_data)-1);
    /*
       In most cases the days_start and data_start_time will agree
       with the global simulation start; however in the case where we
       have loaded a summary case from a restarted simulation where
       the case we have restarted from is not available - then there
       will be a difference.
    */
    sum_data->days_start      = ecl_sum_tstep_get_sim_days( first_ministep );
    sum_data->sim_length      = ecl_sum_tstep_get_sim_days( last_ministep );
    sum_data->start_time      = ecl_sum_tstep_get_sim_time( first_ministep);
    sum_data->end_time        = ecl_sum_tstep_get_sim_time( last_ministep );

    sum_data->first_report_step = sum_data->data_list[0]->get_first_report_step();
    sum_data->last_report_step = sum_data->data_list.back()->get_last_report_step();
  }
  sum_data->index_valid = true;
}



/*
  This function is meant to be called in write mode; and will create a
  new and empty tstep which is appended to the current data. The tstep
  will also be returned, so the calling scope can call
  ecl_sum_tstep_iset() to set elements in the tstep.
*/

ecl_sum_tstep_type * ecl_sum_data_add_new_tstep( ecl_sum_data_type * data , int report_step , double sim_seconds) {
  ecl::ecl_sum_file_data_type * file_data = data->data_list.back();
  ecl_sum_tstep_type * tstep = file_data->add_new_tstep( report_step, sim_seconds );
  if (tstep) {
    data->data_list_last_index.back()++;
  }
  ecl_sum_data_build_index( data );
  return tstep;
}


int * ecl_sum_data_alloc_param_mapping( int * current_param_mapping, int * old_param_mapping, size_t size) {
  int * new_param_mapping = (int*)util_malloc( size * sizeof * new_param_mapping );
  for (size_t i = 0; i < size; i++) {
    if (current_param_mapping[i] >= 0)
      new_param_mapping[i] = old_param_mapping[ current_param_mapping[i] ];
    else
      new_param_mapping[i] = -1;
  }
  return new_param_mapping;
}


void ecl_sum_data_add_case(ecl_sum_data_type * self, const ecl_sum_data_type * other) {

  int max_tstep_nr = -1;
  for (int tstep_nr = 0; tstep_nr < ecl_sum_data_get_length( other ); tstep_nr++) {
    ecl_sum_tstep_type * other_tstep = ecl_sum_data_iget_ministep( other , tstep_nr );
    if (ecl_sum_tstep_get_sim_time(other_tstep) < ecl_sum_data_get_data_start(self))
      max_tstep_nr = tstep_nr;
  }
  if (max_tstep_nr >= 0) {

    int * current_param_mapping = ecl_smspec_alloc_mapping( self->smspec , other->smspec );

    int max_other_list_index = ecl_sum_data_get_list_index_from_ministep_index(other, max_tstep_nr);
    self->data_list_first_index.back() += max_tstep_nr + 1;
    self->data_list_last_index.back() += max_tstep_nr + 1;

    for (int list_index = max_other_list_index; list_index >= 0; list_index--) {
      self->data_list.insert( self->data_list.begin(), other->data_list[list_index] );
      self->data_list_first_index.insert( self->data_list_first_index.begin(), other->data_list_first_index[list_index] );
      if (list_index == max_other_list_index)
        self->data_list_last_index.insert( self->data_list_last_index.begin(), max_tstep_nr );
      else
        self->data_list_last_index.insert( self->data_list_last_index.begin(), other->data_list_last_index[list_index] );

      self->param_map.insert( self->param_map.begin(), ecl_sum_data_alloc_param_mapping(current_param_mapping, other->param_map[list_index], ecl_smspec_get_params_size( self->smspec ) ) );

    }

    free( current_param_mapping );

   
  }

  ecl_sum_data_build_index(self);
  

}


/*
  Observe that this can be called several times (but not with the same
  data - that will die).

  Warning: The index information of the ecl_sum_data instance has
  __NOT__ been updated when leaving this function. That is done with a
  call to ecl_sum_data_build_index().
*/

bool ecl_sum_data_fread(ecl_sum_data_type * data , const stringlist_type * filelist) {
  ecl::ecl_sum_file_data_type * file_data = new ecl::ecl_sum_file_data_type( data->smspec );
  if (file_data->fread( filelist )) {
    ecl_sum_data_append_file_data( data, file_data );
    ecl_sum_data_build_index(data);
    return true;
  }
  return false;  
}









/**
   If the variable @include_restart is true the function will query
   the smspec object for restart information, and load summary
   information from case(s) which this case was restarted from (this
   only really applies to predictions where the basename has been
   (manually) changed from the historical part.
*/

ecl_sum_data_type * ecl_sum_data_fread_alloc( ecl_smspec_type * smspec , const stringlist_type * filelist , bool include_restart) {
  ecl_sum_data_type * data = ecl_sum_data_alloc( smspec );
  ecl_sum_data_fread( data , filelist );

  /*****************************************************************/
  /* OK - now we have loaded all the data. Must sort the internal
     storage vector, and build up various internal indexing vectors;
     this is done in a sepearate function.
  */
  ecl_sum_data_build_index( data );
  return data;
}


void ecl_sum_data_summarize(const ecl_sum_data_type * data , FILE * stream) {
  fprintf(stream , "REPORT         INDEX              DATE                 DAYS\n");
  fprintf(stream , "---------------------------------------------------------------\n");
  {
    int index;
    for (index = 0; index < ecl_sum_data_get_length(data); index++) {
      const ecl_sum_tstep_type * ministep = ecl_sum_data_iget_ministep( data , index );
      int day,month,year;
      ecl_util_set_date_values( ecl_sum_tstep_get_sim_time( ministep ) , &day, &month , &year);
      fprintf(stream , "%04d          %6d               %02d/%02d/%4d           %7.2f \n", ecl_sum_tstep_get_report( ministep ) , index , day,month,year, ecl_sum_tstep_get_sim_days( ministep ));
    }
  }
  fprintf(stream , "---------------------------------------------------------------\n");
}



/*****************************************************************/

bool ecl_sum_data_has_report_step(const ecl_sum_data_type * data , int report_step ) {
  if (int_vector_safe_iget( data->report_first_index , report_step) >= 0)
    return true;
  else
    return false;
}



/**
   Returns the last index included in report step @report_step.
   Observe that if the dataset does not include @report_step at all,
   the function will return INVALID_MINISTEP_NR; this must be checked for in the
   calling scope.
*/

int ecl_sum_data_iget_report_end( const ecl_sum_data_type * data , int report_step ) {
  return int_vector_safe_iget( data->report_last_index  , report_step );
}


/**
   Returns the first index included in report step @report_step.
   Observe that if the dataset does not include @report_step at all,
   the function will return INVALID_MINISTEP_NR; this must be checked for in the
   calling scope.
*/


int ecl_sum_data_iget_report_start( const ecl_sum_data_type * data , int report_step ) {
  return int_vector_safe_iget( data->report_first_index  , report_step );
}





int ecl_sum_data_iget_report_step(const ecl_sum_data_type * data , int internal_index) {
  const ecl_sum_tstep_type * ministep = ecl_sum_data_iget_ministep( data , internal_index );
  return ecl_sum_tstep_get_report( ministep );
}


int ecl_sum_data_iget_mini_step(const ecl_sum_data_type * data , int internal_index) {
  {
    const ecl_sum_tstep_type * ministep = ecl_sum_data_iget_ministep( data , internal_index );
    return ecl_sum_tstep_get_ministep( ministep );
  }
}


/**
    This will look up a value based on an internal index. The internal
    index will ALWAYS run in the interval [0,num_ministep), without
    any holes.
*/


double ecl_sum_data_iget( const ecl_sum_data_type * data , int time_index , int params_index ) {
  int list_index = ecl_sum_data_get_list_index_from_ministep_index(data, time_index);
  if (list_index < 0) 
    throw std::invalid_argument(__func__ + std::string(": wrong index.") );
  else {
    ecl::ecl_sum_file_data_type * file_data = data->data_list[list_index];
    int * param_map = data->param_map[list_index];
    return file_data->iget( time_index - data->data_list_first_index[list_index] , param_map[params_index] );
  }
  return 0;
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

double ecl_sum_data_interp_get(const ecl_sum_data_type * data , int time_index1 , int time_index2 , double weight1 , double weight2 , int params_index) {
  return ecl_sum_data_iget(data, time_index1, params_index) * weight1 + ecl_sum_data_iget(data, time_index2, params_index) * weight2;
}


static double ecl_sum_data_vector_iget(const ecl_sum_data_type * data,  time_t sim_time, int params_index, bool is_rate,
                                       int time_index1 , int time_index2 , double weight1 , double weight2 ) {

  double value = 0.0;
  if (is_rate) {
    int time_index = ecl_sum_data_get_index_from_sim_time(data, sim_time);
    // uses step function since it is a rate
    value = ecl_sum_data_iget(data, time_index, params_index);
  } else {
    // uses interpolation between timesteps
    value = ecl_sum_data_interp_get(data, time_index1, time_index2, weight1, weight2, params_index);
  }
  return value;
}

void ecl_sum_data_fwrite_interp_csv_line(const ecl_sum_data_type * data, time_t sim_time, const ecl_sum_vector_type * keylist, FILE *fp){
  int num_keywords = ecl_sum_vector_get_size(keylist);
  double weight1, weight2;
  int    time_index1, time_index2;

  ecl_sum_data_init_interp_from_sim_time(data, sim_time, &time_index1, &time_index2, &weight1, &weight2);

  for (int i = 0; i < num_keywords; i++) {
    if (ecl_sum_vector_iget_valid(keylist, i)) {
      int params_index = ecl_sum_vector_iget_param_index(keylist, i);
      bool is_rate = ecl_sum_vector_iget_is_rate(keylist, i);
      double value = ecl_sum_data_vector_iget( data, sim_time, params_index , is_rate, time_index1, time_index2, weight1, weight2);

      if (i == 0)
        fprintf(fp, "%f", value);
      else
        fprintf(fp, ",%f", value);
    } else {
      if (i == 0)
        fputs("", fp);
      else
        fputs(",", fp);
    }
  }
}


/*
  If the keylist contains invalid indices the corresponding element in the
  results vector will *not* be updated; i.e. it is smart to initialize the
  results vector with an invalid-value marker before calling this function:

  double_vector_type * results = double_vector_alloc( ecl_sum_vector_get_size(keys), NAN);
  ecl_sum_data_get_interp_vector( data, sim_time, keys, results);

*/

void ecl_sum_data_get_interp_vector( const ecl_sum_data_type * data , time_t sim_time, const ecl_sum_vector_type * keylist, double_vector_type * results){
  int num_keywords = ecl_sum_vector_get_size(keylist);
  double weight1, weight2;
  int    time_index1, time_index2;

  ecl_sum_data_init_interp_from_sim_time(data, sim_time, &time_index1, &time_index2, &weight1, &weight2);
  double_vector_reset( results );
  for (int i = 0; i < num_keywords; i++) {
    if (ecl_sum_vector_iget_valid(keylist, i)) {
      int params_index = ecl_sum_vector_iget_param_index(keylist, i);
      bool is_rate = ecl_sum_vector_iget_is_rate(keylist, i);
      double value = ecl_sum_data_vector_iget( data, sim_time, params_index, is_rate, time_index1, time_index2, weight1, weight2);
      double_vector_iset( results, i , value );
    }
  }
}

double ecl_sum_data_get_from_sim_time( const ecl_sum_data_type * data , time_t sim_time , const smspec_node_type * smspec_node) {
  int params_index = smspec_node_get_params_index( smspec_node );
  if (smspec_node_is_rate( smspec_node )) {
    /*
      In general the mapping from sim_time to index is based on half
      open intervals, which are closed in the upper end:

          []<------------]<--------------]<-----------]
          t0             t1             t2           t3

       However - as indicated on the figure above there is a zero
       measure point right at the start which corresponds to
       time_index == 0; this is to ensure that there is correspondance
       with the ECLIPSE results if you ask for a value interpolated to
       the starting time.
    */
    int time_index = ecl_sum_data_get_index_from_sim_time( data , sim_time );
    return ecl_sum_data_iget( data , time_index , params_index);
  } else {
    /* Interpolated lookup based on two (hopefully) consecutive ministeps. */
    double weight1 , weight2;
    int    time_index1 , time_index2;


    ecl_sum_data_init_interp_from_sim_time( data , sim_time , &time_index1 , &time_index2 , &weight1 , &weight2);
    return ecl_sum_data_interp_get( data , time_index1 , time_index2 , weight1 , weight2 , params_index);
  }
}


int ecl_sum_data_get_report_step_from_days(const ecl_sum_data_type * data , double sim_days) {
  if ((sim_days < data->days_start) || (sim_days > data->sim_length))
    return -1;
  else {
    int report_step = -1;

    double_vector_type * days_map = double_vector_alloc( 0 , 0 );
    int_vector_type    * report_map = int_vector_alloc( 0 , 0 );
    int i;

    for (i=1; i < int_vector_size( data->report_last_index ); i++) {
      int ministep_index = int_vector_iget( data->report_last_index , i );
      const ecl_sum_tstep_type * ministep = ecl_sum_data_iget_ministep(data, ministep_index);

      double_vector_iset( days_map , i , ecl_sum_tstep_get_sim_days( ministep ));
      int_vector_iset( report_map , i , ecl_sum_tstep_get_report( ministep ));
    }

    {
      /** Hmmmm - double == comparison ... */
      int index = double_vector_index_sorted( days_map , sim_days );

      if (index >= 0)
        report_step = int_vector_iget( report_map , index );
    }

    int_vector_free( report_map );
    double_vector_free( days_map );
    return report_step;
  }
}

/**
   Will go through the data and find the report step which EXACTLY
   matches the input sim_time. If no report step matches exactly the
   function will return -1.

   Observe that by default the report steps consist of half-open time
   intervals like this: (t1, t2]. However the first report step
   (i.e. report step 1, is a fully inclusive interval: [t0 , t1] where
   t0 is the simulation start time. That is not implemented here;
   meaning that if you supply the start time as @sim_time argument you
   will get -1 and not 0 as you might expect.

   It would certainly be possible to detect the start_time input
   argument and special case the return, but the opposite would be
   'impossible' - you would never get anything sensible out when using
   report_step == 0 as input to one of the functions expecting
   report_step input.
*/


int ecl_sum_data_get_report_step_from_time(const ecl_sum_data_type * data , time_t sim_time) {

  if (!ecl_sum_data_check_sim_time(data , sim_time))
    return -1;

  {
    int report_step = -1;

    time_t_vector_type * time_map = time_t_vector_alloc( 0 , 0 );
    int_vector_type    * report_map = int_vector_alloc( 0 , 0 );
    int i;

    for (i=1; i < int_vector_size( data->report_last_index ); i++) {
      int ministep_index = int_vector_iget( data->report_last_index , i );
      const ecl_sum_tstep_type * ministep = ecl_sum_data_iget_ministep(data, ministep_index);

      time_t_vector_iset( time_map , i , ecl_sum_tstep_get_sim_time( ministep ));
      int_vector_iset( report_map , i , ecl_sum_tstep_get_report( ministep ));

    }

    {
      int index = time_t_vector_index_sorted( time_map , sim_time );

      if (index >= 0)
        report_step = int_vector_iget( report_map , index );
    }

    int_vector_free( report_map );
    time_t_vector_free( time_map );
    return report_step;
  }
}


double ecl_sum_data_time2days( const ecl_sum_data_type * data , time_t sim_time) {
  time_t start_time = ecl_smspec_get_start_time( data->smspec );
  return util_difftime_days( start_time , sim_time );
}

double ecl_sum_data_get_from_sim_days( const ecl_sum_data_type * data , double sim_days , const smspec_node_type * smspec_node) {
  time_t sim_time = ecl_smspec_get_start_time( data->smspec );
  util_inplace_forward_days_utc( &sim_time , sim_days );
  return ecl_sum_data_get_from_sim_time( data , sim_time , smspec_node );
}


time_t ecl_sum_data_iget_sim_time( const ecl_sum_data_type * data , int internal_index ) {
  const ecl_sum_tstep_type * ministep_data = ecl_sum_data_iget_ministep( data , internal_index  );
  return ecl_sum_tstep_get_sim_time( ministep_data );
}


time_t ecl_sum_data_get_report_time( const ecl_sum_data_type * data , int report_step) {
  if (report_step == 0)
    return ecl_smspec_get_start_time( data->smspec );
  else {
    int internal_index = ecl_sum_data_iget_report_end( data , report_step );
    const ecl_sum_tstep_type * ministep = ecl_sum_data_iget_ministep( data , internal_index );
    return ecl_sum_tstep_get_sim_time( ministep );
  }
}


double ecl_sum_data_iget_sim_days( const ecl_sum_data_type * data , int internal_index ) {
  const ecl_sum_tstep_type * ministep_data = ecl_sum_data_iget_ministep( data , internal_index );
  return ecl_sum_tstep_get_sim_days( ministep_data );
}


int ecl_sum_data_get_first_report_step( const ecl_sum_data_type * data ) {
  return data->first_report_step;
}


int ecl_sum_data_get_last_report_step( const ecl_sum_data_type * data ) {
  return data->last_report_step;
}


/*****************************************************************/
/* High level vector routines */



void ecl_sum_data_init_time_vector( const ecl_sum_data_type * data , time_t_vector_type * time_vector , bool report_only) {
  time_t_vector_reset( time_vector );
  time_t_vector_append( time_vector , ecl_smspec_get_start_time( data->smspec ));
  if (report_only) {
    int report_step;
    for (report_step = data->first_report_step; report_step <= data->last_report_step; report_step++) {
      int last_index = int_vector_iget(data->report_last_index , report_step);
      const ecl_sum_tstep_type * ministep = ecl_sum_data_iget_ministep( data , last_index );
      time_t_vector_append( time_vector , ecl_sum_tstep_get_sim_time( ministep ) );
    }
  } else {
    int i;
    for (i = 1; i < ecl_sum_data_get_length(data); i++) {
      const ecl_sum_tstep_type * ministep = ecl_sum_data_iget_ministep( data , i  );
      time_t_vector_append( time_vector , ecl_sum_tstep_get_sim_time( ministep ));
    }
  }
}

time_t_vector_type *  ecl_sum_data_alloc_time_vector( const ecl_sum_data_type * data , bool report_only) {
  time_t_vector_type * time_vector = time_t_vector_alloc(0,0);
  ecl_sum_data_init_time_vector( data , time_vector , report_only);
  return time_vector;
}


void ecl_sum_data_init_data_vector( const ecl_sum_data_type * data , double_vector_type * data_vector , int data_index , bool report_only) {
  double_vector_reset( data_vector );
  double_vector_append( data_vector , ecl_smspec_get_start_time( data->smspec ));
  if (report_only) {
    int report_step;
    for (report_step = data->first_report_step; report_step <= data->last_report_step; report_step++) {
      int last_index = int_vector_iget(data->report_last_index , report_step);
      double_vector_append(data_vector, ecl_sum_data_iget(data, last_index, data_index));
    }
  } else {
    int i;
    for (i = 0; i < ecl_sum_data_get_length(data); i++)
      double_vector_append(data_vector, ecl_sum_data_iget(data, i, data_index));
  }
}


double_vector_type * ecl_sum_data_alloc_data_vector( const ecl_sum_data_type * data , int data_index , bool report_only) {
  double_vector_type * data_vector = double_vector_alloc(0,0);
  double_vector_reset( data_vector );
  double_vector_append( data_vector , ecl_smspec_get_start_time( data->smspec ));

  for (size_t index = 0; index < data->data_list.size(); index++) {
    int end_min_step = data->data_list_last_index[index] - data->data_list_first_index[index]; //last step in the file_data to be used
    int * param_map = data->param_map[index];
    data->data_list[index]->update_data_vector( data_vector, param_map[data_index], report_only, end_min_step);
  }

  return data_vector;
}


void ecl_sum_data_init_double_vector(const ecl_sum_data_type * data, int params_index, double * output_data) {
  int i;
  for (i = 0; i < ecl_sum_data_get_length(data); i++)
    output_data[i] = ecl_sum_data_iget(data, i, params_index);
}

void ecl_sum_data_init_datetime64_vector(const ecl_sum_data_type * data, int64_t * output_data, int multiplier) {
  int i;
  for (i = 0; i < ecl_sum_data_get_length(data); i++) {
    const ecl_sum_tstep_type * ministep = ecl_sum_data_iget_ministep( data , i  );
    output_data[i] = ecl_sum_tstep_get_sim_time(ministep) * multiplier;
  }
}

void ecl_sum_data_init_double_vector_interp(const ecl_sum_data_type * data,
                                            const smspec_node_type * smspec_node,
                                            const time_t_vector_type * time_points,
                                            double * output_data) {
  bool is_rate = smspec_node_is_rate(smspec_node);
  int params_index = smspec_node_get_params_index(smspec_node);
  time_t start_time = ecl_sum_data_get_data_start(data);
  time_t end_time   = ecl_sum_data_get_sim_end(data);
  double start_value = 0;
  double end_value = 0;

  if (!is_rate) {
    start_value = ecl_sum_data_iget_first_value(data, params_index);
    end_value = ecl_sum_data_iget_last_value(data, params_index);
  }

  for (int time_index=0; time_index < time_t_vector_size(time_points); time_index++) {
    time_t sim_time = time_t_vector_iget( time_points, time_index);
    double value;
    if (sim_time < start_time)
      value = start_value;

    else if (sim_time > end_time)
      value = end_value;

    else {
      int time_index1, time_index2;
      double weight1, weight2;
      ecl_sum_data_init_interp_from_sim_time(data, sim_time, &time_index1, &time_index2, &weight1, &weight2);
      value = ecl_sum_data_vector_iget( data,
                                        sim_time,
                                        params_index,
                                        is_rate,
                                        time_index1,
                                        time_index2,
                                        weight1,
                                        weight2);
    }

    output_data[time_index] = value;
  }
}




void ecl_sum_data_init_double_frame(const ecl_sum_data_type * data, const ecl_sum_vector_type * keywords, double *output_data) {
  int time_stride = ecl_sum_vector_get_size(keywords);
  int key_stride = 1;
  for (int time_index=0; time_index < ecl_sum_data_get_length(data); time_index++) {
    for (int key_index = 0; key_index < ecl_sum_vector_get_size(keywords); key_index++) {
      int param_index = ecl_sum_vector_iget_param_index(keywords, key_index);
      int data_index = key_index*key_stride + time_index * time_stride;

      output_data[data_index] = ecl_sum_data_iget(data, time_index, param_index);
    }
  }
}


void ecl_sum_data_init_double_frame_interp(const ecl_sum_data_type * data,
                                           const ecl_sum_vector_type * keywords,
                                           const time_t_vector_type * time_points,
                                           double * output_data) {
  int num_keywords = ecl_sum_vector_get_size(keywords);
  int time_stride = num_keywords;
  int key_stride = 1;
  time_t start_time = ecl_sum_data_get_data_start(data);
  time_t end_time   = ecl_sum_data_get_sim_end(data);


  for (int time_index=0; time_index < time_t_vector_size(time_points); time_index++) {
    time_t sim_time = time_t_vector_iget( time_points, time_index);
    if (sim_time < start_time) {
      for (int key_index = 0; key_index < num_keywords; key_index++) {
        int param_index = ecl_sum_vector_iget_param_index(keywords, key_index);
        int data_index = key_index*key_stride + time_index*time_stride;
        bool is_rate = ecl_sum_vector_iget_is_rate(keywords, key_index);
        if (is_rate)
          output_data[data_index] = 0;
        else
          output_data[data_index] = ecl_sum_data_iget_first_value(data, param_index);
      }
    } else if (sim_time > end_time) {
      for (int key_index = 0; key_index < num_keywords; key_index++) {
        int param_index = ecl_sum_vector_iget_param_index(keywords, key_index);
        int data_index = key_index*key_stride + time_index*time_stride;
        bool is_rate = ecl_sum_vector_iget_is_rate(keywords, key_index);
        if (is_rate)
          output_data[data_index] = 0;
        else
          output_data[data_index] = ecl_sum_data_iget_last_value(data, param_index);
      }
    } else {
      double weight1, weight2;
      int    time_index1, time_index2;

      ecl_sum_data_init_interp_from_sim_time(data, sim_time, &time_index1, &time_index2, &weight1, &weight2);

      for (int key_index = 0; key_index < num_keywords; key_index++) {
        int param_index = ecl_sum_vector_iget_param_index(keywords, key_index);
        int data_index = key_index*key_stride + time_index*time_stride;
        bool is_rate = ecl_sum_vector_iget_is_rate(keywords, key_index);
        double value = ecl_sum_data_vector_iget( data, sim_time, param_index , is_rate, time_index1, time_index2, weight1, weight2);
        output_data[data_index] = value;
      }
    }
  }
}

/**
   This function will return the total number of ministeps in the
   current ecl_sum_data instance; but observe that actual series of
   ministeps can have non-zero offset and also "holes" in the series.
*/

int ecl_sum_data_get_length( const ecl_sum_data_type * data ) {
  return (int)data->data_list_last_index.back() + 1;
}

void ecl_sum_data_scale_vector(ecl_sum_data_type * data, int index, double scalar) {
  int len = ecl_sum_data_get_length(data);
  for (int i = 0; i < len; i++) {
    ecl_sum_tstep_type * ministep = ecl_sum_data_iget_ministep(data,i);
    ecl_sum_tstep_iscale(ministep, index, scalar);
  }
}

void ecl_sum_data_shift_vector(ecl_sum_data_type * data, int index, double addend) {
  int len = ecl_sum_data_get_length(data);
  for (int i = 0; i < len; i++) {
    ecl_sum_tstep_type * ministep = ecl_sum_data_iget_ministep(data,i);
    ecl_sum_tstep_ishift(ministep, index, addend);
  }
}

bool ecl_sum_data_report_step_equal( const ecl_sum_data_type * data1 , const ecl_sum_data_type * data2) {
  bool equal = true;
  if (int_vector_size( data1->report_last_index ) == int_vector_size(data2->report_last_index)) {
    int i;
    for (i = 0; i < int_vector_size( data1->report_last_index ); i++) {
      int time_index1 = int_vector_iget( data1->report_last_index , i );
      int time_index2 = int_vector_iget( data2->report_last_index , i );

      if ((time_index1 != INVALID_MINISTEP_NR) && (time_index2 != INVALID_MINISTEP_NR)) {
        const ecl_sum_tstep_type * ministep1 = ecl_sum_data_iget_ministep( data1 , time_index1 );
        const ecl_sum_tstep_type * ministep2 = ecl_sum_data_iget_ministep( data2 , time_index2 );

        if (!ecl_sum_tstep_sim_time_equal( ministep1 , ministep2)) {
          equal = false;
          break;
        }
      } else if (time_index1 != time_index2) {
        equal = false;
        break;
      }
    }
  } else
    equal = false;

  return equal;
}


bool ecl_sum_data_report_step_compatible( const ecl_sum_data_type * data1 , const ecl_sum_data_type * data2) {
  bool compatible = true;
  int min_size = util_int_min( int_vector_size( data1->report_last_index ) , int_vector_size( data2->report_last_index));
  int i;

  for (i = 0; i < min_size; i++) {
    int time_index1 = int_vector_iget( data1->report_last_index , i );
    int time_index2 = int_vector_iget( data2->report_last_index , i );

    if ((time_index1 != INVALID_MINISTEP_NR) && (time_index2 != INVALID_MINISTEP_NR)) {
      const ecl_sum_tstep_type * ministep1 = ecl_sum_data_iget_ministep( data1 , time_index1 );
      const ecl_sum_tstep_type * ministep2 = ecl_sum_data_iget_ministep( data2 , time_index2 );

      if (!ecl_sum_tstep_sim_time_equal( ministep1 , ministep2)) {
        compatible = false;
        break;
      }
    }
  }
  return compatible;
}


double ecl_sum_data_iget_last_value(const ecl_sum_data_type * data, int param_index) {
  return ecl_sum_data_iget(data, ecl_sum_data_get_length(data)-1, param_index);
}

double ecl_sum_data_get_last_value(const ecl_sum_data_type * data, int param_index) {
  return ecl_sum_data_iget_last_value(data, param_index);
}

double ecl_sum_data_iget_first_value(const ecl_sum_data_type * data, int param_index) {
  return ecl_sum_data_iget(data, 0, param_index);
}
