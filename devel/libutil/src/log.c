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
#include <pthread.h>
#include <stdarg.h>



struct log_struct {
  char             * filename;
  FILE             * stream; 
  int                log_level;
  pthread_mutex_t    mutex;
};



void log_set_file(log_type *logh , const char *filename) {
  logh->filename = util_realloc_string_copy( logh->filename , filename );
}



static log_type *log_alloc_internal(const char *filename , bool new, int log_level) {
  log_type   *logh;
  logh = util_malloc(sizeof *logh , __func__ );
  
  logh->log_level     = log_level;
  logh->filename      = NULL;
  log_set_file( logh ,filename );
  pthread_mutex_init( &logh->mutex , NULL );
  
  
  if (util_string_equal(filename,"<stdout>"))
    logh->stream = stdout;
  else if (util_string_equal(filename , "<stderr>"))
    logh->stream = stderr;
  else {
    if (new)
      logh->stream = util_fopen(filename , "w");
    else {
      logh->stream = util_fopen(filename , "a");
      log_add_message(logh , "========================= Starting new log =========================" , logh->log_level , false );
    }
  }
  return logh;
}


log_type * log_alloc_new(const char *filename, int log_level) {
  log_type *logh = log_alloc_internal(filename , true , log_level);
  return logh;
}



log_type *log_alloc_existing(const char *filename, int log_level) {
  return log_alloc_internal(filename , false , log_level);
}



static bool log_include_message(const log_type *logh, int message_level) {
  if (message_level > logh->log_level)
    return false;
  else
    return true;
}



void log_add_message(log_type *logh, char* message, int message_level , bool free_message) {
  pthread_mutex_lock( &logh->mutex );
  {
    struct tm time_fields;
    time_t    epoch_time;
  
    if (log_include_message(logh,message_level)) {
      time(&epoch_time);
      localtime_r(&epoch_time , &time_fields);
      fprintf(logh->stream,"%02d/%02d - %02d:%02d:%02d  %s\n",time_fields.tm_mday, time_fields.tm_mon + 1, time_fields.tm_hour , time_fields.tm_min , time_fields.tm_sec , message);
      fsync( fileno(logh->stream) );
    }
  }
  pthread_mutex_unlock( &logh->mutex );
  if (free_message)
    free( message );
}


int log_get_level( const log_type * logh ) {
  return logh->log_level; 
}



void log_add_fmt_message(log_type * logh , int message_level , const char * fmt , ...) {
  if (log_include_message(logh,message_level)) {
    char * message;
    va_list ap;
    va_start(ap , fmt);
    message = util_alloc_sprintf_va( fmt , ap );
    log_add_message( logh , message , message_level , true);
  }
}



void log_close( log_type * logh ) {
  fclose( logh->stream );
  free( logh->filename );
  free( logh );
}
