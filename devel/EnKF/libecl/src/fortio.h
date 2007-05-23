#ifndef __FORTIO_H__
#define __FORTIO_H__

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

typedef struct fortio_struct fortio_type;

void          fortio_copy_record(fortio_type * , fortio_type * , int , void * , bool *);
fortio_type * fortio_open(const char *, const char * , bool);
void          fortio_close(fortio_type *);
int           fortio_init_read(fortio_type *);
void          fortio_complete_read(fortio_type *);
void          fortio_init_write(fortio_type * , int);
void          fortio_complete_write(fortio_type *);
void          fortio_skip_record(fortio_type *);
void          fortio_read_record(fortio_type * , char *buffer);
void          fortio_write_record(fortio_type * , const char *, int);
FILE        * fortio_get_FILE(const fortio_type *);
int           fortio_get_record_size(const fortio_type *);
bool          fortio_get_endian_flip(const fortio_type *);
void          fortio_rewind(const fortio_type *fortio);
const char  * fortio_filename_ref(const fortio_type * );
#endif
