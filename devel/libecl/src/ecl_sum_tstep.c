 /*
   Copyright (C) 2012  Statoil ASA, Norway. 
    
   The file 'ecl_sum_tstep.c' is part of ERT - Ensemble based Reservoir Tool. 
    
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

#include <time.h>

#include <util.h>
#include <type_macros.h>

#include <ecl_sum_tstep.h>
#include <ecl_kw.h>
#include <ecl_smspec.h>
#include <ecl_kw_magic.h>

#define ECL_SUM_TSTEP_ID 88631

struct ecl_sum_tstep_struct {
  UTIL_TYPE_ID_DECLARATION;
  float                  * data;            /* A memcpy copy of the PARAMS vector in ecl_kw instance - the raw data. */
  time_t                   sim_time;      
  int                      ministep;      
  int                      report_step;
  double                   sim_days;        /* Accumulated simulation time up to this ministep. */
  int                      data_size;       /* Number of elements in data - only used for checking indices. */
  int                      internal_index;  /* Used for lookups of the next / previous ministep based on an existing ministep. */
};


UTIL_SAFE_CAST_FUNCTION( ecl_sum_tstep , ECL_SUM_TSTEP_ID)
UTIL_SAFE_CAST_FUNCTION_CONST( ecl_sum_tstep , ECL_SUM_TSTEP_ID)


void ecl_sum_tstep_free( ecl_sum_tstep_type * ministep ) {
  free( ministep->data );
  free( ministep );
}



void ecl_sum_tstep_free__( void * __ministep) {
  ecl_sum_tstep_type * ministep = ecl_sum_tstep_safe_cast( __ministep );
  ecl_sum_tstep_free( ministep );
}



/**
   If the ecl_kw instance is in some way invalid (i.e. wrong size);
   the function will return NULL:
*/


ecl_sum_tstep_type * ecl_sum_tstep_alloc( int ministep_nr            ,
                                          int report_step    ,
                                          const ecl_kw_type * params_kw , 
                                          const char * src_file , 
                                          const ecl_smspec_type * smspec) {

  int data_size = ecl_kw_get_size( params_kw );
  
  if (data_size == ecl_smspec_get_params_size( smspec )) {
    ecl_sum_tstep_type * ministep = util_malloc( sizeof * ministep , __func__);
    UTIL_TYPE_ID_INIT( ministep , ECL_SUM_TSTEP_ID);
    ministep->data        = ecl_kw_alloc_data_copy( params_kw );
    ministep->data_size   = data_size;
    
    ministep->report_step = report_step;
    ministep->ministep    = ministep_nr;
    ecl_smspec_set_time_info( smspec , ministep->data , &ministep->sim_days , &ministep->sim_time);

    return ministep;
  } else {
    /* 
       This is actually a fatal error / bug; the difference in smspec
       header structure should have been detected already in the
       ecl_smspec_load_restart() function and the restart case
       discarded.
    */
    fprintf(stderr , "** Warning size mismatch between timestep loaded from:%s and header:%s - timestep discarded.\n" , src_file , ecl_smspec_get_header_file( smspec ));
    return NULL;
  }
}



double ecl_sum_tstep_iget(const ecl_sum_tstep_type * ministep , int index) {
  if ((index >= 0) && (index < ministep->data_size))
    return ministep->data[index];
  else {
    util_abort("%s: param index:%d invalid: Valid range: [0,%d) \n",__func__ , index , ministep->data_size);
    return -1;
  }
}


time_t ecl_sum_tstep_get_sim_time(const ecl_sum_tstep_type * ministep) {
  return ministep->sim_time;
}


 double ecl_sum_tstep_get_sim_days(const ecl_sum_tstep_type * ministep) {
  return ministep->sim_days;
}

int ecl_sum_tstep_get_report(const ecl_sum_tstep_type * ministep) {
  return ministep->report_step;
}


int ecl_sum_tstep_get_ministep(const ecl_sum_tstep_type * ministep) {
  return ministep->ministep;
}


/*****************************************************************/

void ecl_sum_tstep_fwrite( const ecl_sum_tstep_type * ministep , fortio_type * fortio) {
  {
    ecl_kw_type * ministep_kw = ecl_kw_alloc( MINISTEP_KW , 1 , ECL_INT_TYPE );
    ecl_kw_iset_int( ministep_kw , 0 , ministep->ministep );
    ecl_kw_fwrite( ministep_kw , fortio );
    ecl_kw_free( ministep_kw );
  }

  {
    ecl_kw_type * params_kw = ecl_kw_alloc_new_shared( PARAMS_KW , ministep->data_size , ECL_FLOAT_TYPE , ministep->data);
    ecl_kw_fwrite( params_kw , fortio );
    ecl_kw_free( params_kw );
  }
  
}

