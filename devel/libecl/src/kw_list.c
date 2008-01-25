#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <ecl_kw.h>
#include <fortio.h>
#include <string.h>
#include <ecl_fstate.h>
#include <restart_kw_list.h>


void kw_list(const char *filename) {
  fortio_type *fortio;
  bool fmt_file = ecl_fstate_fmt_file(filename);
  restart_kw_list_type * kw_list = restart_kw_list_alloc();

  printf("-----------------------------------------------------------------\n");
  printf("%s: \n",filename); 
  fortio = fortio_open(filename , "r" , true);
  ecl_kw_type * ecl_kw = ecl_kw_alloc_empty(fmt_file , true);
  while(  ecl_kw_fread_realloc(ecl_kw , fortio) ) {
    ecl_kw_summarize(ecl_kw);
    restart_kw_list_add(kw_list , ecl_kw_get_header_ref(ecl_kw));
  }
  printf("-----------------------------------------------------------------\n");

  ecl_kw_free(ecl_kw);
  fortio_close(fortio);

  {
    const char * kw;
    restart_kw_list_reset(kw_list);
    kw = restart_kw_list_get_next(kw_list);
    while (kw != NULL) {
      printf("kw:%s \n",kw);
      kw = restart_kw_list_get_next(kw_list);
    }
    restart_kw_list_free(kw_list);
  }
}


int main (int argc , char **argv) {
  int i;
  for (i = 1; i < argc; i++)
    kw_list(argv[i]);
  return 0;
}
