#include <stdlib.h>
#include <stdio.h>
#include <util.h>
#include <void_arg.h>
#include <string.h>      
#include <path_fmt.h>
#include <stdarg.h>
#include <hash.h>

int main(int argc , char ** argv) {
  const int size = 2000000;
  long * data;
  long * copy;
  int i;

  data = util_malloc(size * sizeof * data , __func__);
  copy = util_malloc(size * sizeof * copy , __func__);
  for (i=0; i < size; i++) {
    data[i] = random();
    copy[i] = data[i];
  }

  {
    FILE * stream = util_fopen("/tmp/compress.gz" , "w");
    util_fwrite_compressed(data , size * sizeof * data , stream);
    fclose(stream);
  }
  for (i=0; i < size; i++) 
    data[i] = random();

  {
    FILE * stream = util_fopen("/tmp/compress.gz" , "r");
    util_fread_compressed(data , stream);
    fclose(stream);
  }
  
  for (i=0; i < size; i++) 
    if (data[i] != copy[i]) 
      printf("ERROR \n");
  
  printf("Check OK \n");
}
