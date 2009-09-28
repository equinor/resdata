#define  _GNU_SOURCE   /* Must define this to get access to pthread_rwlock_t */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <thread_pool2.h>
#include <util.h>

/**
   Ts file implements a small thread_pool object based on
   pthread_create() function calls. The characetristics of this
   implementation is as follows:

    1. The jobs are mangaged by a sperate thread - dispatch_thread.
    2. The new jobs are just appended to the queue, the dispatch_thread
       sees them in the queue and dispatches them. 
*/


typedef void * (start_func_ftype) (void *) ;

/**
   Internal struct which is used as queue node.
*/
typedef struct {
  thread_pool_type * pool;                /* A back-reference to the thread_pool holding the queue. */
  int                internal_index;      /* The index in the space [0,max_running) of the job slot where this job is running. */
  void             * func_arg;            /* The arguments to this job - supplied by the calling scope. */   
  start_func_ftype * func;                /* The function to call - supplied by the calling scope. */
} thread_pool_arg_type;




struct thread_pool_struct {
  thread_pool_arg_type  * queue;           
  int                     queue_index;
  int                     queue_size;
  int                     queue_alloc_size; 
  
  
  int                     submitted_jobs;  /* Should be atomic */
  int                     completed_jobs;  /* Should be atomic */
  int                     max_running;
  bool                    join; 
  bool                    accepting_jobs; 
  bool                  * thread_running;
  pthread_t             * thread_list;
  int                   * run_count;
  pthread_t               dispatch_thread;
  pthread_rwlock_t        queue_lock;
};



/**
   This function will grow the queue. It is called by the main thread
   (i.e. the context of the calling scope), and the queue is read by
   the dispatch_thread - i.e. access to the queue must be protected by
   rwlock.
*/

static void thread_pool_resize_queue( thread_pool_type * pool, int queue_length ) {
  pthread_rwlock_wrlock( &pool->queue_lock );
  {
    pool->queue            = util_realloc( pool->queue , queue_length * sizeof * pool->queue , __func__);
    pool->queue_alloc_size = queue_length;
  }
  pthread_rwlock_unlock( &pool->queue_lock );
}



/**
   The pthread_create() call which this is all about, does not start
   the user supplied function. Instead it will start an instance of
   this function, which will do some housekeeping before calling the
   user supplied function.

   
*/

static void * thread_pool_start_job( void * arg ) {
  thread_pool_arg_type * tp_arg = (thread_pool_arg_type * ) arg;
  thread_pool_type * tp         =  tp_arg->pool;
  int internal_index            =  tp_arg->internal_index;
  void * func_arg               =  tp_arg->func_arg;
  start_func_ftype * func       =  tp_arg->func;
  
  func( func_arg );                                /* Starting the real external function */
  tp->thread_running[ internal_index ] = false;    /* We mark the job as completed. */
  tp->completed_jobs++;
  free( arg );
  return NULL;
}



/**
   This function is run by the dispatch_thread. The thread will keep
   an eye on the queue, and dispatch new jobs when there are free
   slots available.
*/


static void * thread_pool_main_loop( void * arg ) {
  thread_pool_type * tp = (thread_pool_type *) arg;
  {
    const int usleep_busy = 1000;  /* 1/100 second */
    const int usleep_init = 1000;
    int internal_offset   = 0;
    do {
      if (tp->queue_size > tp->queue_index) {
        /* 
           There are jobs in the queue which would like to run - 
           let us see if we can find a slot for them.
        */
        int counter     = 0;
        bool slot_found = false;
        do {
          int internal_index = (counter + internal_offset) % tp->max_running;
          if (!tp->thread_running[internal_index]) {
            /* OK thread[internal_index] is ready to take this job.*/
            thread_pool_arg_type * tp_arg;

            pthread_rwlock_rdlock( &tp->queue_lock );
            tp_arg = util_alloc_copy( &tp->queue[ tp->queue_index ] , sizeof * tp_arg , __func__);
            pthread_rwlock_unlock( &tp->queue_lock );            

            tp_arg->internal_index = internal_index;

            tp->thread_running[internal_index] = true;
            pthread_create( &tp->thread_list[ internal_index ] , NULL , thread_pool_start_job , tp_arg );
            tp->run_count[ internal_index ] += 1;
            tp->queue_index++;
            internal_offset += (counter + 1);
            slot_found = true;
          } else
            counter++;
        } while (!slot_found && (counter < tp->max_running));
        
        if (!slot_found)
          usleep( usleep_busy );  /* There are no available job slots. */
      } else
        usleep( usleep_init ); /* There are no jobs wanting to run. */
    } while ((tp->join == false) || (tp->submitted_jobs > tp->completed_jobs));
  }

  /* 
     There are no more jobs in the queue, and the main scope has
     signaled that join should start.
  */
  {
    int i;
    for (i=0; i < tp->max_running; i++) {
      if (tp->run_count[i] > 0)
        pthread_join( tp->thread_list[i] , NULL );
    }
  }
  /* When we are here all the jobs have completed. */
  return NULL;
}


