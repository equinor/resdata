#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include <work.h>
#include <timer.h>

struct timer_struct {
  char    *name;
  size_t   count;
  bool     running;
  clock_t  clock_start;
  time_t   epoch_start;
  double   sum1,sum2;
  double   min_time, max_time;
  bool     epoch_time;
};



timer_type * timer_alloc(const char *name, bool epoch_time) {
  timer_type *timer;
  timer = malloc(sizeof(timer_type));
  timer->name = calloc(strlen(name) + 1, sizeof(char));
  strcpy(timer->name , name);

  timer->epoch_time = epoch_time;
  timer_reset(timer);
  return timer;
}


void timer_free(timer_type *timer) {
  free(timer->name);
  free(timer);
}


void timer_start(timer_type *timer) {
  if (timer->running) {
    fprintf(stderr,"timer:%s already running. Use timer_stop() or timer_restart(). Aborting \n",timer->name);
    abort();
  }
  timer->running    = true;

  if (timer->epoch_time)
    time(&timer->epoch_start);
  else
    timer->clock_start = clock();
  
}


void timer_stop(timer_type *timer) {
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
    timer->sum1   += cpu_sec;
    timer->sum2   += cpu_sec * cpu_sec;
    timer->min_time = GSL_MIN_DBL(timer->min_time , cpu_sec);
    timer->max_time = GSL_MAX_DBL(timer->max_time , cpu_sec);
    timer->running = false;

  } else {
    fprintf(stderr,"Timer:%s is not running. Aborting \n",timer->name);
    abort();
  }
}


void timer_restart(timer_type *timer) {
  timer->running = false;
  timer_start(timer);
}


void timer_reset(timer_type *timer) {
  timer->count   = 0;
  timer->sum1    = 0.0;
  timer->sum2    = 0.0;
  timer->min_time =  GSL_DBL_MAX;
  timer->max_time = -GSL_DBL_MAX;
  timer->running  = false;
}


static char *pad(const char *string , const char *padchar, size_t w , char *buffer) {
  strcpy(buffer , string);
  
  while (strlen(buffer) < w) {
    strcat(buffer , padchar);
  };
  
  return buffer;
}




void timer_header_start(const timer_type *timer, FILE *stream) {
  
}


void timer_stats(const timer_type *timer , double *mean, double *std_dev) {
  *mean   = timer->sum1 / timer->count;
  *std_dev = sqrt(timer->sum2 / timer->count - (*mean) * (*mean));
}


void timer_list_report(const timer_type **timer_list , int N , FILE *stream, work_type *work) {
  double *perc_list;
  double  total_time = 0;

  size_t  max_width = 0;
  size_t  total_width;
  char *str_buffer;
  size_t i;

  work_reset(work);
  perc_list  = work_get_double(work , N , __func__);
  str_buffer = work_get_char(work , 128 , __func__);
  
  for (i=0; i < N; i++) {
    total_time += timer_list[i]->sum1;
    max_width = GSL_MAX_INT(strlen(timer_list[i]->name) , max_width);
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
    printf("%8.4f  (%6.2f %%)  %10zd          %10.8f +/- %10.8f       %10.8f %10.8f  \n",timer->sum1,perc_list[i],timer->count,mean,std_dev,timer->min_time,timer->max_time);
  }
	   
  printf("%s\n" , pad("","-",total_width,str_buffer));
  
  
}


double timer_total(const timer_type *timer) {
  return timer->sum1;
}


void timer_report(const timer_type *timer , FILE *stream) {
  if (timer->count > 0) {
    double mean,std_dev;
    timer_stats(timer , &mean , &std_dev);
    fprintf(stream,"%s: %12.6f    %10zd    %10.7f +/- %10.7f\n",timer->name,timer->sum1,timer->count,mean,std_dev);
  } else {
    fprintf(stderr,"No usage statistics collected for timer: %s  Aborting\n",timer->name);
    abort();
  }
}





