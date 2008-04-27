#ifndef __UTIL_H__
#define __UTIL_H__
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <hash.h>

#define UTIL_PATH_SEP_STRING "/"   /* A \0 terminated separator used when we want a (char *) instance.                   */
#define UTIL_PATH_SEP_CHAR   '/'   /* A simple character used when we want an actual char instance (i.e. not a pointer). */

#define UTIL_NEWLINE_STRING "\n"

typedef enum {util_filter_warn0 , util_filter_warn_unused = 1, util_filter_warn_unknown = 2 , util_filter_warn_all = 3} util_filter_warn_type;


time_t       util_make_time2(int , int , int , int , int , int );
time_t       util_make_time1(int , int , int);

bool         util_fscanf_date(FILE * , time_t *);
bool         util_sscanf_date(const char * , time_t *);

bool         util_is_executable(const char * );
bool 	     util_file_exists(const char *);
int          util_get_path_length(const char * );
bool 	     util_path_exists(const char *);
bool         util_is_abs_path(const char * );
bool 	     util_fmt_bit8   (const char *);
bool         util_fmt_bit8_stream(FILE * );
void 	     util_make_path  (const char *);
const char * util_newest_file(const char *, const char *);
double       util_file_difftime(const char * , const char *);
bool         util_file_update_required(const char *, const char *);
int          util_file_size(const char *);
void         util_unlink_path(const char *);
void         util_unlink_path_TESTING(const char *);
void         util_unlink_existing(const char *filename);
void         util_strupr(char *);
void 	     util_copy_stream(FILE *, FILE *, int , void * );
void 	     util_copy_file(const char * , const char * );
char       * util_alloc_cwd(void);

int      util_forward_line(FILE * , bool * );
void     util_rewind_line(FILE *);
int      util_count_content_file_lines(FILE * );
int      util_count_file_lines(FILE * );
FILE   * util_fopen(const char *  , const char *);
void     util_alloc_file_components(const char * , char ** , char **, char **);
char   * util_realloc_full_path(char * , const char *, const char *);
char   * util_alloc_tmp_file(const char * , const char * , bool );
char   * util_fscanf_alloc_line(FILE *, bool *);
char   * util_fscanf_realloc_line(FILE *, bool * , char *);
char   * util_fscanf_alloc_token(FILE * );
void     util_fskip_token(FILE * );
void     util_fskip_chars(FILE * , const char * , bool *);
void     util_fskip_cchars(FILE * , const char * , bool *);
bool     util_fscanf_int(FILE * , int * );
bool     util_sscanf_int(const char * , int * );
char   * util_alloc_full_path(const char *, const char *);
char   * util_alloc_strip_copy(const char *);
char   * util_realloc_strip_copy(char *);
void     util_set_strip_copy(char * , const char *);
char   * util_alloc_string_sum(const char **  , int);
char   * util_strcat_realloc(char *, const char * );
char   * util_alloc_string_copy(const char *);
char  ** util_alloc_stringlist_copy(const char **, int );
void     util_split_string(const char *, const char *, int *, char ***);
char   * util_alloc_joined_string(const char **  , int , const char * );
char   * util_alloc_multiline_string(const char ** , int );
char   * util_realloc_string_copy(char * , const char *);
char   * util_realloc_substring_copy(char * , const char *, int );
char   * util_alloc_string_sum2(const char *, const char *);
char   * util_alloc_dequoted_string(char *);
void     util_free_string_list(char **, int );
char  ** util_alloc_string_list(int , int );
char   * util_alloc_substring_copy(const char *, int );
bool     util_is_directory(const char * );
bool     util_is_link(const char * );
void     util_make_slink(const char *, const char * );
void 	 util_set_datetime_values(time_t , int * , int * , int * , int * , int *  , int *);
void 	 util_set_date_values(time_t , int * , int * , int * );


bool     util_intptr_2bool(const int *);
int      util_C2f90_bool(bool );
void     util_memcpy_string_C2f90(const char *  , char *  , int );
void     util_pad_f90string(char * , int , int );
char *   util_alloc_cstring(const char *, const int *);
char *   util_alloc_string_copy(const char *);
void     util_enkf_unlink_ensfiles(const char *, const char *, int , bool );

void    util_abort(const char * fmt , ...);
void *  util_realloc(void *  , size_t  , const char * );
void *  util_malloc(size_t , const char * );
void 	util_double_to_float(float  * , const double * , int );
void 	util_float_to_double(double * , const float  * , int );

int 	util_get_month_nr(const char * );
int 	util_check_month(const char * );

void    util_filter_file(const char * , const char * , const char * , char , char , const hash_type * , util_filter_warn_type);
char *  util_fread_alloc_file_content(const char * , const char * , int *);
void    util_fwrite_string(const char * , FILE *);
char *  util_fread_realloc_string(char *  , FILE *);
char *  util_fread_alloc_string(FILE *);
void    util_fskip_string(FILE *stream);
void    util_endian_flip_vector(void *, int , int );
bool    util_proc_alive(pid_t pid);
int     util_proc_mem_free(void);


void     util_apply_int_limits(int * , int , int );
void     util_apply_float_limits(float * , float , float );
void     util_apply_double_limits(double * , double , double );
void     util_update_double_max_min(double , double * , double * );
void     util_update_float_max_min(float , float * , float * );
void     util_update_int_max_min(int , int * , int * );
float  	 util_float_max (float   , float );
int    	 util_int_max   (int     , int);
double 	 util_double_max(double  , double );
float  	 util_float_min (float   , float );
int    	 util_int_min   (int     , int);
double 	 util_double_min(double  , double );
void     util_fskip_lines(FILE * , int);
bool     util_same_file(const char *  , const char * );
void     util_read_path(const char * , int , bool , char *  );
void     util_read_filename(const char * , int , bool , char * );
void     util_read_string(const char *  , int  , char * );
void     util_fread (void *, size_t , size_t , FILE * , const char * );
void     util_fwrite(const void *, size_t , size_t , FILE * , const char * );
int      util_fread_int(FILE * );
void 	 util_fwrite_int   (int    , FILE * );
void 	 util_fwrite_double(double , FILE * );
void 	 util_fwrite_int_vector   (const int     * , int , FILE * , const char * );
void 	 util_fwrite_double_vector(const double  * , int , FILE * , const char * );

void 	 util_compress_buffer(const void * , int , void * , unsigned long * );
void 	 util_fread_compressed(char * , FILE * );
void 	 util_fwrite_compressed(const void * , int , FILE * );
void     util_block_growing_file(const char * );
void     util_block_growing_directory(const char * );

#define UTIL_FWRITE_SCALAR(s,stream) fwrite(&s , sizeof s , 1 , stream)
#define UTIL_FREAD_SCALAR(s,stream)  fread(&s , sizeof s , 1 , stream)

#define UTIL_FWRITE_VECTOR(s,n,stream) fwrite(s , sizeof s , (n) , stream)
#define UTIL_FREAD_VECTOR(s,n,stream)  fread(s , sizeof s , (n) , stream)


#endif
