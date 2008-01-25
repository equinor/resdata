#ifndef __ECL_GRID_H__
#define __ECL_GRID_H__

#include <ecl_box.h>

typedef struct ecl_grid_struct ecl_grid_type;


ecl_grid_type * ecl_grid_alloc_GRDECL(int , int , int , const float *  , const float *  , const int * );
ecl_grid_type * ecl_grid_alloc_EGRID(const char * , bool);
ecl_grid_type * ecl_grid_alloc_GRID(const char * , bool);
ecl_grid_type * ecl_grid_alloc(const char * , bool);
void            ecl_grid_free(ecl_grid_type * );
int             ecl_grid_count_box_active(const ecl_grid_type * , const ecl_box_type * );
void            ecl_grid_set_box_active_list(const ecl_grid_type * , const ecl_box_type * , int * );
void            ecl_grid_get_dims(const ecl_grid_type * , int *, int * , int * , int *);
const int     * ecl_grid_alloc_index_map(const ecl_grid_type * );

#endif
