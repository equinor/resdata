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

int main(int argc , char ** argv) {
  for (int iarg = 1; iarg < argc; iarg++) {
    ecl_file_type * ecl_file = ecl_file_fread_alloc( argv[iarg] );
    const ecl_kw_type   * swat_kw  = ecl_file_iget_named_kw( ecl_file , "SWAT" , 0 );
    const ecl_kw_type   * sgas_kw  = ecl_file_iget_named_kw( ecl_file , "SGAS" , 0 );
    ecl_kw_type         * soil_kw  = ecl_kw_alloc_copy( swat_kw );
    
    ecl_kw_scale( soil_kw , -1.0 );
    ecl_kw_shift( soil_kw ,  1.0 );
    ecl_kw_inplace_sub(soil_kw , sgas_kw );
    ecl_kw_set_header_name( soil_kw , "SOIL" );

    ecl_file_insert_kw( ecl_file , soil_kw , false , "ENDSOL" , 0 );
    ecl_file_fwrite( ecl_file , argv[iarg] , false );
    ecl_file_free( ecl_file );
  }
}
