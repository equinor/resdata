#ifndef __SHCED_KW_H__
#define __SCHED_KW_H__
#include <stdlib.h>

typedef enum {WCONHIST , UNTYPED , DATES} sched_type_enum;

typedef void  (add_line_ftype)  (void * , int , const char **);
typedef void  (free_ftype)      (void *);

typedef struct sched_kw_struct sched_kw_type;

sched_kw_type * sched_kw_alloc(const char * , sched_type_enum , bool , int *);
void            sched_kw_free(sched_kw_type * );
void            sched_kw_fprintf(const sched_kw_type *  , FILE * );

#endif
