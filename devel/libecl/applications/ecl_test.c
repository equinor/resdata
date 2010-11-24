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
#include <time.h>
#include <ecl_grav.h>

int main(int argc , char ** argv) {
  ecl_file_type * restart1  = ecl_file_fread_alloc_unrst_section("/private/joaho/EnKF/devel/EnKF/python/ctypes/test/data/eclipse/case/ECLIPSE.UNRST" , 10);
  ecl_file_type * restart2  = ecl_file_fread_alloc_unrst_section("/private/joaho/EnKF/devel/EnKF/python/ctypes/test/data/eclipse/case/ECLIPSE.UNRST" , 40);
  ecl_file_type * init_file = ecl_file_fread_alloc("/private/joaho/EnKF/devel/EnKF/python/ctypes/test/data/eclipse/case/ECLIPSE.INIT");
  ecl_grid_type * ecl_grid  = ecl_grid_alloc( "/private/joaho/EnKF/devel/EnKF/python/ctypes/test/data/eclipse/case/ECLIPSE.EGRID" );

  {
    ecl_kw_type * swat_kw1 = ecl_file_iget_named_kw( restart1 , "SWAT" , 0 );
    ecl_kw_type * wat_den1 = ecl_file_iget_named_kw( restart1 , "WAT_DEN" , 0 );
    ecl_kw_type * rporv1   = ecl_file_iget_named_kw( restart1 , "RPORV" , 0 );
    ecl_kw_type * swat_kw2 = ecl_file_iget_named_kw( restart2 , "SWAT" , 0 );
    ecl_kw_type * wat_den2 = ecl_file_iget_named_kw( restart2 , "WAT_DEN" , 0 );
    ecl_kw_type * rporv2   = ecl_file_iget_named_kw( restart2 , "RPORV" , 0 );
    double deltag;
    
    deltag = ecl_grav_phase_deltag( 5354 , 9329 , 100 , ecl_grid , NULL , swat_kw1 , wat_den1 , rporv1 , swat_kw2 , wat_den2 , rporv2 );
    printf("deltag:%g \n",deltag);
  }


  ecl_file_free( restart1 );
  ecl_file_free( restart2 );
  ecl_file_free( init_file );
  ecl_grid_free( ecl_grid );
}
