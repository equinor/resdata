#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <ecl_kw.h>
#include <fortio.h>
#include <string.h>
#include <ecl_fstate.h>
#include <restart_kw_list.h>


void kw_cmp(const char *file1 , const char * file2) {
  bool at_eof;
  fortio_type *fortio1 = fortio_open(file1 , "r" , true);
  fortio_type *fortio2 = fortio_open(file2 , "r" , true);
  ecl_block_type * block1 = ecl_block_fread_alloc(0 , false , true , fortio1 , &at_eof);
  ecl_block_type * block2 = ecl_block_fread_alloc(0 , false , true , fortio2 , &at_eof);
  fortio_close(fortio1);
  fortio_close(fortio2);
  
  
  {
    ecl_kw_type *ecl_kw1 , *ecl_kw2;
    int index;
    ecl_kw1 = ecl_block_get_first_kw(block1);
    ecl_kw2 = ecl_block_get_first_kw(block2);

    while (ecl_kw1 != NULL && ecl_kw2 != NULL) {

      if (ecl_kw_equal(ecl_kw1 , ecl_kw2))
	printf("equal\n");
      else
	printf("different\n");

      ecl_kw1 = ecl_block_get_next_kw(block1);
      ecl_kw2 = ecl_block_get_next_kw(block2);
    }
  }    
}


int main (int argc , char **argv) {
  kw_cmp(argv[1] ,argv[2]);
  return 0;
}
