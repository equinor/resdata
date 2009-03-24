#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <util.h>
#include <matrix.h>


typedef enum {
  DEFAULT = 10,    /* Default is currently FORTRAN layout */
  MANUAL  = 11,    /* Allocated with explicit values of stride. */
  FORTRAN = 12     /* Allocated with fortran ordering of the indices. */
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


#define GET_INDEX(m,i,j) (m->row_stride * (i) + m->column_stride * (j))

static int matrix_init_header(matrix_type * matrix , int rows , int columns , int row_stride , int column_stride, matrix_stride_enum stride_type) {
  int data_size = -1;

  if (column_stride * columns <= row_stride)                                /* Column index is running the fastest. */
    data_size = (rows * row_stride) * (columns * column_stride);
  else if (row_stride * rows <= column_stride)                              /* Row index is running fastest ~ Fortran layout. */ 
    data_size = (rows * row_stride) * (columns * column_stride);
  else
    util_abort("%s: invalid stride combination \n",__func__);

  
  matrix->rows       	= rows;
  matrix->columns    	= columns;
  matrix->row_stride 	= row_stride;
  matrix->column_stride = column_stride;
  matrix->stride_type   = stride_type;
  return data_size;
}



/*
  The freshly allocated matrix is explicitly initialized to zero.
*/
static matrix_type * matrix_alloc_with_stride(int rows , int columns , int row_stride , int column_stride, matrix_stride_enum stride_type , bool try) {
  matrix_type * matrix = util_malloc( sizeof * matrix, __func__);
  matrix->data_size     = matrix_init_header( matrix , rows , columns , row_stride , column_stride , stride_type);
  matrix->data_owner    = true;
  
  if (try) {
    /* 
       If try == true it is 'OK' to fail in the allocation,
       otherwise we use util_malloc() which will abort if the memory is not available.
    */
    matrix->data = malloc( sizeof * matrix->data * matrix->data_size );
    if (matrix->data == NULL) {
      free(matrix); 
      matrix = NULL;
    }
  } else
    matrix->data = util_malloc( sizeof * matrix->data * matrix->data_size , __func__);
  matrix_set( matrix , 0 );

  return matrix;
}



/**
   This function will allocate a matrix object where the data is
   shared with the 'src' matrix. A matrix allocated in this way can be
   used with all the matrix_xxx functions, but you should be careful
   when exporting the data pointer to e.g. Lapack routines.
*/

matrix_type * matrix_alloc_shared(const matrix_type * src , int row , int column , int rows , int columns) {
  if (((row + rows) > src->rows) || ((column + columns) > src->columns))
    util_abort("%s: Invalid matrix subsection \n",__func__);
  {
    matrix_type * matrix  = util_malloc( sizeof * matrix, __func__);
    
    matrix->data_size     = matrix_init_header( matrix , rows , columns , src->row_stride , src->column_stride , MANUAL);
    matrix->data_owner    = false;
    matrix->data          = &src->data[ GET_INDEX(src , row , column) ];
    matrix->data_owner    = false;
    
    return matrix;
  }
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

void matrix_pretty_print(const matrix_type * matrix , const char * name , const char * fmt) {
  int i,j;
  for (i=0; i < matrix->rows; i++) {

    if (i == (matrix->rows / 2))
      printf( "%s =" , name);
    else {
      int l;
      for (l = 0; l < strlen(name) + 2; l++)
	printf( " ");
    }
    
    printf(" [");
    for (j=0; j < matrix->columns; j++)
      printf(fmt , matrix_iget(matrix , i,j));
    printf("]\n");
  }
}





/*****************************************************************/
/* Functions which manipulate one element in the matrix.         */

void inline matrix_iset(matrix_type * matrix , int i , int j, double value) {
  matrix->data[ GET_INDEX(matrix , i,j) ] = value;
}


double inline matrix_iget(const matrix_type * matrix , int i , int j) {
  return matrix->data[ GET_INDEX(matrix , i, j) ];
}


void inline matrix_iadd(matrix_type * matrix , int i , int j , double value) {
  matrix->data[ GET_INDEX(matrix , i,j) ] += value;
}


void inline matrix_imul(matrix_type * matrix , int i , int j , double value) {
  matrix->data[ GET_INDEX(matrix , i,j) ] *= value;
}


/*****************************************************************/
/* One scalar operating on all the elements in the matrix         */

void matrix_set(matrix_type * matrix, double value) {
  int i,j;
  for (j=0; j < matrix->columns; j++)
    for (i=0; i < matrix->rows; i++) 
      matrix_iset(matrix , i , j , value);
}


void matrix_shift(matrix_type * matrix, double value) {
  int i,j;
  for (j=0; j < matrix->columns; j++)
    for (i=0; i < matrix->rows; i++) 
      matrix_iadd(matrix , i , j , value);
}


void matrix_scale(matrix_type * matrix, double value) {
  int i,j;
  for (j=0; j < matrix->columns; j++)
    for (i=0; i < matrix->rows; i++) 
      matrix_imul(matrix , i , j , value);
}


/*****************************************************************/
/* Matrix - matrix operations                                    */


/* Implements assignement: A = B */
void matrix_assign(matrix_type * A , const matrix_type * B) {
  if ((A->rows == B->rows) && (A->columns == B->columns)) {
    int i,j;
    
    for (j = 0; j < A->columns; j++)
      for (i=0; i < A->rows; i++)
	A->data[ GET_INDEX(A,i,j) ] = B->data[ GET_INDEX(B,i,j) ];
    
  } else 
    util_abort("%s: size mismatch \n",__func__);
}


/* Updates matrix A by adding in matrix B - elementwise. */
void matrix_inplace_add(matrix_type * A , const matrix_type * B) {
  if ((A->rows == B->rows) && (A->columns == B->columns)) {
    int i,j;
    
    for (j = 0; j < A->columns; j++)
      for (i=0; i < A->rows; i++)
	A->data[ GET_INDEX(A,i,j) ] += B->data[ GET_INDEX(B,i,j) ];
    
  } else 
    util_abort("%s: size mismatch \n",__func__);
}


/* Updates matrix A by multiplying in matrix B - elementwise - i.e. Schur product. */
void matrix_inplace_mul(matrix_type * A , const matrix_type * B) {
  if ((A->rows == B->rows) && (A->columns == B->columns)) {
    int i,j;
    
    for (j = 0; j < A->columns; j++)
      for (i=0; i < A->rows; i++)
	A->data[ GET_INDEX(A,i,j) ] *= B->data[ GET_INDEX(B,i,j) ];
    
  } else 
    util_abort("%s: size mismatch \n",__func__);
}


/* Updates matrix A by subtracting in matrix B - elementwise. */
void matrix_inplace_sub(matrix_type * A , const matrix_type * B) {
  if ((A->rows == B->rows) && (A->columns == B->columns)) {
    int i,j;
    
    for (j = 0; j < A->columns; j++)
      for (i=0; i < A->rows; i++)
	A->data[ GET_INDEX(A,i,j) ] -= B->data[ GET_INDEX(B,i,j) ];
    
  } else 
    util_abort("%s: size mismatch \n",__func__);
}


/* Updates matrix A by subtracting in matrix B - elementwise. */
void matrix_inplace_div(matrix_type * A , const matrix_type * B) {
  if ((A->rows == B->rows) && (A->columns == B->columns)) {
    int i,j;
    
    for (j = 0; j < A->columns; j++)
      for (i=0; i < A->rows; i++)
	A->data[ GET_INDEX(A,i,j) ] /= B->data[ GET_INDEX(B,i,j) ];
    
  } else 
    util_abort("%s: size mismatch \n",__func__);
}


/**
   For this function to work the following must be satisfied:

     columns in A == rows in B == columns in B 
  
   For general matrix multiplactions where A = B * C all have
   different dimensions you can use matrix_matmul().
*/


void matrix_inplace_matmul(matrix_type * A, const matrix_type * B) {
  if ((A->columns == B->rows) && (B->rows == B->columns)) {
    double * tmp = util_malloc( sizeof * A->data * A->columns , __func__);
    int i,j,k;
    
    for (i=0; i < A->rows; i++) {
      for (k=0; k < B->rows; k++)
	tmp[k] = 0;

      for (j=0; j < B->rows; j++) {
	
	double scalar_product = 0;
	for (k=0; k < A->columns; k++) 
	  scalar_product += A->data[ GET_INDEX(A,i,k) ] * B->data[ GET_INDEX(B,k,j) ];
	
	/* Assign first to tmp[k] */
	tmp[j] = scalar_product;
      }
      for (j=0; j < A->columns; j++)
	A->data[ GET_INDEX(A , i, j) ] = tmp[j];
    }
    free(tmp);
  } else
    util_abort("%s: size mismatch \n",__func__);
}



/* 
   This function does a general matrix multiply of B * C, and stores
   the result in A.
*/

void matrix_matmul(matrix_type * A, const matrix_type *B , const matrix_type * C) {
  if ((A->columns == C->columns) && (A->rows == B->rows) && (B->columns == C->rows)) {
    int i,j,k;
    for (i=0; i < A->rows; i++) {
      for (j=0; j < A->columns; j++) {
	double scalar_product = 0;
	for (k = 0; k < B->columns; k++) 
	  scalar_product += B->data[ GET_INDEX(B,i,k) ] * C->data[ GET_INDEX(C,k,j) ];
	A->data[ GET_INDEX(A , i , j) ] = scalar_product;
      }
    }
  } else
    util_abort("%s: size mismatch \n",__func__);
}
