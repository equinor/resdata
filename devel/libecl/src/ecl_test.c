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

#define GRID_FILE "/d/felles/bg/scratch/masar/STRUCT/realization-0-step-0-to-358/RG01-STRUCT-0.EGRID"
#define RESTART_FILE "/d/felles/bg/scratch/masar/STRUCT/realization-0-step-0-to-358/RG01-STRUCT-0.X0050"

int
main (int argc, char **argv)
{
  ecl_sum_type * ecl_sum = ecl_sum_fread_alloc_case("../../libenkf/src/Gurbat/EXAMPLE_01_BASE.DATA" );
  
  stringlist_type * keys = ecl_sum_alloc_matching_general_var_list( ecl_sum , argv[1]);
  stringlist_fprintf(keys , " "  , stdout);
  stringlist_free( keys );
    
  ecl_sum_free( ecl_sum );
}
