#ifndef __LOG_H__
#define __LOG_H__

#include <stdbool.h>


typedef struct log_struct log_type;

void         log_set_file(log_type * , const char *);
log_type   * log_alloc_new(const char *filename, int log_level);
log_type   * log_alloc_existing(const char *filename, int log_level);
void         log_add_message(log_type *logh, int message_level , char* message, bool free_message);
void         log_add_fmt_message(log_type * logh , int message_level , const char * fmt , ...);
int          log_get_level( const log_type * logh);
void         log_close( log_type * logh );
#endif
