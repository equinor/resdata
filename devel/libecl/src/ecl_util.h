#ifndef __ECL_UTIL_H__
#define __ECL_UTIL_H__
#include <stdbool.h>

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
              ecl_data_file            = 512 } ecl_file_type;   


#define ecl_str_len   8
typedef enum   ecl_type_enum_def  ecl_type_enum;
enum           ecl_type_enum_def {ecl_char_type , ecl_float_type , ecl_double_type , ecl_int_type , ecl_bool_type , ecl_mess_type};


int            ecl_util_get_sizeof_ctype(ecl_type_enum );

/*****************************************************************/


ecl_type_enum   ecl_util_guess_type(const char * key);
char          * ecl_util_alloc_base_guess(const char *);
bool            ecl_util_unified(ecl_file_type );
int             ecl_util_filename_report_nr(const char *);
void            ecl_util_get_file_type(const char * , ecl_file_type * , bool * , int * );
char          * ecl_util_alloc_filename(const char * /* path */, const char * /* base */, ecl_file_type , bool /* fmt_file */ , int /*report_nr*/);
char          * ecl_util_alloc_exfilename(const char * /* path */, const char * /* base */, ecl_file_type , bool /* fmt_file */ , int /*report_nr*/);
char         ** ecl_util_alloc_filelist(const char * /* path */, const char * /* base */, ecl_file_type , bool /* fmt_file */ , int /*report_nr*/ , int);
char         ** ecl_util_alloc_exfilelist(const char * /* path */, const char * /* base */, ecl_file_type , bool /* fmt_file */ , int /*report_nr*/ , int);
char         ** ecl_util_alloc_scandir_filelist(const char *, const char *,ecl_file_type , bool , int *);
char         ** ecl_util_alloc_simple_filelist(const char *, const char *, ecl_file_type , bool , int , int );
void            ecl_util_memcpy_typed_data(void *, const void * , ecl_type_enum , ecl_type_enum , int );
void            ecl_util_escape_kw(char * kw);



#endif
