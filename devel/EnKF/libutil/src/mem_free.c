#include <stdlib.h>
#include <util.h>
#include <msg.h>
#include <string.h>
#include <stdbool.h>



int main(void) {

  while (true) {
    printf("Ledig minne: %8d kB \n",util_proc_mem_free());
    sleep(1);
  } 

  return 0;
}
