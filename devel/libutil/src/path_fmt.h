#ifndef __PATH_FMT_H__
#define __PATH_FMT_H__
#include <stdarg.h>
#include <stdbool.h>

typedef struct path_fmt_struct path_fmt_type;


path_fmt_type * path_fmt_alloc_directory_fmt(const char * );
path_fmt_type * path_fmt_alloc_file_fmt(const char * );
path_fmt_type * path_fmt_copyc(const path_fmt_type *);
/*char          * path_fmt_alloc_path_va(const path_fmt_type * , bool , va_list );*/
char          * path_fmt_alloc_path(const path_fmt_type * , bool , ...);
char          * path_fmt_alloc_file(const path_fmt_type * , bool , ...);
void 		path_fmt_free(path_fmt_type * );
const char    * path_fmt_get_fmt(const path_fmt_type * );
void            path_fmt_reset_fmt(path_fmt_type * , const char * );
void            path_fmt_make_path(const path_fmt_type * );

#endif
