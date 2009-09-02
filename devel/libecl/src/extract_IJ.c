#include <ecl_kw.h>
#include <ecl_grid.h>
#include <int_vector.h>
#include <bool_vector.h>
#include <util.h>


int main (int argc , char ** argv) {
  if (argc != 5)
    util_exit("Usage:  GRID_FILE  REGIONS_FILE   REGION_KW   REGION_VALUE \n");
  {
    char * grid_file       = argv[1];
    char * regions_file    = argv[2];
    char * kw              = argv[3];
    char * region_value_st = argv[4];
    
    {
      bool_vector_type * IJ_set            = bool_vector_alloc( 0 , false );
      int_vector_type  * global_index_list = int_vector_alloc( 0 , -1 );
      ecl_grid_type    * ecl_grid = ecl_grid_alloc( grid_file );
      ecl_kw_type      * regions_kw;
      FILE             * stream = util_fopen( regions_file , "r"); 
      int               region_value,nx,ny,nz;
      ecl_kw_grdecl_fseek_kw( kw , false , true , stream );
      ecl_grid_get_dims(ecl_grid , &nx , &ny , &nz , NULL);
      regions_kw = ecl_kw_fscanf_alloc_grdecl_data( stream , nx*ny*nz , ecl_int_type);
      
      util_sscanf_int( region_value_st , &region_value);
      ecl_grid_get_region_cells( ecl_grid , regions_kw , region_value , true , false , global_index_list);
      
      {
	int index;
	for (index = 0; index < int_vector_size( global_index_list ); index++) {
	  int i,j,k,ij; 
	  ecl_grid_get_ijk1( ecl_grid , int_vector_iget( global_index_list , index) , &i, &j , &k);
	  ij = i + j*nx;
	  if (!bool_vector_safe_iget( IJ_set , ij)) {
	    printf("%4d  %4d \n",i,j);
	    bool_vector_iset( IJ_set , ij , true);
	  }
	}
      }
      
      bool_vector_free(IJ_set);
      int_vector_free( global_index_list );
      ecl_kw_free( regions_kw );
      ecl_grid_free( ecl_grid );
    }
  }
}
