#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <util.h>
#include <matrix.h>
#include <thread_pool.h>
#include <arg_pack.h>

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

static inline size_t MATRIX_DATA_SIZE( const matrix_type * m) {
  return m->columns * m->column_stride;
}




static void matrix_init_header(matrix_type * matrix , int rows , int columns , int row_stride , int column_stride) {

  if (!((column_stride * columns <= row_stride) || (row_stride * rows <= column_stride)))
    util_abort("%s: invalid stride combination \n",__func__);
  
  matrix->data_size     = 0;
  matrix->rows       	= rows;
  matrix->columns    	= columns;
  matrix->row_stride 	= row_stride;
  matrix->column_stride = column_stride;
}


/**
   This is the low-level function allocating storage. If the input
   flag 'safe_mode' is equal to true, the function will return NULL if
   the allocation fails, otherwise the function will abort() if the
   allocation fails.

   Before returning all elements will be initialized to zero.

   1. It is based on first free() of the original pointer, and then
       subsequently calling malloc() to get new storage. This is to
       avoid prohibitive temporary memory requirements during the
       realloc() call.
       
    2. If the malloc() fails the function will return NULL, i.e. you
       will NOT keep the original data pointer. I.e. in this case the
       matrix will be invalid. It is the responsability of the calling
       scope to do the right thing.

    3. realloc() functionality - i.e. keeping the original content of
       the matrix is implemented at higher level. The memory layout of
       the matrix will in general change anyway; so the promise made
       by realloc() is not very interesting.
*/

static void matrix_realloc_data__( matrix_type * matrix , bool safe_mode ) {
  if (matrix->data_owner) {
    size_t data_size = MATRIX_DATA_SIZE( matrix );
    if (matrix->data_size == data_size) return;
    if (matrix->data != NULL)
      free(matrix->data);
    
    if (safe_mode) {
      /* 
	 If safe_mode == true it is 'OK' to fail in the allocation,
	 otherwise we use util_malloc() which will abort if the memory
	 is not available.
      */
      matrix->data = malloc( sizeof * matrix->data * data_size );
    } else
      matrix->data = util_malloc( sizeof * matrix->data * data_size , __func__);
    

    /* Initializing matrix content to zero. */
    if (matrix->data != NULL) {
      for (int i = 0; i < data_size; i++)
	matrix->data[i] = 0;
    } else 
      data_size = 0;

    /**
       Observe that if the allocation failed the matrix will
       be returned with data == NULL, and data_size == 0.
    */
    matrix->data_size = data_size;
  } else 
    util_abort("%s: can not manipulate memory when is not data owner\n",__func__);
}



/*
  The freshly allocated matrix is explicitly initialized to zero. If
  the variable safe_mode equals true the function will return NULL if
  the allocation of data fails, otherwise it will abort() if the
  allocation fails.
*/
static matrix_type * matrix_alloc_with_stride(int rows , int columns , int row_stride , int column_stride, bool safe_mode) {
  matrix_type * matrix  = util_malloc( sizeof * matrix, __func__);
  matrix->data      = NULL;
  matrix->data_size = 0;
  matrix_init_header( matrix , rows , columns , row_stride , column_stride);
  matrix->data_owner    = true;
  matrix_realloc_data__( matrix  , safe_mode );
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
    matrix->data          = &src->data[ GET_INDEX(src , row , column) ];
    matrix->data_owner    = false;
    
    return matrix;
  }
}


/**
   This function will allocate a matrix structure; this matrix
   structure will TAKE OWNERSHIP OF THE SUPPLIED DATA. This means that
   it is (at the very best) highly risky to use the data pointer in
   the calling scope after the matrix has been allocated. If the
   supplied pointer is too small it is immediately realloced ( in
   which case the pointer in the calling scope will be immediately
   invalid).
*/   

