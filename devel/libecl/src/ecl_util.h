#ifndef __ECL_UTIL_H__
#define __ECL_UTIL_H__
#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>
#include <time.h>

typedef enum {ecl_other_file           = 0   , 
	      ecl_restart_file         = 1   , 
	      ecl_unified_restart_file = 2   , 
	      ecl_summary_file         = 4   , 
	      ecl_unified_summary_file = 8   , 
	      ecl_summary_header_file  = 16  , 
	      ecl_grid_file            = 32  , 
	      ecl_egrid_file           = 64  , 
	      ecl_init_file            = 128 ,
              ecl_rft_file             = 256 ,
              ecl_data_file            = 512 } ecl_file_enum;   


#define ecl_str_len   8
enum           ecl_type_enum_def {ecl_char_type , ecl_float_type , ecl_double_type , ecl_int_type , ecl_bool_type , ecl_mess_type};  /* This is used as index in ecl_kw.c - don't touch. */
typedef enum   ecl_type_enum_def  ecl_type_enum;


int            ecl_util_get_sizeof_ctype(ecl_type_enum );

/*****************************************************************/

void            ecl_util_init_stdin(const char * , const char *);
ecl_type_enum   ecl_util_guess_type(const char * key);
char          * ecl_util_alloc_base_guess(const char *);
bool            ecl_util_unified(ecl_file_enum );
int             ecl_util_filename_report_nr(const char *);
void            ecl_util_get_file_type(const char * , ecl_file_enum * , bool * , int * );
char          * ecl_util_alloc_filename(const char * /* path */, const char * /* base */, ecl_file_enum , bool /* fmt_file */ , int /*report_nr*/);
char          * ecl_util_alloc_exfilename(const char * /* path */, const char * /* base */, ecl_file_enum , bool /* fmt_file */ , int /*report_nr*/);
char         ** ecl_util_alloc_filelist(const char * /* path */, const char * /* base */, ecl_file_enum , bool /* fmt_file */ , int /*report_nr*/ , int);
char         ** ecl_util_alloc_exfilelist(const char * /* path */, const char * /* base */, ecl_file_enum , bool /* fmt_file */ , int /*report_nr*/ , int);
char         ** ecl_util_alloc_scandir_filelist(const char *, const char *,ecl_file_enum , bool , int *);
char         ** ecl_util_alloc_simple_filelist(const char *, const char *, ecl_file_enum , bool , int , int );
void            ecl_util_memcpy_typed_data(void *, const void * , ecl_type_enum , ecl_type_enum , int );
void            ecl_util_escape_kw(char * kw);
void            ecl_util_alloc_summary_files(const char * , const char * , char ** , char *** , int *  , bool * , bool * );
void            ecl_util_alloc_restart_files(const char *  , const char *  , char *** , int *  , bool * , bool *);
const    char * ecl_util_type_name(ecl_type_enum );
time_t          ecl_util_get_start_date(const char * );
bool            ecl_util_fmt_file(const char *);
int             ecl_util_fname_cmp(const void *, const void *);

#ifdef __cplusplus
}
#endif
#endif
