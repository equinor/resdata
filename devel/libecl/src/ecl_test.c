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
  ecl_grid_type *ecl_grid;
  ecl_kw_type *ecl_kw;
  ecl_file_type *ecl_file;
  int global_size;
  int active_size;
  int i;
  int size;

  ecl_grid = ecl_grid_alloc (GRID_FILE, 1);
  global_size = ecl_grid_get_global_size (ecl_grid);
  active_size = ecl_grid_get_active_size (ecl_grid);

  ecl_file = ecl_file_fread_alloc (RESTART_FILE, 1);
  ecl_kw = ecl_file_iget_named_kw (ecl_file, "PRESSURE", 0);
  size = ecl_kw_get_size(ecl_kw);

  for (i = 0; i < global_size; i++)
    {
      void *float_data;
      double double_data;
      int ret;
      int x, y, z;

      ret = ecl_grid_get_active_index1 (ecl_grid, i);
      if (ret != -1)
	{
	  float_data = ecl_kw_iget_ptr (ecl_kw, ret);
	  double_data = ecl_kw_iget_as_double(ecl_kw, ret);

	  ecl_grid_get_ijk1A (ecl_grid, ret, &x, &y, &z);
	  printf ("pressure: %f double: %f on index %d (%d, %d, %d)\n",
		  ((float *) float_data)[ret], double_data, ret, x, y, z);
	}

    }

  printf ("active_size: %d, global_size: %d\n", active_size, global_size);


}
