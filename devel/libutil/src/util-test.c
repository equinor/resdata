#include <stdlib.h>
#include <stdio.h>
#include <util.h>
#include <string.h>      
#include <path_fmt.h>
#include <stdarg.h>
#include <hash.h>
#include <unistd.h>
#include <thread_pool.h>
#include <stringlist.h>
#include <menu.h>
#include <subst.h>
#include <arg_pack.h>
#include <vector.h>



int main(int argc , char ** argv) {
  int buffer_size = 256;
  
  vector_type * vector = vector_alloc_new();
  void * buffer = util_malloc( buffer_size , __func__);
  vector_append_buffer( vector , buffer , buffer_size);
  vector_append_buffer( vector , buffer , buffer_size);
  vector_append_buffer( vector , buffer , buffer_size);
  vector_append_owned_ref( vector , buffer , free );

  vector_free( vector );
}
