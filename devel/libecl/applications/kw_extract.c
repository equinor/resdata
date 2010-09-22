#include <stdlib.h>
#include <ecl_kw.h>
#include <fortio.h>
#include <set.h>
#include <util.h>
#include <string.h>
#include <ecl_util.h>
#include <ecl_sum.h>
#include <hash.h>
#include <stdbool.h>
#include <ecl_grid.h>
#include <ecl_endian_flip.h>
#include <msg.h>

/**
   This file will extract all occurences of kw1,kw2,...,kwn from the
   source file and copy them over to the target file. Ordering in the
   target file will be according to the ordering in the source file,
   and not by the ordering given on the command line.
*/

int main(int argc, char ** argv) {
  if (argc < 4) {
    fprintf(stderr,"%s  src_file target_file kw1 kw2 kw3 \n",argv[0]);
    exit(0);
  }
  {
    const char *  src_file   = argv[1];
    const char * target_file = argv[2];
    const char ** kw_list    = (const char **) &argv[3];
    int num_kw               = argc - 3;
    fortio_type * fortio_src;
    fortio_type * fortio_target;
    int ikw;
    bool fmt_src , fmt_target;
    set_type    * kw_set = set_alloc( num_kw , kw_list );
    
    fmt_src           = ecl_util_fmt_file(src_file);
    fmt_target        = fmt_src;                         /* Can in principle be different */
    fortio_src        = fortio_fopen(src_file    , "r" , ECL_ENDIAN_FLIP, fmt_src);
    fortio_target     = fortio_fopen(target_file , "w" , ECL_ENDIAN_FLIP, fmt_target);

    {
      ecl_kw_type * ecl_kw = ecl_kw_alloc_empty();
      while (true) {
        if (ecl_kw_fread_header( ecl_kw , fortio_src )) {
          const char * header = ecl_kw_get_header( ecl_kw ); 
          if (set_has_key( kw_set , header )) {
            ecl_kw_fread_realloc_data(ecl_kw , fortio_src );
            ecl_kw_fwrite( ecl_kw , fortio_target );
          } else
            ecl_kw_fskip_data( ecl_kw , fortio_src );
        } else 
          break; /* We have reached EOF */
      }
      ecl_kw_free( ecl_kw );
    }
    
    fortio_fclose(fortio_src);
    fortio_fclose(fortio_target);
    set_free( kw_set );
  }
}
