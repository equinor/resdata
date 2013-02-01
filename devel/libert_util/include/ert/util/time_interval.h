/*
   Copyright (C) 2013  Statoil ASA, Norway. 
    
   The file 'time_interval.h' is part of ERT - Ensemble based Reservoir Tool. 
    
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

#ifndef __TIME_INTERVAL_H__
#define __TIME_INTERVAL_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <time.h>

  typedef struct time_interval_struct time_interval_type;

  time_interval_type * time_interval_alloc( time_t start_time , time_t end_time );
  void                 time_interval_free( time_interval_type * ti );
  bool                 time_interval_is_empty( time_interval_type * ti );
  bool                 time_interval_update( time_interval_type * ti , time_t start_time , time_t end_time);
  bool                 time_interval_contains( const time_interval_type * ti , time_t t);
  
#ifdef __cplusplus
}
#endif
#endif 
