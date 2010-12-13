#ifndef __LOOKUP_TABLE_H__
#define __LOOKUP_TABLE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <double_vector.h>

typedef struct lookup_table_struct lookup_table_type;


void                 lookup_table_sort_data( lookup_table_type * lt);
void                 lookup_table_set_data( lookup_table_type * lt , double_vector_type * x , double_vector_type * y , bool data_owner );
lookup_table_type  * lookup_table_alloc( double_vector_type * x , double_vector_type * y , bool data_owner);
void                 lookup_table_append( lookup_table_type * lt , double x , double y);
void                 lookup_table_free( lookup_table_type * lt );

#ifdef __cplusplus
}
#endif
#endif