/**
   This function initializes a couple of counters, and starts up the
   dispatch thread.
*/
void thread_pool_restart( thread_pool_type * tp ) {
  tp->join           = false;
  tp->queue_index    = 0;
  tp->queue_size     = 0;
  tp->submitted_jobs = 0;
  tp->completed_jobs = 0;
  {
    int i;
    for (i=0; i < tp->max_running; i++) {
      tp->run_count[i] = 0;
      tp->thread_running[i] = 0;
    }
  }
  /* Starting the main thread. */
  pthread_create( &tp->dispatch_thread , NULL , thread_pool_main_loop , tp );
  tp->accepting_jobs = true;
}



/**
   This function is called by the calling scope when all the jobs have
   been submitted, and we just wait for them to complete.

   This function just sets the join switch to true - this tells the
   dispatch_thread to start the join process on the worker threads.
*/

void thread_pool_join(thread_pool_type * pool) {
  pool->join = true;                          /* Signals to the main thread that joining can start. */
  if (pool->max_running > 0) {
    pthread_join( pool->dispatch_thread , NULL ); /* Wait for the main thread to complete. */
    pool->accepting_jobs = false;
  }
}



/**
   max_running is the maximum number of concurrent threads.
*/

thread_pool_type * thread_pool_alloc(int max_running) {
  thread_pool_type * pool = util_malloc(sizeof *pool , __func__);
  pool->thread_running  = util_malloc( max_running * sizeof * pool->thread_running , __func__);
  pool->thread_list     = util_malloc( max_running * sizeof * pool->thread_list    , __func__);
  pool->run_count       = util_malloc( max_running * sizeof * pool->run_count    , __func__);
  pool->max_running     = max_running;
  pool->queue           = NULL;
  pthread_rwlock_init( &pool->queue_lock , NULL);
  thread_pool_resize_queue( pool  , 32 );  
  thread_pool_restart( pool );
  return pool;
}



void thread_pool_add_job(thread_pool_type * pool , start_func_ftype * start_func , void * func_arg ) {
  if (pool->max_running == 0) /* Blocking non-threaded mode: */
    start_func( func_arg );
  else {
    if (pool->accepting_jobs) {
      if (pool->queue_size == pool->queue_alloc_size)
        thread_pool_resize_queue( pool , pool->queue_alloc_size * 2);

      /* 
         The new job is added to the queue - the main thread is watching
         the queue and will pick up the new job.
      */
      pool->submitted_jobs++;
      
      pool->queue[pool->queue_size].pool     = pool;
      pool->queue[pool->queue_size].func_arg = func_arg;
      pool->queue[pool->queue_size].func     = start_func;

      pool->queue_size++;
    } else
      util_abort("%s: thread_pool is not running - restart with thread_pool_restart()?? \n",__func__);
  }
}
			 

  
void thread_pool_free(thread_pool_type * pool) {
  util_safe_free( pool->thread_running );
  util_safe_free( pool->thread_list );
  util_safe_free( pool->run_count );
  util_safe_free( pool->queue );
  free(pool);
}


