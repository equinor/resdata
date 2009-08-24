#ifndef __LOG_H__
#define __LOG_H__

#include <stdbool.h>


typedef struct log_struct log_type;

void        log_init_new(log_type *);
log_type   *log_alloc_empty(int);
void        log_set_file(log_type * , const char *);
log_type   *log_alloc_internal(char *filename , bool new, int log_level);
log_type   *log_alloc_new(char *filename, int log_level);
log_type   *log_alloc_existing(char *filename, int log_level);
void        log_add_message(const log_type *logh, const char* message, int message_level);
void        log_update(log_type *logh);
void        log_set_auto(log_type *logh, const char *message , int log_fraction);

#endif
