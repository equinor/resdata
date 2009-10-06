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
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>

/**
   This file implements a simple log structure. The characteristics of
   this object are as follows:

*/


struct log_struct {
  char             * filename;
  FILE             * stream; 
  int                fd; 
  int                log_level;
  pthread_mutex_t    mutex;
};



void log_reset_filename(log_type *logh , const char *filename) {
  if (logh->stream != NULL)  { /* Close the existing file descriptor. */
    size_t file_size;
    fclose( logh->stream );
    file_size = util_file_size( logh->filename );
    if (file_size == 0)
      unlink( logh->filename ); /* Unlink the old log file if it had zero size. */ 
  }
  
  logh->filename = util_realloc_string_copy( logh->filename , filename );
  pthread_mutex_lock( &logh->mutex );

  logh->stream = util_mkdir_fopen( filename , "a+");
  logh->fd     = fileno( logh->stream );
  
  pthread_mutex_unlock( &logh->mutex );
  
}





static log_type *log_alloc_internal(const char *filename , bool new, int log_level) {
  log_type   *logh;
  logh = util_malloc(sizeof *logh , __func__ );
  
  logh->log_level     = log_level;
  logh->filename      = NULL;
  logh->stream         = NULL;
  pthread_mutex_init( &logh->mutex , NULL );
  log_reset_filename( logh ,filename );
    
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


/**
   If dup_stream != NULL the message (without the date/time header) is duplicated on this stream.
*/
void log_add_message(log_type *logh, int message_level , FILE * dup_stream , char* message, bool free_message) {
  pthread_mutex_lock( &logh->mutex );
  {
    struct tm time_fields;
    time_t    epoch_time;
  
    if (log_include_message(logh,message_level)) {
      time(&epoch_time);
      localtime_r(&epoch_time , &time_fields);

      if (message != NULL)
        fprintf(logh->stream,"%02d/%02d - %02d:%02d:%02d  %s\n",time_fields.tm_mday, time_fields.tm_mon + 1, time_fields.tm_hour , time_fields.tm_min , time_fields.tm_sec , message);
      else
        fprintf(logh->stream,"%02d/%02d - %02d:%02d:%02d   \n",time_fields.tm_mday, time_fields.tm_mon + 1, time_fields.tm_hour , time_fields.tm_min , time_fields.tm_sec);

      /** We duplicate the message to the stream 'dup_stream'. */
      if ((dup_stream != NULL) && (message != NULL))
        fprintf(dup_stream , "%s\n", message);
      
      log_sync( logh );
    }
  }
  pthread_mutex_unlock( &logh->mutex );
  if (free_message)
    free( message );
}




int log_get_level( const log_type * logh ) {
  return logh->log_level; 
}


void log_set_level( log_type * logh , int new_level) {
  logh->log_level = new_level;
}



void log_add_fmt_message(log_type * logh , int message_level , FILE * dup_stream , const char * fmt , ...) {
  if (log_include_message(logh,message_level)) {
    char * message;
    va_list ap;
    va_start(ap , fmt);
    message = util_alloc_sprintf_va( fmt , ap );
    log_add_message( logh , message_level , dup_stream , message , true);
  }
}






/**
   This function can be used to get low level to the FILE pointer of
   the stream. To 'ensure' that the data actually hits the disk
   you should call log_sync() after writing.

   It is your responsabiulity to avoid racing++ when using the
   log_get_stream() function.
*/

FILE * log_get_stream(log_type * logh ) {
  return logh->stream;
}


inline void log_sync(log_type * logh) {
  fsync( logh->fd );
  fseek( logh->stream , 0 , SEEK_END );
}


const char * log_get_filename( log_type * logh ) {
  return logh->filename;
}


void log_close( log_type * logh ) {
  if ((logh->stream != stdout) && (logh->stream != stderr))
    fclose( logh->stream );  /* This closes BOTH the FILE * stream and the integer file descriptor. */
  
  free( logh->filename );
  free( logh );
}
