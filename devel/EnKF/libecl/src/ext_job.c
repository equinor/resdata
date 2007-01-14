#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <util.h>
#include <pthread.h>
#include <unistd.h>
#include <ext_job.h>
#include <stdbool.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#define USE_MUTEX



struct ext_job_struct {
  char *run_path;
  char *run_cmd;
  char *abort_cmd;
  char *run_file;
  char *complete_file;

  int max_restart;
  int restart_nr;
  int sleep_time;
  bool do_fork;
  
  time_t           submit_time;
  time_t           start_time;
  time_t           complete_time;
  double           run_time_sec;

  pthread_mutex_t  mutex;
  ext_status_enum  status;
  ext_action_enum  action;
};


static char * alloc_string_copy(const char *src , bool false_OK) {
  if (src != NULL) {
    char *copy = calloc(strlen(src) + 1 , sizeof *copy);
    strcpy(copy , src);
    return copy;
  } else {
    if (false_OK)
      return NULL;
    else {
      fprintf(stderr,"%s: can not take NULL as input - aborting \n" , __func__);
      abort();
    }
  }

}


static char * alloc_path_copy(const char *p1 , const char *p2) {
  char *copy = calloc(strlen(p1) + strlen(p2) + 2 , sizeof *copy);
  strcpy(copy , p1);
  strcat(copy , "/");
  strcat(copy , p2);
  return copy;
}


static void ext_job_lock(ext_job_type *ext_job) {
#ifdef USE_MUTEX
  pthread_mutex_lock(&ext_job->mutex);
#endif
}

static void ext_job_unlock(ext_job_type *ext_job) {
#ifdef USE_MUTEX
  pthread_mutex_unlock(&ext_job->mutex);
#endif
}

static int ext_job_get_state_size(void) { return 6; }

ext_action_enum ext_job_get_action(ext_job_type *ext_job) {
  ext_action_enum action;
  ext_job_lock(ext_job);
  action = ext_job->action;
  ext_job_unlock(ext_job);
  return action;
}

void ext_job_set_action(ext_job_type *ext_job , ext_action_enum action) {
  ext_job_lock(ext_job);
  ext_job->action = action;
  ext_job_unlock(ext_job);
}

ext_status_enum ext_job_get_status(ext_job_type *ext_job) {
  ext_status_enum status;
  ext_job_lock(ext_job);
  status = ext_job->status;
  ext_job_unlock(ext_job);
  return status;
}


void ext_job_set_status(ext_job_type *ext_job , ext_status_enum status) {
  ext_job_lock(ext_job);
  ext_job->status = status;
  ext_job_unlock(ext_job);
}


ext_job_type * ext_job_alloc(const char *run_cmd , const char *abort_cmd , const char * run_path , const char *run_file , const char * complete_file , int max_restart , int sleep_time , bool do_fork) {
  ext_job_type * ext_job;
  ext_job = malloc(sizeof *ext_job);
  ext_job->run_path      = alloc_string_copy(run_path , false);
  ext_job->run_cmd       = alloc_string_copy(run_cmd  , false);
  ext_job->abort_cmd     = alloc_string_copy(abort_cmd , true);
  ext_job->run_file      = alloc_path_copy(run_path , run_file);
  ext_job->complete_file = alloc_path_copy(run_path , complete_file);
  ext_job->max_restart   = max_restart;
  ext_job->restart_nr    = 0;
  ext_job->action        = ext_action_null;
  ext_job->status        = ext_status_null;
  ext_job->sleep_time    = sleep_time;
  ext_job->do_fork       = do_fork;
  pthread_mutex_init(&ext_job->mutex , NULL);
  return ext_job;
}

static void ext_job_set_ctime(const char *file , time_t *ct) {
  struct stat buffer;
  int fildes;
  
  fildes = open(file , O_RDONLY);
  fstat(fildes, &buffer);
  *ct = buffer.st_ctime;
  close(fildes);
}



/*
  Bør skille mellom aktive avslutninger (KILL), 
  og kjøringer som mislykkes (FAIL)??
*/

