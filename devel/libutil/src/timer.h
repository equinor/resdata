#ifndef __TIMER_H__
#define __TIMER_H__

#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include <work.h>

typedef struct time_struct timer_type;



timer_type * timer_alloc(const char *, bool );
void         timer_free(timer_type *);
void         timer_start(timer_type *);
void         timer_stop(timer_type *);
void         timer_restart(timer_type *);
void         timer_reset(timer_type *);
void         timer_report(const timer_type * , FILE *);
void         timer_list_report(const timer_type ** , int , FILE *, work_type *);
double       timer_total(const timer_type *);
#endif

