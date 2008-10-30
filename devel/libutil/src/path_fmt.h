#ifndef __PATH_FMT_H__
#define __PATH_FMT_H__
#ifdef __cplusplus
extern "C" {
#endif
#include <stdarg.h>
#include <stdbool.h>
#include <node_ctype.h>

typedef struct path_fmt_struct path_fmt_type;


path_fmt_type * path_fmt_safe_cast(const void * arg);
path_fmt_type * path_fmt_alloc_directory_fmt(const char * );
path_fmt_type * path_fmt_alloc_path_fmt(const char * );
path_fmt_type * path_fmt_copyc(const path_fmt_type *);
path_fmt_type * path_fmt_scanf_alloc(const char *  , int , const node_ctype * , bool );
char          * path_fmt_alloc_path(const path_fmt_type * , bool , ...);
char          * path_fmt_alloc_file(const path_fmt_type * , bool , ...);
void 		path_fmt_free(path_fmt_type * );
const char    * path_fmt_get_fmt(const path_fmt_type * );
void            path_fmt_reset_fmt(path_fmt_type * , const char * );
void            path_fmt_make_path(const path_fmt_type * );

#ifdef __cplusplus
}
#endif
#endif
