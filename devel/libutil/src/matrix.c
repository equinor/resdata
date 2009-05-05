#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <util.h>
#include <matrix.h>


/**
   This is V E R Y  S I M P L E matrix implementation. It is not
   designed to be fast/efficient or anything. It is purely a minor
   support functionality for the enkf program, and should N O T be
   considered as general matrix functionality.
*/



/**
  Many of matrix functions can potentially involve laaarge amounts of
  memory. The functions:

   o matrix_alloc(), matrix_resize() and matrix_alloc_copy() will
     abort with util_abort() if the memory requirements can not be
     satisfied.

   o The corresponding functions matrix_safe_alloc(),
     matrix_safe_resize() and matrix_safe_alloc_copy() will not abort,
     instead NULL or an unchanged matrix will be returned. When using
     these functons it is the responsability of the calling scope to
     check return values.

  So the expression "safe" should be interpreted as 'can not abort' -
  however the responsability of the calling scope is greater when it
  comes to using these functions - things can surely blow up!
*/



struct matrix_struct {
  double 		* data;           /* The actual storage */
  bool   		  data_owner;     /* is this matrix instance the owner of data? */
  size_t    		  data_size;      /* What is the length of data (number of double values). */

  int    		  rows;           /* The number of rows in the matrix. */
  int    		  columns;        /* The number of columns in the matrix. */
  int    		  row_stride;     /* The distance in data between two conscutive row values. */
  int    		  column_stride;  /* The distance in data between to consecutive column values. */
  
  /* 
     Observe that the stride is considered an internal property - if
     the matrix is stored to disk and then recovered the strides might
     change, and alos matrix_alloc_copy() will not respect strides.
  */
};


/*#define GET_INDEX(m,i,j) (m->row_stride * (i) + m->column_stride * (j))*/

static inline int GET_INDEX( const matrix_type * m , int i , int j) {
  return m->row_stride *i + m->column_stride *j;
}




static void matrix_init_header(matrix_type * matrix , int rows , int columns , int row_stride , int column_stride) {

  if (!((column_stride * columns <= row_stride) || (row_stride * rows <= column_stride)))
    util_abort("%s: invalid stride combination \n",__func__);
  
  matrix->data_size     = (rows * row_stride) * (columns * column_stride);
  matrix->rows       	= rows;
  matrix->columns    	= columns;
  matrix->row_stride 	= row_stride;
  matrix->column_stride = column_stride;
}


/**
   This is the low-level function allocating storage. If the input
   flag 'safe_mode' is equal to true, the function will return NULL if the
   allocation fails, otherwise the function will abort() if the
   allocation fails.

   Before returning all elements will be set to zero.
*/

static double * __alloc_data( int data_size , bool safe_mode ) {
  double * data;
  if (safe_mode) {
    /* 
       If safe_mode == true it is 'OK' to fail in the allocation,
       otherwise we use util_malloc() which will abort if the memory is not available.
    */
    data = malloc( sizeof * data * data_size );
  } else
    data = util_malloc( sizeof * data * data_size , __func__);

  if (data != NULL) 
    for (int i = 0; i < data_size; i++)
      data[i] = 0;

  return data;
}


/**
   Corresponding to __alloc_data() - but based on reallocation of data.
*/
static bool __realloc_data( double ** _data , int data_size , bool safe_mode ) {
  double * data = *_data;
  double * tmp;
  if (safe_mode) {
    /* 
       If safe_mode == true it is 'OK' to fail in the allocation,
       otherwise we use util_malloc() which will abort if the memory is not available.
    */
    tmp = realloc( data , sizeof * data * data_size );
  } else
    tmp = util_realloc( data , sizeof * data * data_size , __func__);
  
  if (tmp != NULL) {
    /* realloc() succeeded */
    data = tmp;
    /* 
       We initialize to zero, because the differnet matrices involved
       might have different data-layout, so the realloc() will
       (probably) not have preserved matrix ordered data anyway; that
       is handled in the calling scope.
    */
    for (int i = 0; i < data_size; i++)
      data[i] = 0;
  
    *_data = data;
    return true;
  } else 
    return false;

}


/*
  The freshly allocated matrix is explicitly initialized to zero. If
  the variable safe_mode equals true the function will return NULL if the
  allocation of data fails, otherwise it will abort() if the
  allocation fails.
*/
static matrix_type * matrix_alloc_with_stride(int rows , int columns , int row_stride , int column_stride, bool safe_mode) {
  matrix_type * matrix  = util_malloc( sizeof * matrix, __func__);

  matrix_init_header( matrix , rows , columns , row_stride , column_stride);
  matrix->data_owner    = true;
  matrix->data          = __alloc_data( matrix->data_size , safe_mode );
  if (safe_mode) {
    if (matrix->data == NULL) {  
      /* Allocation failed - we return NULL */
      matrix_free(matrix);
      matrix = NULL;
    }
  }
  
  return matrix;
}



