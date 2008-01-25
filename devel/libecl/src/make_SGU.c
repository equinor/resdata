#include <stdlib.h>
#include <ecl_kw.h>
#include <fortio.h>
#include <string.h>
#include <ecl_util.h>
#include <util.h>




int main(int argc, char ** argv) {
  int i;
  char *swl_file;
  char *sgu_file;
  fortio_type * fortio;
  ecl_kw_type * swl_kw , *sgu_kw;

  for (i=1; i < argc; i++) {
    swl_file = argv[i];
    sgu_file = util_alloc_string_copy(swl_file);
    
    sgu_file[1] = 'G';
    sgu_file[2] = 'U';
    
    printf("Reading: %s ",swl_file); fflush(stdout);
    fortio = fortio_open(argv[i] , "r" , true);
    swl_kw = ecl_kw_fread_alloc(fortio , false);
    fortio_close(fortio);

    printf("Writing: %s ",sgu_file); fflush(stdout);
    sgu_kw = ecl_kw_alloc_copy(swl_kw);
    ecl_kw_set_header_name(sgu_kw , "SGU");
    ecl_kw_scalar_init(sgu_kw , 1.0);
    ecl_kw_inplace_sub(sgu_kw , swl_kw);
    
    fortio = fortio_open(sgu_file , "w" , true);
    ecl_kw_fwrite(sgu_kw , fortio);
    fortio_close(fortio);

    ecl_kw_free(sgu_kw);
    ecl_kw_free(swl_kw);
    free(sgu_file);
    printf("\n");
  }
  return 0;
}
