#include <stdlib.h>
#include <ecl_kw.h>
#include <fortio.h>


int main(int argc, char ** argv) {
  fortio_type * fortio = fortio_open(argv[1] , "r" , true);
  ecl_kw_fseek_last_kw("MINISTEPXX" , false , true , fortio);
  {
    ecl_kw_type * ecl_kw = ecl_kw_fread_alloc(fortio , false);
    printf("Looking at: %s \n",ecl_kw_get_header_ref(ecl_kw));
    ecl_kw_free(ecl_kw);
  }
  return 0;
}
