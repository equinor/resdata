#ifndef __MATRIX_H__
#define __MATRIX_H__
#include <stdlib.h>
#include <stdio.h>

#ifdef __cplusplus 
extern "C" {
#endif

typedef struct matrix_struct matrix_type;


matrix_type * matrix_alloc(int rows, int columns);
matrix_type * matrix_safe_alloc(int rows, int columns);
bool          matrix_resize(matrix_type * matrix , int rows , int columns);
bool          matrix_safe_resize(matrix_type * matrix , int rows , int columns);
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

void   inline matrix_iset(matrix_type * matrix , int i , int j, double value);
double inline matrix_iget(const matrix_type * matrix , int i , int j);
void   inline matrix_iadd(matrix_type * matrix , int i , int j , double value);
void   inline matrix_imul(matrix_type * matrix , int i , int j , double value);
void          matrix_matmul(matrix_type * A, const matrix_type *B , const matrix_type * C);

void          matrix_inplace_matmul(matrix_type * A, const matrix_type * B);

double        matrix_get_column_sum(const matrix_type * matrix , int column);
double        matrix_get_row_sum(const matrix_type * matrix , int column);

double      * matrix_get_data(const matrix_type * matrix);


int 	 matrix_get_rows(const matrix_type * matrix);
int 	 matrix_get_columns(const matrix_type * matrix);
int 	 matrix_get_row_stride(const matrix_type * matrix);
int 	 matrix_get_column_stride(const matrix_type * matrix);
void     matrix_get_dims(const matrix_type * matrix ,  int * rows , int * columns , int * row_stride , int * column_stride);

#ifdef __cplusplus 
}
#endif
#endif
