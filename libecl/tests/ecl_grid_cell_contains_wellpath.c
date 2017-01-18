/*
   Copyright (C) 2017  Statoil ASA, Norway.

   The file 'ecl_grid_cell_contains_wellpath.c' is part of ERT -
   Ensemble based Reservoir Tool.

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
#include <stdbool.h>
#include <math.h>

#include <ert/util/test_util.h>
#include <ert/ecl/ecl_grid.h>
#include <ert/util/vector.h>


typedef struct {
  double x;
  double y;
  double z;
  double md;

  int i;
  int j;
  int k;
} point_type;



vector_type * load_expected( const char * filename ) {
  FILE * stream = util_fopen( filename , "r");
  vector_type * expected = vector_alloc_new();

  while (true) {
    double x,y,z,md;
    int i,j,k;

    if (fscanf( stream , "%lg %lg %lg %lg %d %d %d" , &x,&y,&md,&z,&i,&j,&k) == 7) {
      point_type * p = util_malloc( sizeof * p );
      p->x = x;
      p->y = y;
      p->md = md;
      p->z = z;

      p->i = i-1;
      p->j = j-1;
      p->k = k-1;

      vector_append_owned_ref( expected, p , free );
    } else
      break;
  }

  fclose( stream );
  test_assert_int_equal( 10 , vector_get_size( expected ));
  return expected;
}



int main(int argc , char ** argv) {
  util_install_signals();
  {
    ecl_grid_type * grid = ecl_grid_alloc( argv[1] );
    vector_type * expected = load_expected( argv[2] );

    for (int i=0; i < vector_get_size( expected ); i++) {
      const point_type * p = vector_iget_const( expected , i );
      int g = ecl_grid_get_global_index_from_xyz(grid , p->x, p->y , p->z , 0 );
      test_assert_int_equal( g , ecl_grid_get_global_index3(grid, p->i,p->j, p->k));
    }
    ecl_grid_free( grid );
    vector_free( expected );
  }
  exit(0);
}
