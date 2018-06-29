#include <stdexcept>

#include "ert/ecl/ecl_sum_file_data.hpp"
#include <ert/ecl/ecl_sum_tstep.hpp>
#include <ert/ecl/ecl_kw.h>
#include <ert/ecl/ecl_kw_magic.h>
#include <ert/ecl/ecl_endian_flip.h>

namespace ecl {


ecl_sum_file_data_type::ecl_sum_file_data_type(ecl_smspec_type * smspec_) :
  smspec( smspec_ )
{
  data = vector_alloc_new();
  report_first_index = int_vector_alloc( 0 , INVALID_MINISTEP_NR );
  report_last_index  = int_vector_alloc( 0 , INVALID_MINISTEP_NR );

  clear_index( );

}

ecl_sum_file_data_type::~ecl_sum_file_data_type() {

  vector_free( data );
  int_vector_free( report_first_index );
  int_vector_free( report_last_index  );

}


int ecl_sum_file_data_type::get_length() const {
  return vector_get_size( data );
}

int  ecl_sum_file_data_type::get_first_report_step() const{
  return first_report_step;
}

int ecl_sum_file_data_type::get_last_report_step() const {
  return last_report_step;
}



time_t ecl_sum_file_data_type::get_data_start() const { return start_time; }
time_t ecl_sum_file_data_type::get_sim_end() const { return end_time; }

time_t ecl_sum_file_data_type::iget_sim_time(int time_index) const {
  const ecl_sum_tstep_type * ministep = iget_ministep( time_index  );
  return ecl_sum_tstep_get_sim_time(ministep);
}

double ecl_sum_file_data_type::iget_seconds(int time_index) const {
  throw;
  return 0;
}


double ecl_sum_file_data_type::iget( int time_index , int params_index ) const {
  if (params_index >= 0) {
    const ecl_sum_tstep_type * ministep_data = iget_ministep( time_index  );
    return ecl_sum_tstep_iget( ministep_data , params_index); 
  }
  else
    return 0;
}



void ecl_sum_file_data_type::clear_index() {
  int_vector_reset( report_first_index);
  int_vector_reset( report_last_index);

  first_report_step     =  1024 * 1024;
  last_report_step      = -1024 * 1024;
  days_start            = 0;
  sim_length            = -1;
  index_valid           = false;
  start_time            = INVALID_TIME_T;
  end_time              = INVALID_TIME_T;
}


void ecl_sum_file_data_type::append_tstep__(ecl_sum_tstep_type * tstep) {
  /*
     Here the tstep is just appended naively, the vector will be
     sorted by ministep_nr before the data instance is returned.
  */

  vector_append_owned_ref( data , tstep , ecl_sum_tstep_free__);
  index_valid = false;
}


/*
  This function is meant to be called in write mode; and will create a
  new and empty tstep which is appended to the current data. The tstep
  will also be returned, so the calling scope can call
  ecl_sum_tstep_iset() to set elements in the tstep.
*/

ecl_sum_tstep_type * ecl_sum_file_data_type::add_new_tstep( int report_step , double sim_seconds) {
  int ministep_nr = vector_get_size( data );
  ecl_sum_tstep_type * tstep = ecl_sum_tstep_alloc_new( report_step , ministep_nr , sim_seconds , smspec );
  ecl_sum_tstep_type * prev_tstep = NULL;

  if (vector_get_size( data ) > 0)
    prev_tstep = (ecl_sum_tstep_type*)vector_get_last( data );

  append_tstep__( tstep );
  {
    bool rebuild_index = true;

    /*
      In the simple case that we just add another timestep to the
      currently active report_step, we do a limited update of the
      index, otherwise we call ecl_sum_data_build_index() to get a
      full recalculation of the index.
    */

    if (prev_tstep != NULL) {
      if (ecl_sum_tstep_get_report( prev_tstep ) == ecl_sum_tstep_get_report( tstep )) {        // Same report step
        if (ecl_sum_tstep_get_sim_days( prev_tstep ) < ecl_sum_tstep_get_sim_days( tstep )) {   // This tstep will become the new latest tstep
          int internal_index = vector_get_size( data ) - 1;
          int_vector_iset( report_last_index , report_step , internal_index );

          sim_length = ecl_sum_tstep_get_sim_days( tstep );
          end_time = ecl_sum_tstep_get_sim_time(tstep);

          rebuild_index = false;
        }
      }
    }
    if (rebuild_index)
      build_index();
  }
  ecl_smspec_lock( smspec );

  return tstep;
}


ecl_sum_tstep_type * ecl_sum_file_data_type::iget_ministep( int internal_index ) const {
  return (ecl_sum_tstep_type*)vector_iget( data , internal_index );
}


static int cmp_ministep( const void * arg1 , const void * arg2) {
  const ecl_sum_tstep_type * ministep1 = ecl_sum_tstep_safe_cast_const( arg1 );
  const ecl_sum_tstep_type * ministep2 = ecl_sum_tstep_safe_cast_const( arg2 );

  time_t time1 = ecl_sum_tstep_get_sim_time( ministep1 );
  time_t time2 = ecl_sum_tstep_get_sim_time( ministep2 );

  if (time1 < time2)
    return -1;
  else if (time1 == time2)
    return 0;
  else
    return 1;
}


void ecl_sum_file_data_type::update_report_vectors( int_vector_type * parent_report_first_index, int_vector_type * parent_report_last_index, int first_ministep, int last_ministep ) {
  for (int index = 0; index < int_vector_size(report_first_index); index++) {  
     if (int_vector_iget( report_first_index, index ) != INVALID_MINISTEP_NR) {
        int first_parent_index = int_vector_iget( report_first_index , index ) + first_ministep;
        int last_parent_index  = int_vector_iget( report_last_index , index ) + first_ministep;
        if (first_parent_index <= last_ministep)
        {
           int_vector_append( parent_report_first_index, first_parent_index );
           if (last_parent_index > last_ministep)
             last_parent_index = last_ministep;
           int_vector_append( parent_report_last_index, last_parent_index );
        }
        else
           break;
     }
     else
     {
        int_vector_append( parent_report_first_index, INVALID_MINISTEP_NR );
        int_vector_append( parent_report_last_index, INVALID_MINISTEP_NR );
     }
  }
}


void ecl_sum_file_data_type::build_index( ) {
  /* Clear the existing index (if any): */
  clear_index();

  /*
    Sort the internal storage vector after sim_time.
  */
  vector_sort( data , cmp_ministep );


  /* Identify various global first and last values.  */
  {
    const ecl_sum_tstep_type * first_ministep = (const ecl_sum_tstep_type*)vector_iget_const( data, 0 );
    const ecl_sum_tstep_type * last_ministep  = (const ecl_sum_tstep_type*)vector_get_last_const( data );
    /*
       In most cases the days_start and data_start_time will agree
       with the global simulation start; however in the case where we
       have loaded a summary case from a restarted simulation where
       the case we have restarted from is not available - then there
       will be a difference.
    */
    days_start      = ecl_sum_tstep_get_sim_days( first_ministep );
    sim_length      = ecl_sum_tstep_get_sim_days( last_ministep );
    start_time      = ecl_sum_tstep_get_sim_time( first_ministep);
    end_time        = ecl_sum_tstep_get_sim_time( last_ministep );
  }

  /* Build up the report -> ministep mapping. */
  {
    int internal_index;
    for (internal_index = 0; internal_index < vector_get_size( data ); internal_index++) {
      const ecl_sum_tstep_type * ministep = iget_ministep( internal_index  );
      int report_step = ecl_sum_tstep_get_report(ministep);
      /* Indexing internal_index - report_step */
      {
        int current_first_index = int_vector_safe_iget( report_first_index , report_step );
        if (current_first_index < 0) /* i.e. currently not set. */
            int_vector_iset( report_first_index , report_step , internal_index);
        else
          if (internal_index  < current_first_index)
            int_vector_iset( report_first_index , report_step , internal_index);
      }

      {
        int current_last_index =  int_vector_safe_iget( report_last_index , report_step );
        if (current_last_index < 0)
          int_vector_iset( report_last_index , report_step ,  internal_index);
        else
          if (internal_index > current_last_index)
            int_vector_iset( report_last_index , report_step , internal_index);
      }

      first_report_step = util_int_min( first_report_step , report_step );
      last_report_step  = util_int_max( last_report_step  , report_step );
    }
  }
  index_valid = true;
}


void ecl_sum_file_data_type::update_data_vector( double_vector_type * data_vector , int data_index , bool report_only, int end_min_step) {

  if (end_min_step >= get_length())
    throw std::invalid_argument("ecl_sum_file_data_type::update_data_vector: argument 'end_min_step' too high.");

  if (report_only) {
    int report_step;
    for (report_step = first_report_step; report_step <= last_report_step; report_step++) {
      int last_index = int_vector_iget(report_last_index , report_step);
      if (last_index <= end_min_step)
        double_vector_append( data_vector, iget(last_index, data_index) );
    }
  } else {
    int i;
    for (i = 0; i <= end_min_step; i++)
      double_vector_append( data_vector, iget(i, data_index) );
  }
}


bool ecl_sum_file_data_type::has_report_step(int report_step ) const {
  if (int_vector_safe_iget( report_first_index , report_step) >= 0)
    return true;
  else
    return false;
}


// ******************** Start writing ***************************************************


void ecl_sum_file_data_type::report2internal_range(int report_step , int * index1 , int * index2 ) const {
  if (index1 != NULL)
    *index1 = int_vector_safe_iget( report_first_index , report_step );

  if (index2 != NULL)
    *index2 = int_vector_safe_iget( report_last_index  , report_step );
}


void ecl_sum_file_data_type::fwrite_report__( int report_step , fortio_type * fortio) const {
  {
    ecl_kw_type * seqhdr_kw = ecl_kw_alloc( SEQHDR_KW , SEQHDR_SIZE , ECL_INT );
    ecl_kw_iset_int( seqhdr_kw , 0 , 0 );
    ecl_kw_fwrite( seqhdr_kw , fortio );
    ecl_kw_free( seqhdr_kw );
  }

  {
    int index , index1 , index2;

    report2internal_range( report_step , &index1 , &index2);
    for (index = index1; index <= index2; index++) {
      const ecl_sum_tstep_type * tstep = iget_ministep( index );
      ecl_sum_tstep_fwrite( tstep , ecl_smspec_get_index_map( smspec ) , fortio );
    }
  }
}


void ecl_sum_file_data_type::fwrite_unified( fortio_type * fortio ) const {

  for (int report_step = first_report_step; report_step <= last_report_step; report_step++) {
    if (has_report_step( report_step ))
      fwrite_report__( report_step , fortio );
  }
}


void ecl_sum_file_data_type::fwrite_multiple( const char * ecl_case , bool fmt_case ) const {
  int report_step;

  for (report_step = first_report_step; report_step <= last_report_step; report_step++) {
    if (has_report_step( report_step )) {
      char * filename = ecl_util_alloc_filename( NULL , ecl_case , ECL_SUMMARY_FILE , fmt_case , report_step );
      fortio_type * fortio = fortio_open_writer( filename , fmt_case , ECL_ENDIAN_FLIP );

      fwrite_report__( report_step , fortio );

      fortio_fclose( fortio );
      free( filename );
    }
  }

}


// ***************************** End writing *************************************

// **************************** Start Reading ************************************

bool ecl_sum_file_data_type::check_file( ecl_file_type * ecl_file ) {
  if (ecl_file_has_kw( ecl_file , PARAMS_KW ) &&
      (ecl_file_get_num_named_kw( ecl_file , PARAMS_KW ) == ecl_file_get_num_named_kw( ecl_file , MINISTEP_KW)))
    return true;
  else
    return false;
}

/**
   Malformed/incomplete files:
   ----------------------------
   Observe that ECLIPSE works in the following way:

     1. At the start of a report step a summary data section
        containing only the 'SEQHDR' keyword is written - this is
        currently an 'invalid' summary section.

     2. ECLIPSE simulates as best it can.

     3. When the time step is complete data is written to the summary
        file.

   Now - if ECLIPSE goes down in flames during step 2 a malformed
   summary file will be left around, to handle this situation
   reasonably gracefully we check that the ecl_file instance has at
   least one "PARAMS" keyword.

   One ecl_file corresponds to one report_step (limited by SEQHDR); in
   the case of non unfied summary files these objects correspond to
   one BASE.Annnn or BASE.Snnnn file, in the case of unified files the
   calling routine will read the unified summary file partly.
*/

void ecl_sum_file_data_type::add_ecl_file(int report_step, const ecl_file_view_type * summary_view, const ecl_smspec_type * smspec) {

  int num_ministep  = ecl_file_view_get_num_named_kw( summary_view , PARAMS_KW);
  if (num_ministep > 0) {
    int ikw;

    for (ikw = 0; ikw < num_ministep; ikw++) {
      ecl_kw_type * ministep_kw = ecl_file_view_iget_named_kw( summary_view , MINISTEP_KW , ikw);
      ecl_kw_type * params_kw   = ecl_file_view_iget_named_kw( summary_view , PARAMS_KW   , ikw);

      {
        int ministep_nr = ecl_kw_iget_int( ministep_kw , 0 );
        ecl_sum_tstep_type * tstep = ecl_sum_tstep_alloc_from_file( report_step ,
                                                                    ministep_nr ,
                                                                    params_kw ,
                                                                    ecl_file_view_get_src_file( summary_view ),
                                                                    smspec );

        if (tstep)
            append_tstep__( tstep );
      }
    }
  }
}


bool ecl_sum_file_data_type::fread(const stringlist_type * filelist) {
  if (stringlist_get_size( filelist ) == 0)
    return false;

  {
    ecl_file_enum file_type = ecl_util_get_file_type( stringlist_iget( filelist , 0 ) , NULL , NULL);
    if ((stringlist_get_size( filelist ) > 1) && (file_type != ECL_SUMMARY_FILE))
      util_abort("%s: internal error - when calling with more than one file - you can not supply a unified file - come on?! \n",__func__);

    {
      int filenr;
      if (file_type == ECL_SUMMARY_FILE) {

        /* Not unified. */
        for (filenr = 0; filenr < stringlist_get_size( filelist ); filenr++) {
          const char * data_file = stringlist_iget( filelist , filenr);
          ecl_file_enum file_type;
          int report_step;
          file_type = ecl_util_get_file_type( data_file , NULL , &report_step);
          if (file_type != ECL_SUMMARY_FILE)
            util_abort("%s: file:%s has wrong type \n",__func__ , data_file);
          {
            ecl_file_type * ecl_file = ecl_file_open( data_file , 0);
            if (ecl_file && check_file( ecl_file )) {
              add_ecl_file( report_step , ecl_file_get_global_view( ecl_file ) , smspec);
              ecl_file_close( ecl_file );
            }
          }
        }
      } else if (file_type == ECL_UNIFIED_SUMMARY_FILE) {
        ecl_file_type * ecl_file = ecl_file_open( stringlist_iget(filelist ,0 ) , 0);
        if (ecl_file && check_file( ecl_file )) {
          int report_step = 1;   /* <- ECLIPSE numbering - starting at 1. */
          while (true) {
            /*
              Observe that there is a number discrepancy between ECLIPSE
              and the ecl_file_select_smryblock() function. ECLIPSE
              starts counting report steps at 1; whereas the first
              SEQHDR block in the unified summary file is block zero (in
              ert counting).
            */
            ecl_file_view_type * summary_view = ecl_file_get_summary_view(ecl_file , report_step - 1 );
            if (summary_view) {
              add_ecl_file(report_step , summary_view , smspec);
              report_step++;
            } else break;
          }
          ecl_file_close( ecl_file );
        }
      }
    }


    if (get_length() > 0) {
      build_index();
      return true;
    } else
      return false;

  }
}

// ***************************** End reading *************************************


} //end namespace


