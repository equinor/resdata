#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <ecl_kw.h>
#include <fortio.h>
#include <string.h>
#include <ecl_fstate.h>


void kw_list(const char *filename) {
  fortio_type *fortio;
  bool fmt_file = ecl_fstate_fmt_file(filename);

  printf("-----------------------------------------------------------------\n");
  printf("%s: \n",filename); 
  fortio = fortio_fopen(filename , "r" , true);
  ecl_kw_type * ecl_kw = ecl_kw_alloc_empty();
  while(  ecl_kw_fread_realloc(ecl_kw , fmt_file , fortio) ) 
    ecl_kw_summarize(ecl_kw);
  printf("-----------------------------------------------------------------\n");

  ecl_kw_free(ecl_kw);
  fortio_fclose(fortio);
}


int main (int argc , char **argv) {
  int i;
  for (i = 1; i < argc; i++)
    kw_list(argv[i]);
  return 0;
}
