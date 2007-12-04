#include <stdlib.h>
#include <ecl_kw.h>
#include <fortio.h>
#include <util.h>
#include <string.h>
#include <ecl_util.h>
#include <ecl_sum.h>
#include <hash.h>
#include <stdbool.h>
#include <ecl_rft_vector.h>
#include <ecl_grid.h>


int main(int argc, char ** argv) {
  if (argc < 4) {
    fprintf(stderr,"%s  src_file target_file kw1 kw2 kw3 \n",argv[0]);
    exit(0);
  }
  {
    const char *  src_file   = argv[1];
    const char * target_file = argv[2];
    const char ** kw_list  = (const char **) &argv[3];
    bool endian_convert    = true;
    int num_kw = argc - 3;
    fortio_type * fortio_src;
    fortio_type * fortio_target;

    bool fmt_file;
    int ikw;
    
    ecl_util_get_file_type(src_file , NULL , &fmt_file , NULL);
    fortio_src    = fortio_open(src_file    , "r" , endian_convert);
    fortio_target = fortio_open(target_file , "w" , endian_convert);
    {
      ecl_kw_type * ecl_kw = ecl_kw_alloc_empty(fmt_file , endian_convert);
      for (ikw = 0; ikw < num_kw; ikw++) {
	if (ecl_kw_fseek_kw(kw_list[ikw] , fmt_file , true , false , fortio_src)) {
	  ecl_kw_fread_realloc(ecl_kw , fortio_src);
	  ecl_kw_fwrite(ecl_kw , fortio_target);
	} else 
	  fprintf(stderr,"** Warning: could not locate keyword:%s in file:%s \n",kw_list[ikw] , src_file);
      }
      ecl_kw_free(ecl_kw);
    }
    fortio_close(fortio_src);
    fortio_close(fortio_target);
  }
}
