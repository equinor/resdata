/*
   Copyright (C) 2011  Statoil ASA, Norway. 
    
   The file 'timer.c' is part of ERT - Ensemble based Reservoir Tool. 
    
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
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include <util.h>
#include <timer.h>

struct timer_struct {
  char    *name;
  size_t   count;

  clock_t  clock_start;
  time_t   epoch_start;
  double   sum1 , sum2;
  double   min_time , max_time;
  bool     running , epoch_time;
};



timer_type * timer_alloc(const char *name, bool epoch_time) {
  timer_type *timer;
  timer       = util_malloc(sizeof * timer , __func__);
  timer->name = util_alloc_string_copy( name );
  
  timer->epoch_time = epoch_time;
  timer_reset(timer);
  return timer;
}


void timer_free(timer_type *timer) {
  free(timer->name);
  free(timer);
}


void timer_start(timer_type *timer) {
  if (timer->running) 
    util_abort("%s: Timer:%s already running. Use timer_stop() or timer_restart(). Aborting \n",__func__ , timer->name);
  timer->running    = true;

  if (timer->epoch_time)
    time(&timer->epoch_start);
  else
    timer->clock_start = clock();
  
}


double timer_stop(timer_type *timer) {
  time_t  epoch_time;
  clock_t clock_time = clock();
  
  time(&epoch_time);
  if (timer->running) {
    double cpu_sec;
    if (timer->epoch_time)
      cpu_sec = 1.0 * (epoch_time - timer->epoch_start);
    else
      cpu_sec = 1.0 * (clock_time - timer->clock_start) / CLOCKS_PER_SEC;
    
    timer->count++;
    timer->sum1    += cpu_sec;
    timer->sum2    += cpu_sec * cpu_sec;
    timer->min_time = util_double_min( timer->min_time , cpu_sec);
    timer->max_time = util_double_max( timer->max_time , cpu_sec);
    timer->running  = false;

    return cpu_sec;
  } else 
    util_abort("%s: Timer:%s is not running. Aborting \n",__func__ , timer->name);
  
  return -1;
}



void timer_reset(timer_type *timer) {
  timer->count    = 0;
  timer->sum1     = 0.0;
  timer->sum2     = 0.0;
  timer->min_time =  99999999;
  timer->max_time = -99999999;
  timer->running  = false;
}


static char *pad(const char *string , const char *padchar, size_t w , char *buffer) {
  strcpy(buffer , string);
  
  while (strlen(buffer) < w) {
    strcat(buffer , padchar);
  };
  
  return buffer;
}




void timer_stats(const timer_type *timer , double *mean, double *std_dev) {
  *mean    = timer->sum1 / timer->count;
  *std_dev = sqrt(timer->sum2 / timer->count - (*mean) * (*mean));
}




void timer_list_report(const timer_type **timer_list , int N , FILE *stream) {
  double *perc_list;
  double  total_time = 0;

  size_t  max_width = 0;
  size_t  total_width;
  char *str_buffer;
  size_t i;

  perc_list  = util_malloc( N * sizeof * perc_list , __func__);
  str_buffer = util_malloc( 128 , __func__);
  
  for (i=0; i < N; i++) {
    total_time += timer_list[i]->sum1;
    max_width = util_int_max(strlen(timer_list[i]->name) , max_width);
  }
  max_width += 3;
  total_width = max_width + 102;

  if (total_time > 0.0) {
    for (i=0; i < N; i++)
      perc_list[i] = timer_list[i]->sum1 * 100 / total_time;
  } else {
    for (i=0; i < N; i++)
      perc_list[i] = -1;
  }
  

  printf("%s" , pad("Timer name"," ",max_width,str_buffer));
  printf("        Total time [sec]          Count           Avg. time [sec]        Min time [sec]  Max time[sec] \n");
  printf("%s\n" , pad("","-",total_width,str_buffer));
  for (i=0; i < N; i++) {
    const timer_type *timer = timer_list[i];
    double mean,std_dev;
    printf("%s:   ", pad(timer->name,".", max_width,str_buffer));
    timer_stats(timer , &mean , &std_dev);
    printf("%8.4f  (%6.2f %%)  %10d          %10.8f +/- %10.8f       %10.8f %10.8f  \n",
           timer->sum1,
           perc_list[i],
           timer->count,
           mean,
           std_dev,
           timer->min_time,
           timer->max_time);
  }
           
  printf("%s\n" , pad("","-",total_width,str_buffer));
  
  free( perc_list );
  free( str_buffer );
}



double timer_get_total_time(const timer_type *timer) {
  return timer->sum1;
}


double timer_get_max_time(const timer_type *timer) {
  return timer->max_time;
}


double timer_get_min_time(const timer_type *timer) {
  return timer->min_time;
}

double timer_get_avg_time(const timer_type *timer) {
  return timer->sum1 / timer->count;
}



void timer_report(const timer_type *timer , FILE *stream) {
  if (timer->count > 0) {
    double mean,std_dev;
    timer_stats(timer , &mean , &std_dev);
    fprintf(stream,"%s: %12.6f    %10d    %10.7f +/- %10.7f\n",timer->name,timer->sum1,timer->count,mean,std_dev);
  } else {
    fprintf(stderr,"No usage statistics collected for timer: %s  Aborting\n",timer->name);
    abort();
  }
}





