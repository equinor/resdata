#ifndef __ECL_GRID_H__
#define __ECL_GRID_H__

typedef struct ecl_grid_struct ecl_grid_type;


ecl_grid_type * ecl_grid_alloc_GRDECL(int , int , int , const float *  , const float *  , const int * );
ecl_grid_type * ecl_grid_alloc_EGRID(const char * , bool);
ecl_grid_type * ecl_grid_alloc_GRID(const char * , bool);
ecl_grid_type * ecl_grid_alloc(const char * , bool);
void            ecl_grid_free(ecl_grid_type * );


#endif
