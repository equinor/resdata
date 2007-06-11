#ifndef __UTIL_H__
#define __UTIL_H__
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

bool 	     util_file_exists(const char *);
bool 	     util_path_exists(const char *);
bool 	     util_fmt_bit8   (const char *, int );
void 	     util_make_path  (const char *);
const char * util_newest_file(const char *, const char *);
double       util_file_difftime(const char * , const char *);
bool         util_file_update_required(const char *, const char *);
int          util_file_size(const char *);
void         util_unlink_path(const char *);
void         util_unlink_existing(const char *filename);
void util_copy_stream(FILE *, FILE *, int , void * );

FILE   * util_fopen(const char *  , const char *);
char   * util_realloc_full_path(char * , const char *, const char *);
char   * util_alloc_full_path(const char *, const char *);
char   * util_alloc_strip_copy(const char *);
void     util_set_strip_copy(char * , const char *);
char   * util_alloc_string_copy(const char *);
void     util_split_string(const char *, const char *, int *, char ***);
char   * util_realloc_string_copy(char * , const char *);
char   * util_realloc_substring_copy(char * , const char *, int );
char   * util_alloc_dequoted_string(char *);
void     util_free_string_list(char **, int );
char  ** util_alloc_string_list(int , int );
char   * util_alloc_substring_copy(const char *, int );

bool     util_intptr_2bool(const int *);
char *   util_alloc_cstring(const char *, const int *);
char *   util_alloc_string_copy(const char *);
void     util_enkf_unlink_ensfiles(const char *, const char *, int , bool );

void    util_abort(const char *, const char *, int , const char *);

void 	util_double_to_float(float  * , const double * , int );
void 	util_float_to_double(double * , const float  * , int );

void    util_fwrite_string(const char * , FILE *);
char *  util_fread_alloc_string(FILE *);
void    util_fskip_string(FILE *stream);
void    util_endian_flip_vector(void *, int , int );
bool    util_proc_alive(pid_t pid);

float  	 util_float_max (float   , float );
int    	 util_int_max   (int     , int);
double 	 util_double_max(double  , double );
float  	 util_float_min (float   , float );
int    	 util_int_min   (int     , int);
double 	 util_double_min(double  , double );

#define UTIL_ABORT(msg) (util_abort(__func__, __FILE__, __LINE__ , msg))
#endif
