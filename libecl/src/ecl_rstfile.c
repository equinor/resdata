/*
   Copyright (C) 2011  Statoil ASA, Norway.

   The file 'ecl_rstfile.c' is part of ERT - Ensemble based Reservoir Tool.

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



/******************************************************************/
/* Query functions. */


/**
   Will look through all the INTEHEAD kw instances of the current
   ecl_file and look for @sim_time. If the value is found true is
   returned, otherwise false.
*/



bool ecl_file_has_sim_time( const ecl_file_type * ecl_file , time_t sim_time) {
  return ecl_file_view_has_sim_time( ecl_file->active_view , sim_time );
}


/*
  This function will determine the restart block corresponding to the
  world time @sim_time; if @sim_time can not be found the function
  will return 0.

  The returned index is the 'occurence number' in the restart file,
  i.e. in the (quite typical case) that not all report steps are
  present the return value will not agree with report step.

  The return value from this function can then be used to get a
  corresponding solution field directly, or the file map can
  restricted to this block.

  Direct access:

     int index = ecl_file_get_restart_index( ecl_file , sim_time );
     if (index >= 0) {
        ecl_kw_type * pressure_kw = ecl_file_iget_named_kw( ecl_file , "PRESSURE" , index );
        ....
     }


  Using block restriction:

     int index = ecl_file_get_restart_index( ecl_file , sim_time );
     if (index >= 0) {
        ecl_file_iselect_rstblock( ecl_file , index );
        {
           ecl_kw_type * pressure_kw = ecl_file_iget_named_kw( ecl_file , "PRESSURE" , 0 );
           ....
        }
     }

  Specially in the case of LGRs the block restriction should be used.
 */

int ecl_file_get_restart_index( const ecl_file_type * ecl_file , time_t sim_time) {
  int active_index = ecl_file_view_find_sim_time( ecl_file->active_view , sim_time );
  return active_index;
}


/**
   Will look through all the SEQNUM kw instances of the current
   ecl_file and look for @report_step. If the value is found true is
   returned, otherwise false.
*/

bool ecl_file_has_report_step( const ecl_file_type * ecl_file , int report_step) {
  return ecl_file_view_has_report_step( ecl_file->active_view , report_step );
}


/**
   This function will look up the INTEHEAD keyword in a ecl_file_type
   instance, and calculate simulation date from this instance.

   Will return -1 if the requested INTEHEAD keyword can not be found.
*/

time_t ecl_file_iget_restart_sim_date( const ecl_file_type * restart_file , int index ) {
  return ecl_file_view_iget_restart_sim_date( restart_file->active_view , index );
}

double ecl_file_iget_restart_sim_days( const ecl_file_type * restart_file , int index ) {
  return ecl_file_view_iget_restart_sim_days( restart_file->active_view , index );
}


/*****************************************************************/
/* Select and open functions, observe that these functions should
   only consider the global map.
*/


/*
  Will select restart block nr @seqnum_index - without considering
  report_steps or simulation time.
*/
bool ecl_file_iselect_rstblock( ecl_file_type * ecl_file , int seqnum_index ) {
  return ecl_file_select_block( ecl_file , SEQNUM_KW , seqnum_index );
}


bool ecl_file_select_rstblock_sim_time( ecl_file_type * ecl_file , time_t sim_time) {
  int seqnum_index = ecl_file_view_seqnum_index_from_sim_time( ecl_file->global_view , sim_time );

  if (seqnum_index >= 0)
    return ecl_file_iselect_rstblock( ecl_file , seqnum_index);
  else
    return false;
}


bool ecl_file_select_rstblock_report_step( ecl_file_type * ecl_file , int report_step) {
  int global_index = ecl_file_view_find_kw_value( ecl_file->global_view , SEQNUM_KW , &report_step);
  if ( global_index >= 0) {
    int seqnum_index = ecl_file_view_iget_occurence( ecl_file->global_view , global_index );
    return ecl_file_iselect_rstblock( ecl_file ,  seqnum_index);
  } else
    return false;
}

/******************************************************************/

static ecl_file_type * ecl_file_open_rstblock_report_step__( const char * filename , int report_step , int flags) {
  ecl_file_type * ecl_file = ecl_file_open( filename , flags );
  if (ecl_file) {
    if (!ecl_file_select_rstblock_report_step( ecl_file , report_step )) {
      ecl_file_close( ecl_file );
      ecl_file = NULL;
    }
  }
  return ecl_file;
}

ecl_file_type * ecl_file_open_rstblock_report_step( const char * filename , int report_step , int flags) {
  return ecl_file_open_rstblock_report_step__(filename , report_step , flags );
}


/******************************************************************/

static ecl_file_type * ecl_file_open_rstblock_sim_time__( const char * filename , time_t sim_time, int flags ) {
  ecl_file_type * ecl_file = ecl_file_open( filename , flags );
  if (ecl_file) {
    if (!ecl_file_select_rstblock_sim_time( ecl_file , sim_time)) {
      ecl_file_close( ecl_file );
      ecl_file = NULL;
    }
  }
  return ecl_file;
}

ecl_file_type * ecl_file_open_rstblock_sim_time( const char * filename , time_t sim_time, int flags) {
  return ecl_file_open_rstblock_sim_time__( filename , sim_time , flags );
}

/******************************************************************/

static ecl_file_type * ecl_file_iopen_rstblock__( const char * filename , int seqnum_index, int flags ) {
  ecl_file_type * ecl_file = ecl_file_open( filename , flags );
  if (ecl_file) {
    if (!ecl_file_iselect_rstblock( ecl_file , seqnum_index )) {
      ecl_file_close( ecl_file );
      ecl_file = NULL;
    }
  }
  return ecl_file;
}


ecl_file_type * ecl_file_iopen_rstblock( const char * filename , int seqnum_index , int flags) {
  return ecl_file_iopen_rstblock__(filename , seqnum_index , flags );
}



