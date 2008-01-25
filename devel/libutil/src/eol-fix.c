#include <stdlib.h>
#include <stdio.h>
#include <util.h>



void eol_fix_file(const char * filename) {
  char * tmp_file = util_alloc_tmp_file("/tmp" , "eol-fix" , true);
  util_copy_file(filename , tmp_file);
  {
    FILE * src    = util_fopen(tmp_file , "r");
    FILE * target = util_fopen(filename , "w");

    do {
      int c = fgetc(src);
      if (c != '\r')
	fputc(c , target);
    } while (!feof(src));

    fclose(src);
    fclose(target);
  }
  free(tmp_file);
}



int main (int argc , char **argv) {
  eol_fix_file(argv[1]);
  return 0;
}
