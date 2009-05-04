#ifndef __ECL_GRID_H__
#define __ECL_GRID_H__
#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>
#include <ecl_box.h>
#include <double_vector.h>
#include <int_vector.h>
#include <ecl_kw.h>

typedef double (block_function_ftype) ( const double_vector_type *); 
typedef struct ecl_grid_struct ecl_grid_type;

const  char   * ecl_grid_get_filename( const ecl_grid_type * );
int             ecl_grid_get_active_index3(const ecl_grid_type * ecl_grid , int i , int j , int k);
int             ecl_grid_get_active_index1(const ecl_grid_type * ecl_grid , int global_index);
bool            ecl_grid_cell_active3(const ecl_grid_type * , int  , int  , int );
bool            ecl_grid_cell_active1(const ecl_grid_type * , int);
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
void            ecl_grid_get_pos3(const ecl_grid_type * , int , int , int , double * , double * , double *);
void            ecl_grid_get_pos1(const ecl_grid_type * grid , int global_index , double *xpos , double *ypos , double *zpos);

void            ecl_grid_compare(const ecl_grid_type * g1 , const ecl_grid_type * g2);

void            ecl_grid_alloc_blocking_variables(ecl_grid_type * , int );
void            ecl_grid_init_blocking(ecl_grid_type * );
double 		ecl_grid_block_eval2d(ecl_grid_type * grid , int i, int j , block_function_ftype * blockf );
double 		ecl_grid_block_eval3d(ecl_grid_type * grid , int i, int j , int k ,block_function_ftype * blockf );

int ecl_grid_get_block_count3d(const ecl_grid_type * ecl_grid , int i , int j, int k);
int ecl_grid_get_block_count2d(const ecl_grid_type * ecl_grid , int i , int j);

//void            ecl_grid_do_blocking(ecl_grid_type * , block_function_ftype * );
//bool            ecl_grid_blocked_cell_active_2d(const ecl_grid_type * , int , int );
//bool            ecl_grid_blocked_cell_active_3d(const ecl_grid_type * , int , int , int);
//double 		ecl_grid_get_blocked_value_2d(const ecl_grid_type * , int  , int );
//double 		ecl_grid_get_blocked_value_2d(const ecl_grid_type * , int  , int );
//double 		ecl_grid_get_blocked_value_3d(const ecl_grid_type * , int  , int  , int);
//double 		ecl_grid_get_blocked_value_3d(const ecl_grid_type * , int  , int  , int);
//
bool            ecl_grid_block_value_2d(ecl_grid_type * , double  , double  ,double );
bool            ecl_grid_block_value_3d(ecl_grid_type * , double  , double  ,double , double);

int             ecl_grid_get_region_cells(const ecl_grid_type * ecl_grid , const ecl_kw_type * region_kw , int region_value , bool active_only, bool export_active_index , int_vector_type * index_list);
int             ecl_grid_get_size( const ecl_grid_type * grid);

#ifdef __cplusplus
}
#endif
#endif
