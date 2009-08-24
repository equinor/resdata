#include <string.h> 
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <util.h>
#include <log.h>


#define LOG_2FILE   0
#define LOG_2STDOUT 1
#define LOG_2STDERR 2


struct log_struct {
  char *filename;
  int   log_level;
  int   log_mode;  /* 0:file   1:stdout   2:stderr */
  char *auto_message;
  int   auto_phase;
  int   auto_fraction;
};


void log_set_file(log_type *logh , const char *filename) {
  if (logh->filename == NULL) 
    logh->filename = calloc(1 + strlen(filename) , sizeof(char));

  if (strlen(logh->filename) <= strlen(filename)) 
    logh->filename = realloc(logh->filename , (1 + strlen(filename)) * sizeof(char));
  
  strcpy(logh->filename , filename);
}


log_type *log_alloc_empty(int log_level) {
  log_type   *logh;
  logh = malloc(sizeof(log_type));
  
  logh->log_level     = log_level;
  logh->auto_phase    = 0;
  logh->auto_fraction = -1;
  logh->auto_message  = NULL;
  logh->filename      = NULL;

  return logh;
}


void log_init_new(log_type *logh) {
  struct stat stat_buf;
  if (stat(logh->filename , &stat_buf) == 0) {
    if ((stat_buf.st_mode && S_IWUSR) == 0) {
      fprintf(stderr,"File: %s is not writable by owner - aborting \n",logh->filename);
      exit(1);
    }
    unlink(logh->filename);
  } else {
    FILE *stream = fopen(logh->filename,"w");
    if (stream == NULL) {
      fprintf(stderr,"Failed to open: %s for writing - aborting \n",logh->filename);
      exit(1);
    }
    fclose(stream);
  }
  /*
    log_add_message(logh , "========================= Starting new log =========================" , logh->log_level);
  */
}


log_type *log_alloc_internal(char *filename , bool new, int log_level) {
  log_type   *logh = log_alloc_empty(log_level);
  
  
  
  if (filename != NULL) {
    if (strcmp(filename,"<stdout>") == 0)
      logh->log_mode = 1;
    else if (strcmp(filename,"<stderr>") == 0)
      logh->log_mode = 2;
    else {
      logh->log_mode = 0;
      logh->filename = NULL;
      log_set_file(logh , filename);
      log_init_new(logh);
    }
  } else 
    logh->filename = NULL;
  
  return logh;
}


log_type *log_alloc_new(char *filename, int log_level) {
  log_type *logh = log_alloc_internal(filename,1,log_level);
  return logh;
}


log_type *log_alloc_existing(char *filename, int log_level) {
  return log_alloc_internal(filename,0,log_level);
}

static bool log_include_message(const log_type *logh, int message_level) {
  if (message_level > logh->log_level)
    return false;
  else
    return true;
}

static bool log_2file(const log_type *logh) {
  return (logh->log_mode == LOG_2FILE);
}

static bool log_2stdout(const log_type *logh) {
  return (logh->log_mode == LOG_2STDOUT);
}

static bool log_2stderr(const log_type *logh) {
  return (logh->log_mode == LOG_2STDERR);
}


void log_add_message(const log_type *logh, const char* message, int message_level) {
  FILE     *stream = NULL;
  struct tm time_fields;
  time_t    epoch_time;
  
  if (logh != NULL) {
    if (log_include_message(logh,message_level)) {
      if (log_2stdout(logh))
	stream = stdout;
      else if (log_2stderr(logh))
	stream = stderr;
      else {
	if (logh->filename != NULL) 
	  stream = util_fopen(logh->filename , "a");
	else {
	  fprintf(stderr, "Trying to log to a log object without filename - aborting \n");
	  abort();
	}
      }
      
      if (stream != NULL) {
	time(&epoch_time);
	localtime_r(&epoch_time , &time_fields);
	fprintf(stream,"%02d/%02d - %02d:%02d:%02d  %s\n",time_fields.tm_mday, time_fields.tm_mon + 1, time_fields.tm_hour , time_fields.tm_min , time_fields.tm_sec , message);
	if (log_2file(logh))
	  fclose(stream);
	else
	  fflush(stream);
      }
    }
  }
}


void log_update(log_type *logh) {
  if (logh != NULL) {
    if (logh->auto_fraction != 0) {
      if (logh->auto_phase == logh->auto_fraction) {
	log_add_message(logh , logh->auto_message , logh->log_level);
	logh->auto_phase = 0;
      } else 
	logh->auto_phase++;
    } else {
      fprintf(stderr,"log_auto_update(): Must call log_set_auto() prior to log_auto_update() \n");
      abort();
    }
  }
}


void log_set_auto(log_type *logh , const char *message , int log_fraction) {
  logh->auto_message = realloc(logh->auto_message , (1 + strlen(message)) * sizeof(char));
  strcpy(logh->auto_message , message);

  logh->auto_fraction = log_fraction;
}
