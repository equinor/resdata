#ifndef __LOG_H__
#define __LOG_H__

#include <stdbool.h>


typedef struct log_struct log_type;

FILE       * log_get_stream(log_type * logh );
const char * log_get_filename( log_type * logh );
void         log_reset_filename( log_type * logh , const char * filename );
void         log_set_file(log_type * , const char *);
log_type   * log_alloc_new(const char *filename, int log_level);
log_type   * log_alloc_existing(const char *filename, int log_level);
void         log_add_message(log_type *logh, int message_level , char* message, bool free_message);
void         log_add_fmt_message(log_type * logh , int message_level , const char * fmt , ...);
int          log_get_level( const log_type * logh);
void         log_set_level( log_type * logh , int new_level);
void         log_close( log_type * logh );
inline  void log_sync(log_type * logh);

#endif
