#include <stdlib.h>
#include <stdio.h>
#include <util.h>
#include <void_arg.h>
#include <string.h>      
#include <path_fmt.h>
#include <stdarg.h>
#include <hash.h>
#include <unistd.h>
#include <thread_pool.h>
#include <config.h> 


int main(int argc , char ** argv) {
  config_type * config = config_alloc( false );
  config_init_item(config , "GRID" , 0 , NULL , true , false , 0 , NULL , -1, -1 , NULL);
  config_init_item(config , "INIT" , 1 , (const char *[1]) {"DEFAULT/init"} , true , false , 0 , NULL , -1 , -1 , NULL);
  config_parse(config , "config.txt" , "--");

  printf("GRID: %s \n",config_get(config , "GRID"));
  printf("INIT: %s \n",config_get(config , "INIT"));
  config_free(config);
}
