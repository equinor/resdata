/*
   Copyright (C) 2011  Statoil ASA, Norway. 
    
   The file 'matrix_test.c' is part of ERT - Ensemble based Reservoir Tool. 
    
   ERT is free software: you can redistribute it and/or modify 
   it under the terms of the GNU General Public License as published by 
   the Free Software Foundation, either version 3 of the License, or 
   (at your option) any later version. 
    
   ERT is distributed in the hope that it will be useful, but WITHOUT ANY 
   WARRANTY; without even the implied warranty of MERCHANTABILITY or 
   FITNESS FOR A PARTICULAR PURPOSE.   
    
   See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html> 
   for more details. 
*/

#include <stdlib.h>
#include <stdio.h>
#include <matrix.h>
#include <rng.h>
#ifdef HAVE_LAPACK
#include <matrix_lapack.h>
#endif

int main( int argc, char ** argv)  {
  rng_type * rng   =  rng_alloc( MZRAN , INIT_DEV_RANDOM );
  matrix_type * A  =  matrix_alloc( 12 , 12 );
  matrix_type * B  =  matrix_alloc( 12 , 12 );
  matrix_random_init( A , rng );
  matrix_assign( B , A );
  matrix_pretty_print( A , "    A " , "%8.4f" );
  
#ifdef HAVE_LAPACK
  matrix_inv( B );
  printf("\n");
  matrix_pretty_print( B , "inv(A)" , "%8.4f" );
  matrix_inplace_matmul( B , A );
  printf("\n");
  matrix_pretty_print( B , "    I " , "%8.4f" );
  {
    matrix_type * A3 = matrix_alloc(3,3);
    matrix_random_init( A3 , rng );
    
    printf("matrix_det3:%g  ",matrix_det3( A3 ));
    printf("matrix_det:%g \n",matrix_det( A3 ));
  }
  
#endif
  
  matrix_free( A );
  matrix_free( B );
  rng_free( rng );
}

