#include <stdlib.h>
#include <stringlist.h>
#include <hash.h>
#include <config.h>


int main(void) {
  const char * config_file = "config_test_input";
  config_type * config = config_alloc();
  
  config_free(config);
}
