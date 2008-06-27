#include <stdlib.h>
#include <ecl_kw.h>
#include <fortio.h>
#include <util.h>
#include <string.h>
#include <ecl_util.h>
#include <ecl_sum.h>
#include <hash.h>
#include <stdbool.h>
#include <ecl_rft_vector.h>
#include <ecl_grid.h>


void test_file(const char * filename) {
  bool endian_swap;
  if (fortio_is_fortran_file(filename , &endian_swap)) 
    printf("File:%s comes from fortran. Endian_swap:%d \n",filename , endian_swap);
  else
    printf("File:%s does not come from fortran. \n",filename);
}



int main(int argc, char ** argv) {

  test_file("fortio.o");
  test_file("/d/felles/bg/scratch/Gurbat/RunPath/tmpdir_81/EXAMPLE_01_BASE_0081.GRID");

}