matrix_type * matrix_alloc_steal_data(int rows , int columns , double * data , int data_size) {
  matrix_type * matrix = util_malloc( sizeof * matrix , __func__);
  matrix_init_header( matrix , rows , columns , 1 , rows );   
  matrix->data_size  = data_size;           /* Can in general be different from rows * columns */
  matrix->data_owner = true;
  matrix->data       = data;
  if (data_size < rows * columns) 
    matrix_realloc_data__(matrix , false);
  
  return matrix;
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

static bool matrix_resize__(matrix_type * matrix , int rows , int columns , bool copy_content , bool safe_mode) {
  if (!matrix->data_owner)
    util_abort("%s: sorry - can not resize shared matrizes. \n",__func__);
  {
    bool resize_OK           = true;
    int copy_rows    	     = util_int_min( rows    , matrix->rows );
    int copy_columns 	     = util_int_min( columns , matrix->columns);
    matrix_type * copy_view  = NULL;
    matrix_type * copy       = NULL;  
    
    if (copy_content) {
      copy_view = matrix_alloc_shared( matrix , 0 , 0 , copy_rows , copy_columns);         /* This is the part of the old matrix which should be copied over to the new. */		      
      copy      = matrix_alloc_copy__( copy_view , safe_mode );                            /* Now copy contains the part of the old matrix which should be copied over - with private storage. */
    }
    {
      int old_rows , old_columns, old_row_stride , old_column_stride,old_data_size;
      matrix_get_dims( matrix , &old_rows , &old_columns , &old_row_stride , &old_column_stride);        /* Storing the old header information - in case the realloc() fails. */
      old_data_size = matrix->data_size;
      
      matrix_init_header(matrix , rows , columns , 1 , rows);                                            /* Resetting the header for the matrix */
      matrix_realloc_data__(matrix , safe_mode);
      if (matrix->data != NULL) {  /* Realloc succeeded */
	if (copy_content) {
	  matrix_type * target_view = matrix_alloc_shared(matrix , 0 , 0 , copy_rows , copy_columns);
	  matrix_assign( target_view , copy);
	  matrix_free( target_view );
	}
      } else {                                                              
	/* Failed to realloc new storage; RETURNING AN INVALID MATRIX */
	matrix_init_header(matrix , old_rows , old_columns , old_row_stride , old_column_stride);
	resize_OK = false;
      }
    }

    if (copy_content) {
      matrix_free(copy_view);
      matrix_free(copy);
    }
    return resize_OK;
  }
}


/** 
    If copy content is true the content of the old matrix is carried
    over to the new one, otherwise the new matrix is cleared.
    
    Will always return true (or abort). 
*/
bool matrix_resize(matrix_type * matrix , int rows , int columns , bool copy_content) {
  return matrix_resize__(matrix , rows , columns , copy_content , false);
}


/**
   Return true if the resize succeded, otherwise it will return false
   and leave the matrix unchanged. When resize implies expanding a
   dimension, the newly created elements will be explicitly
   initialized to zero.

   If copy_content is set to false the new matrix will be fully
   initialized to zero.
*/

bool matrix_safe_resize(matrix_type * matrix , int rows , int columns , bool copy_content) {
  return matrix_resize__(matrix , rows , columns , copy_content , true);
}



/** 
    This function will ensure that the matrix has at least 'rows'
    rows. If the present matrix already has >= rows it will return
    immediately, otherwise the matrix will be resized. 
*/

void matrix_ensure_rows(matrix_type * matrix, int rows, bool copy_content) {
  if (matrix->rows < rows)
    matrix_resize( matrix , rows , matrix->columns , copy_content);
}



/** 
    This function will reduce the size of the matrix. It will only
    affect the headers, and not touch the actual memory of the matrix.
*/

void matrix_shrink_header(matrix_type * matrix , int rows , int columns) {

  if (rows <= matrix->rows)
    matrix->rows = rows;
  
  if (columns <= matrix->columns)
    matrix->columns = columns;

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
/* Functions working on rows & columns                           */

void matrix_set_many_on_column(matrix_type * matrix , int row_offset , int elements , const double * data , int column) {
  if ((row_offset + elements) < matrix->rows) {
    if (matrix->row_stride == 1)  /* Memory is continous ... */
      memcpy( &matrix->data[ GET_INDEX( matrix , row_offset , column) ] , data , elements * sizeof * data);
    else {
      int i;
      for (i = 0; i < elements; i++)
	matrix->data[ row_offset + GET_INDEX( matrix , i , column) ] = data[i];
    }
  }
}

void matrix_set_column(matrix_type * matrix , const double * data , int column) {
  matrix_set_many_on_column( matrix , 0 , matrix->rows , data , column );
}


/*****************************************************************/
/* Matrix - matrix operations                                    */


/* Implements assignement: A = B */
void matrix_assign(matrix_type * A , const matrix_type * B) {
  if ((A->rows == B->rows) && (A->columns == B->columns)) {
    int i,j;
    
    if (A->row_stride == B->row_stride) {
      if (A->columns == A->row_stride)  /** Memory is just one continous block */
	memcpy( A->data , B->data , A->rows * A->columns * sizeof * A->data);
      else {
	/* Copying columns of data */
	for (j = 0; j < A->columns; j++)
	  memcpy( &A->data[ GET_INDEX(A , 0 , j)] , &B->data[ GET_INDEX(B , 0 , j) ] , A->rows * sizeof * A->data);
      } 
    } else {
      /* Copying element by element */
      for (j = 0; j < A->columns; j++)
	for (i=0; i < A->rows; i++)
	  A->data[ GET_INDEX(A,i,j) ] = B->data[ GET_INDEX(B,i,j) ];
    }
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
	
	/* Assign first to tmp[j] */
	tmp[j] = scalar_product;
      }
      for (j=0; j < A->columns; j++)
	A->data[ GET_INDEX(A , i, j) ] = tmp[j];
    }
    free(tmp);
  } else
    util_abort("%s: size mismatch \n",__func__);
}


