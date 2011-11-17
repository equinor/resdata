/*
   Copyright (C) 2011  Statoil ASA, Norway. 
   
   The file 'ecl_INTEHEAD.h' is part of ERT - Ensemble based Reservoir Tool. 
    
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

#ifndef __ECL_INTEHEAD_H__
#define __ECL_INTEHEAD_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <time.h>
#include <ecl_kw.h>

#define INTEHEAD_KW  "INTEHEAD"     /* Long array with lots of data. */

  typedef struct {
    int    day;
    int    year;
    int    month;
    time_t sim_time;
    int    version;         // 100, 300, 500 (Eclipse300-Thermal)
    int    phase_sum;       // Oil = 1   Gas = 2    Water = 4
    int    num_wells;
    int    niwelz;          // IWEL( niwelz , num_wells )
    int    nzwelz;          // ZWEL( nzwelz , num_wells )
    int    niconz;          // ICON( niconz , ncwmax , num_wells )     
    int    ncwmax;
  } ecl_intehead_type;


/* Some magic indices used to look up in the INTEHEAD keyword. */
#define INTEHEAD_DAY_INDEX     64   
#define INTEHEAD_MONTH_INDEX   65
#define INTEHEAD_YEAR_INDEX    66

#define INTEHEAD_VERSION_INDEX 94    /* This is ECLIPSE100 || ECLIPSE300 - not temporal version. */
#define INTEHEAD_PHASE_INDEX   14

#define INTEHEAD_NWELLS_INDEX  16
#define INTEHEAD_NIWELZ_INDEX  24
#define INTEHEAD_NZWELZ_INDEX  27
#define INTEHEAD_NICONZ_INDEX  32
#define INTEHEAD_NCWMAX_INDEX  17


  void                ecl_intehead_free( ecl_intehead_type * intehead );
  ecl_intehead_type * ecl_intehead_alloc( const ecl_kw_type * intehead_kw );
  time_t              ecl_intehead_date( const ecl_kw_type * intehead_kw );

#ifdef __cplusplus
}
#endif
#endif
