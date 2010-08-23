#ifndef __MATRIX_H__
#define __MATRIX_H__
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#ifdef __cplusplus 
extern "C" {
#endif

typedef struct matrix_struct matrix_type;

void          matrix_fscanf_data( matrix_type * matrix , bool row_major_order , FILE * stream );
void          matrix_fprintf( const matrix_type * matrix , const char * fmt , FILE * stream );
void          matrix_pretty_fprint(const matrix_type * matrix , const char * name , const char * fmt , FILE * stream);
matrix_type * matrix_alloc(int rows, int columns);
matrix_type * matrix_safe_alloc(int rows, int columns);
bool          matrix_resize(matrix_type * matrix , int rows , int columns , bool copy_content);
bool          matrix_safe_resize(matrix_type * matrix , int rows , int columns , bool copy_content);
matrix_type * matrix_alloc_copy(const matrix_type * src);
matrix_type * matrix_safe_alloc_copy(const matrix_type * src);

matrix_type * matrix_alloc_shared(const matrix_type * src , int row , int column , int rows , int columns);
void          matrix_free(matrix_type * matrix);
void          matrix_pretty_print(const matrix_type * matrix , const char * name , const char * fmt);
void          matrix_set(matrix_type * matrix, double value);
void          matrix_scale(matrix_type * matrix, double value);
void          matrix_shift(matrix_type * matrix, double value);

void          matrix_assign(matrix_type * A , const matrix_type * B);
void          matrix_inplace_add(matrix_type * A , const matrix_type * B);
void          matrix_inplace_sub(matrix_type * A , const matrix_type * B);
void          matrix_inplace_mul(matrix_type * A , const matrix_type * B);
void          matrix_inplace_div(matrix_type * A , const matrix_type * B);

void          matrix_iset_safe(matrix_type * matrix , int i , int j, double value);
void   inline matrix_iset(matrix_type * matrix , int i , int j, double value);
double inline matrix_iget(const matrix_type * matrix , int i , int j);
void   inline matrix_iadd(matrix_type * matrix , int i , int j , double value);
void   inline matrix_imul(matrix_type * matrix , int i , int j , double value);

void          matrix_inplace_matmul(matrix_type * A, const matrix_type * B);
void          matrix_inplace_matmul_mt(matrix_type * A, const matrix_type * B , int num_threads);

double        matrix_get_column_sum(const matrix_type * matrix , int column);
double        matrix_get_row_sum(const matrix_type * matrix , int column);
void          matrix_subtract_row_mean(matrix_type * matrix);
void          matrix_scale_column(matrix_type * matrix , int column  , double scale_factor);
void          matrix_set_const_column(matrix_type * matrix , const double value , int column);

double      * matrix_get_data(const matrix_type * matrix);
bool          matrix_is_finite(const matrix_type * matrix);
double        matrix_orthonormality( const matrix_type * matrix );

matrix_type * matrix_alloc_steal_data(int rows , int columns , double * data , int data_size);
void          matrix_set_column(matrix_type * matrix , const double * data , int column);
void          matrix_set_many_on_column(matrix_type * matrix , int row_offset , int elements , const double * data , int column);
void          matrix_ensure_rows(matrix_type * matrix, int rows, bool copy_content);
void          matrix_shrink_header(matrix_type * matrix , int rows , int columns);
int 	      matrix_get_rows(const matrix_type * matrix);
int 	      matrix_get_columns(const matrix_type * matrix);
int 	      matrix_get_row_stride(const matrix_type * matrix);
int 	      matrix_get_column_stride(const matrix_type * matrix);
void          matrix_get_dims(const matrix_type * matrix ,  int * rows , int * columns , int * row_stride , int * column_stride);
bool          matrix_is_quadratic(const matrix_type * matrix);
bool          matrix_equal( const matrix_type * m1 , const matrix_type * m2);

void          matrix_diag_set(matrix_type * matrix , const double * diag);
void          matrix_random_init(matrix_type * matrix);
void          matrix_matlab_dump(const matrix_type * matrix, const char * filename);

double        matrix_column_column_dot_product(const matrix_type * m1 , int col1 , const matrix_type * m2 , int col2);

#ifdef __cplusplus 
}
#endif
#endif
