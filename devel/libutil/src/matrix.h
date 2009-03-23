#ifndef __MATRIX_H__
#define __MATRIX_H__
#include <stdlib.h>
#include <stdio.h>


typedef struct matrix_struct matrix_type;


matrix_type * matrix_alloc(int rows, int columns);
void          matrix_free(matrix_type * matrix);
void          matrix_fprintf(const matrix_type * matrix , const char * fmt, FILE * stream);
void   inline matrix_set(matrix_type * matrix , int i , int j, double value);
double inline matrix_get(const matrix_type * matrix , int i , int j);
void          matrix_set_all(matrix_type * matrix, double value);

#endif