/**
   This function will allocate a matrix object where the data is
   shared with the 'src' matrix. A matrix allocated in this way can be
   used with all the matrix_xxx functions, but you should be careful
   when exporting the data pointer to e.g. lapack routines.
*/

matrix_type * matrix_alloc_shared(const matrix_type * src , int row , int column , int rows , int columns) {
  if (((row + rows) > src->rows) || ((column + columns) > src->columns))
    util_abort("%s: Invalid matrix subsection \n",__func__);
  {
    matrix_type * matrix  = util_malloc( sizeof * matrix, __func__);
    
    matrix_init_header( matrix , rows , columns , src->row_stride , src->column_stride);
    matrix->data_owner    = false;
    matrix->data          = &src->data[ GET_INDEX(src , row , column) ];
    matrix->data_owner    = false;
    
    return matrix;
  }
}



/*****************************************************************/

static matrix_type * matrix_alloc__(int rows, int columns , bool safe_mode) {
  return matrix_alloc_with_stride( rows , columns , 1 , rows , safe_mode );    /* Must be the stride (1,rows) to use the lapack routines. */
}

matrix_type * matrix_alloc(int rows, int columns) {
  return matrix_alloc__( rows , columns , false );
}

matrix_type * matrix_safe_alloc(int rows, int columns) {
  return matrix_alloc__( rows , columns , true );
}

/*****************************************************************/

/**
   Will not respect strides - that is considered low level data
   layout.
*/
static matrix_type * matrix_alloc_copy__( const matrix_type * src , bool safe_mode) {
  matrix_type * copy = matrix_alloc__( matrix_get_rows( src ), matrix_get_columns( src ) , safe_mode);
  if (copy != NULL) 
    matrix_assign(copy , src);
  return copy;
}


matrix_type * matrix_alloc_copy(const matrix_type * src) {
  return matrix_alloc_copy__(src , false );
}

/**
   Will return NULL if allocation of the copy failed. 
*/

matrix_type * matrix_safe_alloc_copy(const matrix_type * src) {
  return matrix_alloc_copy__(src , true);
}

/*****************************************************************/

static bool matrix_resize__(matrix_type * matrix , int rows , int columns , bool safe_mode) {
  if (!matrix->data_owner)
    util_abort("%s: sorry - can not resize shared matrizes. \n",__func__);
  {
    bool resize_OK           = true;
    int copy_rows    	     = util_int_min( rows    , matrix->rows );
    int copy_columns 	     = util_int_min( columns , matrix->columns);
    matrix_type * copy_view  = matrix_alloc_shared( matrix , 0 , 0 , copy_rows , copy_columns);         /* This is the part of the old matrix which should be copied over to the new. */
    matrix_type * copy       = matrix_alloc_copy__( copy_view , safe_mode );                            /* Now copy contains the part of the old matrix which should be copied over - with private storage. */

    {
      int old_rows , old_columns, old_row_stride , old_column_stride;
      matrix_get_dims( matrix , &old_rows , &old_columns , &old_row_stride , &old_column_stride);        /* Storing the old header information - in case the realloc() fails. */
      
      matrix_init_header(matrix , rows , columns , 1 , rows);               /* Resetting the header for the matrix */
      if (__realloc_data(&matrix->data , matrix->data_size , safe_mode)) {  /* Realloc succeeded */
	matrix_type * target_view = matrix_alloc_shared(matrix , 0 , 0 , copy_rows , copy_columns);
	matrix_assign( target_view , copy);
	matrix_free( target_view );
      } else {                                                              /* Failed to realloc new storage - recovering the old matrix, and returning false. */
	matrix_init_header(matrix , old_rows , old_columns , old_row_stride , old_column_stride);
	resize_OK = false;
      }
    }

    matrix_free(copy_view);
    matrix_free(copy);
    
    return resize_OK;
  }
}


/** 
    Will alwasy return true (or abort). 
*/
bool matrix_resize(matrix_type * matrix , int rows , int columns ) {
  return matrix_resize__(matrix , rows , columns , false);
}

/**
   Return true if the resize succeded, otherwise it will return false
   and leave the matrix unchanged. When resize implies expanding a
   dimension, the newly created elements will be explicitly
   initialized to zero.
*/

bool matrix_safe_resize(matrix_type * matrix , int rows , int columns ) {
  return matrix_resize__(matrix , rows , columns , true);
}

/*****************************************************************/

