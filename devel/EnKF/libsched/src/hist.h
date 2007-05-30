#ifndef __HIST_H__
#define __HIST_H__
#include <time.h>
#include <stdbool.h>
#include "date_node.h"
#include "rate_node.h"

typedef struct hist_struct hist_type;



hist_type * hist_alloc(time_t);
void   hist_add_date(hist_type *  , date_node_type * );
void   hist_add_rate(hist_type *  , int , rate_type * );

double hist_get_ORAT(const hist_type * , int , const char * );
double hist_get_WRAT(const hist_type * , int , const char * );
double hist_get_GRAT(const hist_type * , int , const char * );
double hist_get_GOR(const hist_type * , int , const char * , bool *);
double hist_get_WCT(const hist_type * , int , const char * , bool *);



#endif
