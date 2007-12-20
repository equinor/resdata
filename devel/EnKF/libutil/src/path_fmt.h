#ifndef __PATH_FMT_H__
#define __PATH_FMT_H__
#include <stdarg.h>

typedef struct path_fmt_struct path_fmt_type;


path_fmt_type * path_fmt_alloc(const char * );
path_fmt_type * path_fmt_copyc(const path_fmt_type *);
void 		path_fmt_set(path_fmt_type *  , ...);
void            path_fmt_set_va(path_fmt_type *  , va_list );
void 		path_fmt_free(path_fmt_type * );
const char    * path_fmt_get_path(const path_fmt_type * );
const char    * path_fmt_get_fmt(const path_fmt_type * );
void            path_fmt_reset_fmt(path_fmt_type * , const char * );
void            path_fmt_make_path(const path_fmt_type * );

#endif
