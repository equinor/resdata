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

/*
  This file is included from the ecl_file.c file with a #include
  statement, i.e. it is the same compilation unit as ecl_file. The
  seperation is only to increase readability.  
*/

/*****************************************************************/
/*                   R E S T A R T   F I L E S                   */
/*****************************************************************/

/* 
   There is no special datastructure for working with restart files,
   they are mostly stock ecl_file instances with the following limited
   structure:

   * They are organized in blocks; where each block starts with a
   SEQNUM keyword, which contains the report step.

   * Each block contains an INTEHEAD keyword, immediately after the
   SEQNUM keyword, which contains the true simulation date of of
   the block, and also some other data. Observe that also INIT
   files and GRID files contain an INTEHEAD keyword.

   Here comes a couple of function which utilize this knowledge about
   the content and structure of restart files.
*/


static time_t INTEHEAD_date( const ecl_kw_type * intehead_kw ) {
  return util_make_date( ecl_kw_iget_int( intehead_kw , INTEHEAD_DAY_INDEX)   , 
                         ecl_kw_iget_int( intehead_kw , INTEHEAD_MONTH_INDEX) , 
                         ecl_kw_iget_int( intehead_kw , INTEHEAD_YEAR_INDEX)  );
}

bool ecl_file_map_has_report_step( const ecl_file_map_type * file_map , int report_step) {
  int global_index = ecl_file_map_find_kw_value( file_map , SEQNUM_KW , &report_step );
  if (global_index >= 0)
    return true;
  else
    return false;
}


static int ecl_file_map_find_sim_time(const ecl_file_map_type * file_map , time_t sim_time) {
  int global_index = -1;
  if ( ecl_file_map_has_kw( file_map , INTEHEAD_KW)) {
    const int_vector_type * intehead_index_list = hash_get( file_map->kw_index , INTEHEAD_KW );
    int index = 0;
    while (index < int_vector_size( intehead_index_list )) {
      const ecl_kw_type * intehead_kw = ecl_file_map_iget_kw( file_map , int_vector_iget( intehead_index_list , index ));
      if (INTEHEAD_date( intehead_kw ) == sim_time) {
        global_index = int_vector_iget( intehead_index_list , index );
        break;
      }
      index++;
    }
  }
  return global_index;
}

bool ecl_file_map_has_sim_time( const ecl_file_map_type * file_map , time_t sim_time) {
  int global_index = ecl_file_map_find_sim_time( file_map , sim_time );
  if (global_index >= 0)
    return true;
  else
    return false;
}

time_t ecl_file_map_iget_restart_sim_date(const ecl_file_map_type * file_map , int index) {
  if (ecl_file_map_get_num_named_kw( file_map , INTEHEAD_KW) > index) {
    ecl_kw_type * intehead_kw = ecl_file_map_iget_named_kw( file_map , INTEHEAD_KW , index);
    return INTEHEAD_date( intehead_kw );
  } else
    return -1;
}


int ecl_file_map_get_restart_index( const ecl_file_map_type * file_map , time_t sim_time) {
  int num_INTEHEAD = ecl_file_map_get_num_named_kw( file_map , INTEHEAD_KW );
  if (num_INTEHEAD == 0)
    return -1;       /* We have no INTEHEAD headers - probably not a restart file at all. */
  else {
    /*
      Should probably do something smarter than a linear search; but I dare not
      take the chance that all INTEHEAD headers are properly set. This is from
      Schlumberger after all.
    */
    int index = 0;
    while (true) {
      time_t itime = ecl_file_map_iget_restart_sim_date( file_map , index );
      
      if (itime == sim_time) /* Perfect hit. */
        return index;

      if (itime > sim_time)  /* We have gone past the target_time - i.e. we do not have it. */
        return -1;
      
      index++;
      if (index == num_INTEHEAD)  /* We have iterated through the whole thing without finding sim_time. */
        return -1;
    }
  }
}

/**
   This function will look up the INTEHEAD keyword in a ecl_file_type
   instance, and calculate simulation date from this instance.

   Will return -1 if the requested INTEHEAD keyword can not be found.
*/

time_t ecl_file_iget_restart_sim_date( const ecl_file_type * restart_file , int index ) {
  return ecl_file_map_iget_restart_sim_date( restart_file->active_map , index );
}



/**
   This function will scan through the ecl_file looking for INTEHEAD
   headers corresponding to sim_time. If sim_time is found the
   function will return the INTEHEAD occurence number, i.e. for a
   unified restart file like:

   INTEHEAD  /  01.01.2000
   ...
   PRESSURE
   SWAT
   ...
   INTEHEAD  /  01.03.2000
   ...
   PRESSURE
   SWAT
   ...
   INTEHEAD  /  01.05.2000
   ...
   PRESSURE
   SWAT
   ....

   The function call:

   ecl_file_get_restart_index( restart_file , (time_t) "01.03.2000")

   will return 1. Observe that this will in general NOT agree with the
   DATES step number.

   If the sim_time can not be found the function will return -1, that
   includes the case when the file in question is not a restart file
   at all, and no INTEHEAD headers can be found.
   
   Observe that the function requires on-the-second-equality; which is
   of course quite strict.
*/


int ecl_file_get_restart_index( const ecl_file_type * restart_file , time_t sim_time) {
  return ecl_file_map_get_restart_index( restart_file->active_map , sim_time );
}


/**
   Will look through all the INTEHEAD kw instances of the current
   ecl_file and look for @sim_time. If the value is found true is
   returned, otherwise false.
*/



bool ecl_file_has_sim_time( const ecl_file_type * ecl_file , time_t sim_time) {
  return ecl_file_map_has_sim_time( ecl_file->active_map , sim_time );
}


/**
   Will look through all the SEQNUM kw instances of the current
   ecl_file and look for @report_step. If the value is found true is
   returned, otherwise false.
*/

bool ecl_file_has_report_step( const ecl_file_type * ecl_file , int report_step) {
  return ecl_file_map_has_report_step( ecl_file->active_map , report_step );
}



ecl_file_map_type * ecl_file_iget_unrstmap( ecl_file_type * ecl_file , int index) {
  return ecl_file_get_blockmap( ecl_file , SEQNUM_KW , index );
}


ecl_file_map_type * ecl_file_get_unrstmap_time_t( ecl_file_type * ecl_file , time_t sim_time) {
  int global_index = ecl_file_map_get_restart_index( ecl_file->active_map , sim_time );
  if (global_index >= 0)
    return ecl_file_get_blockmap( ecl_file , SEQNUM_KW , global_index);
  else
    return NULL;
}


ecl_file_map_type * ecl_file_get_unrstmap_report_step( ecl_file_type * ecl_file , int report_step) {
  int global_index = ecl_file_map_find_kw_value( ecl_file->active_map , SEQNUM_KW , &report_step);
  if ( global_index >= 0) {
    int seqnum_index = ecl_file_map_iget_occurence( ecl_file->active_map , global_index );
    return ecl_file_get_blockmap( ecl_file , SEQNUM_KW , seqnum_index );
  } else 
    return NULL;
}


/**
   The SEQNUM number found in unified restart files corresponds to the 
   REPORT_STEP.
*/




