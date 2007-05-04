#ifndef __SCHED_KW_WCONHIST_H__
#define __SCHED_KW_WCONHIST_H__


typedef struct sched_kw_wconhist_struct sched_kw_wconhist_type;


sched_kw_wconhist_type * sched_kw_wconhist_alloc();
void                     sched_kw_wconhist_free(sched_kw_wconhist_type * );
void                     sched_kw_wconhist_add_line(sched_kw_wconhist_type * kw , const char *);
void                     sched_kw_wconhist_fprintf(const sched_kw_wconhist_type * , FILE *);

#endif