static void matrix_inplace_matmul_mt__(void * arg) {

  arg_pack_type * arg_pack = arg_pack_safe_cast( arg );
  int row_offset     = arg_pack_iget_int( arg_pack , 0 );
  int rows           = arg_pack_iget_int( arg_pack , 1 );
  matrix_type * A    = arg_pack_iget_ptr( arg_pack , 2 );
  matrix_type * B    = arg_pack_iget_ptr( arg_pack , 3 );

  matrix_type * A_view = matrix_alloc_shared( A , row_offset , 0 , rows , matrix_get_columns( A ));
  matrix_inplace_matmul( A_view , B );
  matrix_free( A_view );

}


void matrix_inplace_matmul_mt(matrix_type * A, const matrix_type * B , int num_threads){ 
  thread_pool_type  * thread_pool = thread_pool_alloc( num_threads );
  arg_pack_type    ** arglist     = util_malloc( num_threads * sizeof * arglist , __func__);
  int it;
  {
    int rows       = matrix_get_rows( A ) / num_threads;
    int rows_mod   = matrix_get_rows( A ) % num_threads;
    int row_offset = 0;

    for (it = 0; it < num_threads; it++) {
      int row_size;
      arglist[it] = arg_pack_alloc();
      row_size = rows;
      if (it < rows_mod)
	row_size += 1;
      
      arg_pack_append_int(arglist[it] , row_offset );
      arg_pack_append_int(arglist[it] , row_size   );
      arg_pack_append_ptr(arglist[it] , A );
      arg_pack_append_ptr(arglist[it] , B );
      
      thread_pool_add_job( thread_pool , matrix_inplace_matmul_mt__ , arglist[it]);
      row_offset += row_size;
    }
    thread_pool_join( thread_pool );
    thread_pool_free( thread_pool );
    for (it = 0; it < num_threads; it++) 
      arg_pack_free( arglist[it] );
    free( arglist );
  }
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



/**
   For each row in the matrix we will do the operation

     R -> R - <R>
*/

void matrix_subtract_row_mean(matrix_type * matrix) {
  int i;
  for (i=0; i < matrix->rows; i++) {
    double row_mean = matrix_get_row_sum(matrix , i) / matrix->columns;
    int j;
    for (j=0; j < matrix->columns; j++) 
      matrix->data[ GET_INDEX( matrix , i , j ) ] -= row_mean;
  }
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


bool matrix_is_quadratic(const matrix_type * matrix) {
  if (matrix->rows == matrix->columns)
    return true;
  else
    return false;
}


/**
   Return true if the two matrices m1 and m2 are equal. The equality
   test is based on element-by-element memcmp() comparison, i.e. the
   there is ZERO numerical tolerance in the comparison.
   
   If the two matrices do not have equal dimension false is returned. 
*/

bool matrix_equal( const matrix_type * m1 , const matrix_type * m2) {
  if (! ((m1->rows == m2->rows) && (m1->columns == m2->columns)))
    return false;
  {    
    int i,j;
    for (j=0; j < m1->columns; j++) {
      for (i=0; i < m1->rows; i++) {
	if (memcmp( &m1->data[ GET_INDEX(m1 , i , j)]  , &m2->data[ GET_INDEX(m2 , i,j)] , sizeof * m1->data) != 0)
	  return false;
      }
    }
  }

  /** OK - we came all the way through - they are equal. */
  return true;
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



void matrix_clear( matrix_type * matrix ) {
  matrix_set( matrix , 0 );
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