void ext_job_check(ext_job_type *ext_job) {
  /*
    Here we react to the action 
  */
  char cmd[512];
  pid_t pid;
  ext_job_lock(ext_job);
  switch (ext_job->action) {
  case(ext_action_null):
    break;
  case(ext_action_submit):
    sprintf(cmd , "cd %s ; %s" , ext_job->run_path , ext_job->run_cmd);
    /* 
       This is a blocking call...
    */
    
    if (ext_job->do_fork) {
      pid = fork();
      if (pid == 0) {
	system(cmd);
	exit(1);
      }
    } else
      system(cmd);
    
    ext_job->status = ext_status_submitted;
    ext_job->action = ext_action_watch;
    time(&ext_job->submit_time);
    time(&ext_job->start_time); /* In case I never see the run_file. */
    break;
  case(ext_action_watch):
    if (ext_job->status == ext_status_submitted) {
      if (util_file_exists(ext_job->run_file)) {
	ext_job->status = ext_status_running;
	ext_job_set_ctime(ext_job->run_file , &ext_job->start_time);
      }
      
      if (util_file_exists(ext_job->complete_file)) {
	/* It completed without ever displaying the run_file - fair enough. */
	ext_job->status = ext_status_complete_OK;
	ext_job->action = ext_action_return;
	ext_job_set_ctime(ext_job->complete_file , &ext_job->complete_time);
      }
      
      /* 
	 If it completes, and fails within the sleep window, the
	 status will be stuck in ext_action_watch(). The run_file has
	 been there and gone, and the complete_file never shows up
	 (because the job failed), in this case the job must be
	 manually rescheduled - this is currently not done, 
	 and the job will appear to be lost.
      */

    } else if (ext_job->status == ext_status_running) {
      if (util_file_exists(ext_job->complete_file)) {
	ext_job->status = ext_status_complete_OK;
	ext_job->action = ext_action_return;
	ext_job_set_ctime(ext_job->complete_file , &ext_job->complete_time);
      } else if (!util_file_exists(ext_job->run_file)) {
	/* 
	   It has failed ... opbserve that restarting is handled without 
	   resorting to the ext_action_restart action, that particular
	   action is reserved for external restarts, and not governed
	   the max_restart property.
	*/
	if (ext_job->restart_nr < ext_job->max_restart) {
	  ext_job->status = ext_status_null;
	  ext_job->action = ext_action_null;
	  ext_job->restart_nr++;
	} else {
	  ext_job->status = ext_status_complete_fail;
	  ext_job->action = ext_action_return;
	}
      }
    }
    break;
  case(ext_action_restart):  /* This is for external restart */
  case(ext_action_kill):
    if (ext_job->abort_cmd != NULL) {
      sprintf(cmd , "cd %s ; %s" , ext_job->run_path , ext_job->abort_cmd);
      if (ext_job->do_fork) {
	pid = fork();
	if (pid == 0) {
	  system(cmd);
	  exit(1);
	}
      } else
	system(cmd);
    }
    if (ext_job->action == ext_action_restart) {
      ext_job->action = ext_action_null;
      ext_job->status = ext_status_null;
    } else {
      ext_job->status = ext_status_complete_kill;
      ext_job->action = ext_action_return;
    }
    break;
  case(ext_action_return):
    break;
  }
  ext_job_unlock(ext_job);
}



static void free_not_null(char *p) {
  if (p != NULL) free(p);
}

void ext_job_free(ext_job_type *ext_job) {
  free_not_null(ext_job->run_path);
  free_not_null(ext_job->run_cmd);
  free_not_null(ext_job->abort_cmd);
  free_not_null(ext_job->run_file);
  free_not_null(ext_job->complete_file);
  free(ext_job);
}


static void * ext_job_main(void *_ext_job) {
  ext_job_type *ext_job = (ext_job_type *) _ext_job;
  while (ext_job->action != ext_action_return) {
    ext_job_check(ext_job);
    sleep(ext_job->sleep_time);
  }
  return NULL;
}



void ext_job_summarize_list(int job_size , ext_job_type ** jobList , int *state_summary) {
  int job,i;
  for (i=0; i < ext_job_get_state_size(); i++)
    state_summary[i] = 0;
  
  for (job = 0; job < job_size; job++)
    state_summary[ext_job_get_status(jobList[job])]++;
}

/*
  All jobs are assumed to arrive with ext_action_null / ext_status_null ?
*/

void ext_job_run_pool(int job_size , ext_job_type **jobList , int max_running , int sleep_time) {
  int complete_jobs;
  int active_jobs;
  int job;
  pthread_t  *threadList;
  const int   state_size     = ext_job_get_state_size();
  int        *state_summary = calloc(state_size , sizeof *state_summary);
  
  threadList = calloc(job_size , sizeof *threadList);
  for (job = 0; job < job_size; job++) 
    pthread_create(&threadList[job] , NULL , ext_job_main , jobList[job]);
  
  do {
    sleep(sleep_time);
    ext_job_summarize_list(job_size , jobList , state_summary);
    printf("%d  %d  %d  (%d %d %d) \n",state_summary[0] , state_summary[1] , state_summary[2] , state_summary[3] , state_summary[4] , state_summary[5]);

    complete_jobs = state_summary[ext_status_complete_OK] + state_summary[ext_status_complete_fail] + state_summary[ext_status_complete_kill]; 
    active_jobs   = state_summary[ext_status_submitted] + state_summary[ext_status_running];
    printf("submitted:%d   Running:%d \n",state_summary[ext_status_submitted] , state_summary[ext_status_running]);
    if (active_jobs < max_running) {
      job         = 0;
      do {
	if (ext_job_get_status(jobList[job]) == ext_status_null) {
	  ext_job_set_action(jobList[job] , ext_action_submit);
	  active_jobs++;
	  printf("Submitter job:%d \n",job);
	}
	job++;
      } while (active_jobs < max_running && job < job_size);
    }
    printf("Complete: %d \n",complete_jobs);
  } while (complete_jobs < job_size);
  free(threadList);
  free(state_summary);
}
