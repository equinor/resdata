#include <ecl_grid.h>
#include <util.h>
#include <double_vector.h>



double __avg(const double_vector_type * x) {
  int size = double_vector_size(x);
  if (size > 0) {
    double s = 0;
    int i;
    for (i = 0; i < size; i++)
      s += double_vector_iget( x , i);
    
    return s / size;
  } else return 0;
}


void block_file(ecl_grid_type * ecl_grid ,const char * filename) {
  FILE * stream = util_fopen(filename , "r");
  char * basename  , *path;
  int read_count;

  util_alloc_file_components(filename , &path , &basename , NULL);
  ecl_grid_init_blocking( ecl_grid );
  do {
    double xpos,ypos,owc;
    read_count = fscanf(stream , "%lg %lg %lg" , &xpos , &ypos , &owc);
    if (read_count == 3)
      ecl_grid_block_value_2d(ecl_grid , xpos , ypos , owc);
  } while (read_count == 3);
  ecl_grid_do_blocking( ecl_grid , __avg);

  {
    int i;
    int j;
    int nx,ny,nz,active;
    char * index_file = util_alloc_filename(path , basename , "active");
    char * owc_file   = util_alloc_filename(path , basename , "owc");

    FILE * indexH = util_fopen(index_file , "w");
    FILE * owcH   = util_fopen(owc_file   , "w");
    
    printf("Blocking %s -> [%s , %s] \n",filename , owc_file , index_file);
    ecl_grid_get_dims(ecl_grid , &nx,&ny,&nz,&active);
    for (j=0; j < ny; j++) {
      for (i=0; i < nx; i++)
	if (ecl_grid_blocked_cell_active_2d(ecl_grid , i , j)) {
	  fprintf(owcH , " %g \n",ecl_grid_get_blocked_value_2d(ecl_grid , i , j));
	  fprintf(indexH , "%03d  %03d  \n",i,j);
	}
      }
    fclose(indexH);
    fclose(owcH);
    free(index_file);
    free(owc_file);
  }
  free(basename);
  free(path);
  fclose(stream);
}





int main(int argc , char ** argv) {
  int iarg;
  ecl_grid_type * ecl_grid  = ecl_grid_alloc( argv[1] , true);

  
  ecl_grid_alloc_blocking_variables( ecl_grid , 2);
  for (iarg = 2; iarg < argc; iarg++)
    block_file (ecl_grid , argv[iarg]);

  ecl_grid_free( ecl_grid );

}
