#ifndef __ECL_UTIL_H__
#define __ECL_UTIL_H__
#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>
#include <time.h>

  typedef enum {ECL_OTHER_FILE           = 0   , 
		ECL_RESTART_FILE         = 1   , 
		ECL_UNIFIED_RESTART_FILE = 2   , 
		ECL_SUMMARY_FILE         = 4   , 
		ECL_UNIFIED_SUMMARY_FILE = 8   , 
		ECL_SUMMARY_HEADER_FILE  = 16  , 
		ECL_GRID_FILE            = 32  , 
		ECL_EGRID_FILE           = 64  , 
		ECL_INIT_FILE            = 128 ,
		ECL_RFT_FILE             = 256 ,
		ECL_DATA_FILE            = 512 } ecl_file_enum;   


  /*
    This enum enumerates the four different ways summary and restart information
    can be stored.
  */

  
  typedef enum { ECL_INVALID_STORAGE       = 0,
                 ECL_BINARY_UNIFIED        = 1,
		 ECL_FORMATTED_UNIFIED     = 2,
		 ECL_BINARY_NON_UNIFIED    = 4,
		 ECL_FORMATTED_NON_UNIFIED = 8} ecl_storage_enum;




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
char          * ecl_util_alloc_exfilename_anyfmt(const char * path, const char * base , ecl_file_enum file_type , bool start_fmt , int report_nr);

#ifdef __cplusplus
}
#endif
#endif
