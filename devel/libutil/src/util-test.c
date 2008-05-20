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


void * thread_sleep(void * arg) {
  sleep(1);
  return NULL;
}

int main(int argc , char ** argv) {
  const int N = 10;
  int i;
  thread_pool_type * tp = thread_pool_alloc(N);
  for (i=0; i < 10*N; i++)
    thread_pool_add_job(tp , thread_sleep , NULL);
  
  thread_pool_join(tp);
  thread_pool_free(tp);
}
