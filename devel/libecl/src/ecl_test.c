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

int main(int argc, char ** argv) {
  ecl_grid_type * ecl_grid = ecl_grid_alloc("/d/proj/bg/enkf/share/Gurbat2/HM_ENKF/STFO/REF/STFO2008_REF.EGRID" , true);
  ecl_grid_summarize(ecl_grid);
  ecl_grid_get_active_index( ecl_grid , 35 , 74 , 7);
  ecl_grid_get_active_index( ecl_grid , 35 , 74 ,14);
  ecl_grid_get_active_index( ecl_grid , 34 , 74 ,18);
  ecl_grid_get_active_index( ecl_grid , 34 , 74 ,21);
}
