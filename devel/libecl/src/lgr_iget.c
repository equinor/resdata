#include <ecl_grid.h>
#include <ecl_file.h>



int main(int argc , char ** argv) {
  ecl_grid_type * ecl_grid = ecl_grid_alloc( argv[1] );


  ecl_file_type * ecl_file = ecl_file_fread_alloc( argv[2] );
  char * lgr_name          = argv[3];


  {
    ecl_grid_type * lgr        = ecl_grid_get_lgr( ecl_grid , lgr_name );
    int grid_nr                = ecl_grid_get_grid_nr( lgr );
    ecl_kw_type  * pressure_kw = ecl_file_iget_named_kw( ecl_file , "PRESSURE" , grid_nr );
    int active_index           = ecl_grid_get_active_index3( lgr , 2 , 2 , 2);
    
    printf("PRESSURE(%s:2,2,2) = %g \n",lgr_name , ecl_kw_iget_as_double( pressure_kw , active_index ));
  }
  ecl_grid_free( ecl_grid );
  ecl_file_free( ecl_file );
}


   
