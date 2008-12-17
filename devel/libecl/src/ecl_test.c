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
  const char * filename = "test.GRDECL";
  FILE * stream = util_fopen(filename , "r");
  ecl_kw_grdecl_fseek_kw("PORO" , true , false ,stream , filename);
  ecl_kw_grdecl_fseek_kw("PERMX" , true , false ,stream , filename);
  ecl_kw_grdecl_fseek_kw("PERMZ" , true , false ,stream , filename);
  ecl_kw_grdecl_fseek_kw("GRIDHEAD" , true , false ,stream , filename);
  fclose(stream);
}
