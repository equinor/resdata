#include <stdlib.h>
#include <ecl_kw.h>
#include <fortio.h>
#include <util.h>
#include <string.h>
#include <ecl_util.h>
#include <ecl_sum.h>
#include <hash.h>
#include <stdbool.h>
#include <ecl_rft_file.h>
#include <ecl_grid.h>
#include <ecl_smspec.h>
#include <ecl_sum_data.h>
#include <ecl_file.h>
#include <point.h>
#include <tetrahedron.h>


static void check_point( const ecl_grid_type * ecl_grid , int global_index ) {
  int i,j,k;
  double x,y,z;

  ecl_grid_get_xyz1( ecl_grid , global_index , &x , &y , &z );
  ecl_grid_get_ijk1( ecl_grid , global_index , &i , &j , &k );
  printf("%5d -> %d,%d,%d = %6.2f,%6.2f,%6.2f \n",global_index , i,j,k,x,y,z);
  printf("Contains: %d \n", ecl_grid_cell_contains_xyz1( ecl_grid , global_index + 000, x , y , z));

}


int main (int argc, char **argv)
{
  char * grid_file = argv[1];
  
  ecl_grid_type * grid = ecl_grid_alloc( grid_file );
  check_point( grid , 17000 );
}
