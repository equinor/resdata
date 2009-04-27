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

int main(int argc, char ** argv) {
  ecl_file_type * restart50 = ecl_file_fread_alloc_unrst_section("Gurbat/EXAMPLE_01_BASE.UNRST" , 90 , true);
  if (restart50 != NULL)
    ecl_file_free( restart50 );
  else
    printf("Could not find ... \n");
}
