/*
   Copyright (C) 2011  Statoil ASA, Norway. 
    
   The file 'util-test.c' is part of ERT - Ensemble based Reservoir Tool. 
    
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
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <util.h>
#include <string.h>      
#include <path_fmt.h>
#include <stdarg.h>
#include <hash.h>
#include <unistd.h>
#include <thread_pool.h>
#include <stringlist.h>
#include <menu.h>
#include <arg_pack.h>
#include <vector.h>
#include <double_vector.h>
#include <matrix.h>
#include <matrix_lapack.h>
#include <matrix_blas.h>
#include <parser.h>
#include <block_fs.h>
#include <thread_pool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <subst_list.h>
#include <subst_func.h>
#include <buffer.h>
#include <mzran.h>
#include <statistics.h>
#include <double_vector.h>
#include <lookup_table.h>



int main(int argc , char ** argv) {
  matrix_type * m1 = matrix_alloc(10,10);
  matrix_type * m2 = matrix_alloc(5,5);

  matrix_scalar_set( m1 , 1 );
  matrix_scalar_set( m2 , 2 );

  matrix_pretty_fprint( m1 , "m1" , " %1.0f " , stdout );
  printf("\n\n");
  matrix_pretty_fprint( m2 , "m2" , " %1.0f " , stdout );

  printf("\n\n");
  matrix_copy_block( m1 , 2 , 2 , 3 , 3 , m2 , 0 , 0 );
  matrix_pretty_fprint( m1 , "m1" , " %1.0f " , stdout );

}


