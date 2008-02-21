#include <ecl_grid.h>
#include <stdlib.h>
#include <stdio.h>



int main(int argc, char ** argv) {
  if (argc != 2) {
    fprintf(stderr,"%s: filename \n",argv[0]);
    exit(1);
  }
  {
      bool endian_flip = true;
      ecl_grid_type * ecl_grid;
      const char    * grid_file = argv[1];
      int             active_cells;
      
      ecl_grid = ecl_grid_alloc(grid_file , endian_flip);

      ecl_grid_get_dims(ecl_grid , NULL , NULL , NULL , &active_cells);
      printf("Grid file .......: %s  \n",grid_file);
      printf("Active cells ....: %8d \n",active_cells);
      ecl_grid_free(ecl_grid);
  }
}
