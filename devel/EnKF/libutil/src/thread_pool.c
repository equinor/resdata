#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <thread_pool.h>

struct thread_pool_struct {
  int        pool_size;
  int        jobs_running;
  pthread_t *thread_list;
};





static void thread_pool_resize(thread_pool_type * pool, int new_size) {
  pool->pool_size   = new_size;
  pool->thread_list = realloc(pool->thread_list , new_size * sizeof * pool->thread_list);
}



void thread_pool_join(thread_pool_type * pool) {
  int i;
  for (i=0; i < pool->jobs_running; i++)
    pthread_join(pool->thread_list[i] , NULL);  /* Second argument: void **value_ptr */
  pool->jobs_running = 0;
}


thread_pool_type * thread_pool_alloc(int pool_size) {
  thread_pool_type * pool = malloc(sizeof *pool);
  pool->thread_list = NULL;
  thread_pool_resize(pool , pool_size);
  pool->jobs_running = 0;
  return pool;
}


void thread_pool_add_job(thread_pool_type * pool , 
			 void *(start_func) (void *) , void *arg) {
  
  if (pool->jobs_running == pool->pool_size) 
    thread_pool_join(pool);

  if (pthread_create(&pool->thread_list[pool->jobs_running] , NULL , start_func , arg) != 0) {
    fprintf(stderr,"%s: failed to add new job - aborting \n",__func__);
    abort();
  }
  pool->jobs_running++;
  
}
  
void thread_pool_free(thread_pool_type * pool) {
  free(pool->thread_list);
  free(pool);
}


