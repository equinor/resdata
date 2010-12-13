#include <double_vector.h>
#include <stdbool.h>
#include <util.h>
#include <lookup_table.h>

struct lookup_table_struct {
  bool                 data_owner; 
  double_vector_type * x;
  double_vector_type * y;
  int                  size; 
  double               xmin;
  double               xmax;
  bool                 prev_index;
};



void lookup_table_sort_data( lookup_table_type * lt) {
  if (double_vector_get_read_only( lt->x ))
    if (!double_vector_is_sorted( lt->x , false))
      util_abort("%s: x vector is not sorted and read-only - this will not fly\n",__func__);
  
  {
    int * sort_perm = double_vector_alloc_sort_perm( lt->x );
    double_vector_permute( lt->x , sort_perm );
    double_vector_permute( lt->y , sort_perm );
    free( sort_perm );
  }
  
  lt->xmin = double_vector_get_first( lt->x );
  lt->xmax = double_vector_get_last( lt->x );
  lt->size = double_vector_size( lt-> x);
  lt->prev_index = -1;
}


/**
   IFF the @read_only flag is set to true; the x vector MUST be
   sorted.  
*/

void lookup_table_set_data( lookup_table_type * lt , double_vector_type * x , double_vector_type * y , bool data_owner ) {
  
  if (lt->data_owner) {
    double_vector_free( lt->x );
    double_vector_free( lt->y );
  }

  lt->x = x;
  lt->y = y;
  lt->data_owner = data_owner;
  lookup_table_sort_data( lt );
}



lookup_table_type * lookup_table_alloc( double_vector_type * x , double_vector_type * y , bool data_owner) {
  lookup_table_type * lt = util_malloc( sizeof * lt , __func__ );

  if ((x == NULL) && (y == NULL)) {
    x = double_vector_alloc(0 , 0);
    y = double_vector_alloc(0 , 0);
    data_owner = true;
  } 
  lookup_table_set_data( lt , x , y , false );
  lt->data_owner = data_owner;
  
  return lt;
}


void lookup_table_append( lookup_table_type * lt , double x , double y) {
  double_vector_append( lt->x , x );
  double_vector_append( lt->y , y );
}


void lookup_table_free( lookup_table_type * lt ) {
  if (lt->data_owner) {
    double_vector_free( lt->x );
    double_vector_free( lt->y );
  } 
  free( lt );
}



