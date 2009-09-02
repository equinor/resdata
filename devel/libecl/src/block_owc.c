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


double __error(const double_vector_type * x) {
  double min =  9999999999.0;
  double max = -min;
  int size = double_vector_size(x);
  if (size > 0) {
    int i;
    for (i = 0; i < size; i++) {
      double value = double_vector_iget( x , i);
      if (value > max)
	max = value;
      if (value < min)
	min = value;
    }
    return (max - min) / 2;
  } else return 0;
}



void block_file(ecl_grid_type * ecl_grid ,const char * raw_data, const char * index_read_file , const char * blocked_file, const char * index_write_file, double owc_std) {
  FILE * stream = util_fopen(raw_data , "r");
  int read_count;

  ecl_grid_init_blocking( ecl_grid );
  do {
    double xpos,ypos,owc;
    read_count = fscanf(stream , "%lg %lg %lg" , &xpos , &ypos , &owc);
    if (read_count == 3)
      ecl_grid_block_value_2d(ecl_grid , xpos , ypos , owc);
  } while (read_count == 3);
  fclose(stream );

  {
    int i;
    int j , index;
    int nx,ny,nz,active;
    int_vector_type * ilist = int_vector_alloc( 0 , -1 );
    int_vector_type * jlist = int_vector_alloc( 0 , -1 );
    FILE * owcH;
    FILE * index_writeH;
    {
      FILE * stream = util_fopen(index_read_file , "r");
      int read_count;
      do {
	read_count = fscanf(stream , "%d %d" , &i , &j);
	if (read_count == 2) {
	  int_vector_append(ilist , i);
	  int_vector_append(jlist , j);
	}
      } while (read_count == 2);
      fclose(stream);
    }
    
    
    owcH   = util_fopen(blocked_file   , "w");
    index_writeH = util_fopen(index_write_file , "w");
    ecl_grid_get_dims(ecl_grid , &nx,&ny,&nz,&active);
    for (index = 0; index < int_vector_size( ilist ); index++) {
      i = int_vector_iget( ilist , index );
      j = int_vector_iget( jlist , index );
      if (ecl_grid_get_block_count2d(ecl_grid , i , j) > 2) {
	fprintf(owcH , " %g  %g \n",ecl_grid_block_eval2d(ecl_grid , i , j , __avg) , owc_std);
	fprintf(index_writeH , "%d  %d \n",i,j);
      }
    }
    fclose(index_writeH);
    fclose(owcH);
  }
}





int main(int argc , char ** argv) {
  if (argc != 7)
    util_exit("Usage: GRID_FILE  RAW_OWC  IJ_index   OUTFILE_woc   OUTFILE_index  OWC_STD\n");
  {
    ecl_grid_type * ecl_grid     = ecl_grid_alloc( argv[1] );
    const char    * raw_file     = argv[2];
    const char    * index_file   = argv[3];
    const char    * blocked_file = argv[4];
    const char    * blocked_index_file = argv[5];
    double          owc_std            = atof(argv[6]);
    
    ecl_grid_alloc_blocking_variables( ecl_grid , 2);
    block_file (ecl_grid , raw_file , index_file , blocked_file , blocked_index_file , owc_std);
    ecl_grid_free( ecl_grid );
  }

}
