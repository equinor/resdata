#include <stdbool.h>
#include <stdlib.h>
#include <util.h>
#include <matrix.h>


typedef enum {
  DEFAULT = 10,
  MANUAL  = 11,
  FORTRAN = 12
} matrix_stride_enum ;



struct matrix_struct {
  double 		* data;           /* The actual storage */
  bool   		  data_owner;     /* is this matrix instance the owner of data? */
  int    		  data_size;      /* What is the length of data (number of double values). */

  int    		  rows;           /* The number of rows in the matrix. */
  int    		  columns;        /* The number of columns in the matrix. */
  int    		  row_stride;     /* The distance in data between two conscutive row values. */
  int    		  column_stride;  /* The distance in data between to consecutive column values. */

  matrix_stride_enum      stride_type;    /* What type of stride: {DEFAULT, MANUAL, FORTRAN} */
};






static matrix_type * matrix_alloc_with_stride(int rows , int columns , int row_stride , int column_stride, matrix_stride_enum stride_type , bool try) {
  matrix_type * matrix = util_malloc( sizeof * matrix, __func__);

  if (row_stride < column_stride) {
    if (column_stride < rows * row_stride)
      util_abort("%s: impossible stride combination\n",__func__);
    matrix->data_size = (rows * row_stride) * (columns * column_stride);
  } else {
    if (row_stride < column_stride) {
      if (row_stride < columns * column_stride)
	util_abort("%s: impossible stride combination \n",__func__);
    }
  }

  matrix->rows       	= rows;
  matrix->columns    	= columns;
  matrix->row_stride 	= row_stride;
  matrix->column_stride = column_stride;
  matrix->stride_type   = stride_type;
  matrix->data_owner    = true;
  if (try) {
    matrix->data = malloc( sizeof * matrix->data * matrix->data_size );
    if (matrix->data == NULL) {
      free(matrix); 
      matrix = NULL;
    }
  } else
    matrix->data = util_malloc( sizeof * matrix->data * matrix->data_size , __func__);
  
  return matrix;
}




matrix_type * matrix_alloc(int rows, int columns) {
  return matrix_alloc_with_stride( rows , columns , 1 , rows , DEFAULT , false);
}






void matrix_free(matrix_type * matrix) {
  if (matrix->data_owner)
    free(matrix->data);
  free(matrix);
}


/*****************************************************************/

void matrix_fprintf(const matrix_type * matrix , const char * fmt, FILE * stream) {
  int i,j;
  for (i=0; i < matrix->rows; i++) {
    fprintf(stream , "   ");
    for (j=0; j < matrix->columns; j++)
      fprintf(stream , fmt , matrix_get(matrix , i,j));
    fprintf(stream , "\n");
  }
}


void inline matrix_set(matrix_type * matrix , int i , int j, double value) {
  matrix->data[i * matrix->row_stride + j * matrix->column_stride] = value;
}


double inline matrix_get(const matrix_type * matrix , int i , int j) {
  return matrix->data[i * matrix->row_stride + j * matrix->column_stride];
}


void matrix_set_all(matrix_type * matrix, double value) {
  int i,j;
  for (j=0; j < matrix->columns; j++)
    for (i=0; i < matrix->rows; i++) 
      matrix_set(matrix , i , j , value);
      
}
