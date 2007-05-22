#ifndef __DATE_NODE_H__
#define __DATE_NODE_H__
#include <stdbool.h>
#include <hash.h>
#include <time.h>

typedef struct date_node_struct      date_node_type;
date_node_type      * date_node_alloc(int , const char *, const hash_type *);
int                   date_node_get_date_nr(const date_node_type * );
void                  date_node_fprintf_rate_date(const date_node_type * , const char * , const char *);
void                  date_node_fwrite(const date_node_type * , FILE *);
date_node_type      * date_node_fread_alloc(int , time_t , FILE *, bool *);
void                  date_node_fprintf(const date_node_type * , FILE *, int , time_t , bool *);
void                  date_node_free__(void *);

#endif
