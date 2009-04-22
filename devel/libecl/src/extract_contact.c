#include <ecl_grid.h>
#include <ecl_util.h>
#include <fortio.h>
#include <stdbool.h>
#include <ecl_kw.h>
#include <util.h>
#include <string.h>
#include <int_vector.h>
#include <double_vector.h>



void extract_contact(const ecl_kw_type   * swat1    , 
		     const ecl_kw_type   * swat2    , 
		     const ecl_grid_type * ecl_grid , 
		     const int_vector_type * ilist  , 
		     const int_vector_type * jlist  , 
		     double_vector_type * contact   , 
		     const char * target_file) {
  
  int index,k,nz,active_size,nx,ny;
  double * diff;
  double * z;
  FILE * target_stream = util_fopen( target_file , "w"); 

  ecl_grid_get_dims( ecl_grid , &nx, &ny , &nz , &active_size);
  diff = util_malloc( nz * sizeof * diff , __func__);
  z    = util_malloc( nz * sizeof * z    , __func__);
  
  for (index = 0; index < int_vector_size( ilist ); index++) {
    double z_sum = 0;
    double w_sum = 0;
    int i 	 = int_vector_iget( ilist , index);
    int j 	 = int_vector_iget( jlist , index);
    
    for (k=0; k < nz; k++) {
      int active_index = ecl_grid_get_active_index( ecl_grid , i , j , k);
      if (active_index >= 0) {
	double sw1 = ecl_kw_iget_as_double( swat1 , active_index );
	double sw2 = ecl_kw_iget_as_double( swat2 , active_index );
	
	diff[k] = (sw2 - sw1);  /* Absolute difference in saturation */
      } else
	diff[k] = -1;           /* Not active */
      
      {
	double xpos,ypos;
	ecl_grid_get_pos(ecl_grid , i , j , k , &xpos , &ypos , &z[k]);
	if (diff[k] > 0) {
	  z_sum += z[k] * diff[k];
	  w_sum += diff[k];
	}
      }
    }
    
    
    /* Writing the profile - just for debugging/inspection */
    {
      char * filename = util_alloc_sprintf("%s_profile_%d.%d" , target_file , i ,j);
      FILE * stream   = util_fopen(filename , "w");
      
      for (k=0; k < nz; k++)
	fprintf(stream , "%d   %g   %g   \n",k , z[k] , diff[k]);
      
      fclose(stream);
      free(filename);
    }
    
    if (w_sum > 0)
      fprintf(target_stream , "%g\n", z_sum / w_sum);
    else
      fprintf(target_stream , "0.0\n");
    
  }
  fclose( target_stream );
  free( diff );
}





int main (int argc , char ** argv) {
  if (argc != 6) 
    util_exit("%s: needs four arguments: GRID_FILE  RESTART_FILE1  RESTART_FILE2  INDEX_FILE  RESULT_FILE\n");
  
  {
    const char * grid_file     = argv[1];
    const char * restart_file1 = argv[2];
    const char * restart_file2 = argv[3];
    const char * index_file    = argv[4];
    const char * result_file   = argv[5];
    int_vector_type * ilist    = int_vector_alloc(100 , -1);
    int_vector_type * jlist    = int_vector_alloc(100 , -1);
    double_vector_type * contact = double_vector_alloc(100 , 0.0);

    int nx,ny,nz,active_size;
    bool fmt_file;
    ecl_file_enum file_type;
    ecl_grid_type * grid = ecl_grid_alloc( grid_file , true );
    ecl_kw_type 	* swat1 = NULL;
    ecl_kw_type 	* swat2 = NULL;
  
  
    ecl_grid_get_dims( grid , &nx , &ny , &nz , &active_size );
    ecl_util_get_file_type( restart_file1 , &file_type , &fmt_file , NULL);
    if (file_type != ecl_restart_file)
      util_exit("Files must be of type not unified restart_file \n");
    {
      fortio_type * fortio = fortio_fopen(restart_file1 , "r" , true , fmt_file);
      if (ecl_kw_fseek_kw("SWAT" , true , false , fortio))
	swat1 = ecl_kw_fread_alloc(fortio);
      fortio_fclose( fortio );
    }

    {
      fortio_type * fortio = fortio_fopen(restart_file2 , "r" , true , fmt_file);
      if (ecl_kw_fseek_kw("SWAT" , true , false , fortio))
	swat2 = ecl_kw_fread_alloc(fortio);
      fortio_fclose( fortio );
    }

    {
      FILE * stream = util_fopen(index_file , "r");
      int read_count;
      do {
	int i,j;
	read_count = fscanf(stream , "%d %d" , &i , &j);
	if (read_count == 2) {
	  int_vector_append(ilist , i);
	  int_vector_append(jlist , j);
	}
      } while (read_count == 2);
      fclose(stream);
    }

    extract_contact(swat1 , swat2 , grid , ilist , jlist , contact , result_file);
    ecl_kw_free(swat1);
    ecl_kw_free(swat2);
    ecl_grid_free( grid );
    double_vector_free( contact );
    int_vector_free(ilist);
    int_vector_free(jlist);
  }
}
