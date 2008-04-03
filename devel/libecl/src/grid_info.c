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
      int             active_cells , nx,ny,nz;
      
      ecl_grid = ecl_grid_alloc(grid_file , endian_flip);

      ecl_grid_get_dims(ecl_grid , &nx , &ny , &nz , &active_cells);
      printf("	Grid file .......: %s  \n",grid_file);
      printf("	Active cells ....: %d \n",active_cells);
      printf("	nx ..............: %d \n",nx);
      printf("	ny ..............: %d \n",ny);
      printf("	nz ..............: %d \n",nz);
      printf("	Volume ..........: %d \n",nx*ny*nz);
      ecl_grid_free(ecl_grid);
  }
}
