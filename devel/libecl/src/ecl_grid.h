#ifndef __ECL_GRID_H__
#define __ECL_GRID_H__
#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>
#include <ecl_box.h>


typedef struct ecl_grid_struct ecl_grid_type;

inline bool     ecl_grid_ijk_valid(const ecl_grid_type * , int  , int , int ); 
inline int      ecl_grid_get_global_index(const ecl_grid_type * , int  , int , int );
ecl_grid_type * ecl_grid_alloc_GRDECL(int , int , int , const float *  , const float *  , const int * );
ecl_grid_type * ecl_grid_alloc(const char * , bool);
void            ecl_grid_free(ecl_grid_type * );
int             ecl_grid_count_box_active(const ecl_grid_type * , const ecl_box_type * );
void            ecl_grid_set_box_active_list(const ecl_grid_type * , const ecl_box_type * , int * );
void            ecl_grid_get_dims(const ecl_grid_type * , int *, int * , int * , int *);
int             ecl_grid_get_active_index(const ecl_grid_type *  , int  , int  , int );
void            ecl_grid_summarize(const ecl_grid_type * );
void            ecl_grid_get_ijk(const ecl_grid_type * , int , int *, int * , int *);
void            ecl_grid_get_ijk_from_active_index(const ecl_grid_type *, int , int *, int * , int * );
const int     * ecl_grid_get_index_map_ref(const ecl_grid_type * );

#ifdef __cplusplus
}
#endif
#endif