void matrix_free(matrix_type * matrix) {
  if (matrix->data_owner)
    util_safe_free(matrix->data);
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



/* Discard the strides?? */
void matrix_fwrite(const matrix_type * matrix , FILE * stream) {
  
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


/* Updates matrix A by subtracting matrix B - elementwise. */
void matrix_inplace_sub(matrix_type * A , const matrix_type * B) {
  if ((A->rows == B->rows) && (A->columns == B->columns)) {
    int i,j;
    
    for (j = 0; j < A->columns; j++)
      for (i=0; i < A->rows; i++)
	A->data[ GET_INDEX(A,i,j) ] -= B->data[ GET_INDEX(B,i,j) ];
    
  } else 
    util_abort("%s: size mismatch \n",__func__);
}


/* Updates matrix A by dividing matrix B - elementwise. */
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
   different dimensions you can use matrix_matmul() (which calls the
   BLAS routine dgemm());
*/


void matrix_inplace_matmul(matrix_type * A, const matrix_type * B) {
  if ((A->columns == B->rows) && (B->rows == B->columns)) {
    double * tmp = util_malloc( sizeof * A->data * A->columns , __func__);
    int i,j,k;
    
    for (i=0; i < A->rows; i++) {
      
      /* Clearing the tmp vector */
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





/*****************************************************************/
/* Row/column functions                                          */

double matrix_get_row_sum(const matrix_type * matrix , int row) {
  double sum = 0;
  int j;
  for (j=0; j < matrix->columns; j++)
    sum += matrix->data[ GET_INDEX( matrix , row , j ) ];
  return sum;
}


double matrix_get_column_sum(const matrix_type * matrix , int column) {
  double sum = 0;
  int i;
  for (i=0; i < matrix->rows; i++)
    sum += matrix->data[ GET_INDEX( matrix , i , column ) ];
  return sum;
}


/*****************************************************************/
/** 
   This function will return the double data pointer of the matrix,
   when you use this explicitly you ARE ON YOUR OWN. 
*/

double * matrix_get_data(const matrix_type * matrix) {
  return matrix->data;
}

/**
   The query functions below can be used to ask for the dimensions &
   strides of the matrix.
*/

int matrix_get_rows(const matrix_type * matrix) {
  return matrix->rows;
}

int matrix_get_columns(const matrix_type * matrix) {
  return matrix->columns;
}

int matrix_get_row_stride(const matrix_type * matrix) {
  return matrix->row_stride;
}

int matrix_get_column_stride(const matrix_type * matrix) {
  return matrix->column_stride;
}


void matrix_get_dims(const matrix_type * matrix ,  int * rows , int * columns , int * row_stride , int * column_stride) {

  *rows       	 = matrix->rows;
  *columns    	 = matrix->columns;
  *row_stride 	 = matrix->row_stride;
  *column_stride = matrix->column_stride;
  
}


/*****************************************************************/
/* Various special matrices */


/** 
    Will set the diagonal elements in matrix to the values in diag,
    and all remaining elements to zero. Assumes that matrix is
    rectangular.
*/

void matrix_diag_set(matrix_type * matrix , const double * diag) {
  if (matrix->rows == matrix->columns) {
    matrix_set(matrix , 0);
    for (int i=0; i < matrix->rows; i++)
      matrix->data[ GET_INDEX(matrix , i , i) ] = diag[i];
  } else
    util_abort("%s: size mismatch \n",__func__);
}



/**
   Fills the matrix with uniformly distributed random numbers;
   samepled with the standard built in rand() function.
*/

void matrix_random_init(matrix_type * matrix) {
  int i,j;
  for (j=0; j < matrix->columns; j++)
    for (i=0; i < matrix->rows; i++)
      matrix->data[ GET_INDEX(matrix , i , j) ] = 1.0 * rand() / RAND_MAX;
}




/**
   This function dumps the following binary file:

   rows
   columns
   data(1,1)
   data(2,1)
   data(3,1)
   ....
   data(1,2)
   data(2,2)
   ....

   Not exactly a matlab format.

   The following matlab code can be used to instatiate a matrix based
   on the file:

     function m = load_matrix(filename)
     fid  = fopen(filename);
     dims = fread(fid , 2 , 'int32');
     m    = fread(fid , [dims(1) , dims(2)] , 'double');
     fclose(fid);
   
   >> A = load_matrix( 'filename' ); 
*/


void matrix_matlab_dump(const matrix_type * matrix, const char * filename) {
  FILE * stream = util_fopen( filename , "w");
  int i,j;
  util_fwrite_int( matrix->rows    , stream);
  util_fwrite_int( matrix->columns , stream);

  for (j=0; j < matrix->columns; j++)
    for (i=0; i < matrix->rows; i++)
      util_fwrite_double( matrix->data[ GET_INDEX(matrix , i , j) ] , stream);

  fclose(stream);
}
