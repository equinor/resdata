/*
   Copyright (C) 2013  Statoil ASA, Norway. 
    
   The file 'time_interval.c' is part of ERT - Ensemble based Reservoir Tool. 
    
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
#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/util.h>
#include <ert/util/time_interval.h>

#define TIME_INTERVAL_EMPTY -1


struct time_interval_struct {
  bool   empty;
  time_t start_time;
  time_t end_time;
};


bool time_interval_update( time_interval_type * ti , time_t start_time , time_t end_time) {
  if (start_time < end_time) {
    ti->start_time = start_time;
    ti->end_time   = end_time;
    ti->empty = false;
  } else {
    ti->start_time = TIME_INTERVAL_EMPTY;
    ti->end_time   = TIME_INTERVAL_EMPTY;
    ti->empty = true;
  }
  return !ti->empty;
}


time_interval_type * time_interval_alloc( time_t start_time , time_t end_time ) {
  time_interval_type * ti = util_malloc( sizeof * ti );
  time_interval_update( ti , start_time , end_time );
  return ti;
}


void time_interval_free( time_interval_type * ti ) {
  free( ti );
}
  

bool time_interval_is_empty( time_interval_type * ti ) {
  return ti->empty;
}


bool time_interval_contains( const time_interval_type * ti , time_t t) {
  if (ti->empty)
    return false;
  else {
    if (t < ti->start_time)
      return false;
    else if (t >= ti->end_time)
      return false;
    else
      return true;
  }
}
