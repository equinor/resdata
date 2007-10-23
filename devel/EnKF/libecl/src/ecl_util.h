#ifndef __ECL_UTIL_H__
#define __ECL_UTIL_H__
#include <stdbool.h>

typedef enum {ecl_other_file           = 0 , 
	      ecl_restart_file         = 1 , 
	      ecl_unified_restart_file = 2 , 
	      ecl_summary_file         = 3 , 
	      ecl_unified_summary_file = 4 , 
	      ecl_summary_header_file  = 5 , 
	      ecl_grid_file            = 6 , 
	      ecl_egrid_file           = 7 , 
	      ecl_init_file            = 8 ,
              ecl_rft_file             = 9 }   ecl_file_type;


char          * ecl_util_alloc_base_guess(const char *);
bool            ecl_util_unified(ecl_file_type );
int             ecl_util_filename_report_nr(const char *);
void            ecl_util_get_file_type(const char * , ecl_file_type * , bool * , int * );
char          * ecl_util_alloc_filename(const char * /* path */, const char * /* base */, ecl_file_type , bool /* fmt_file */ , int /*report_nr*/);
char          * ecl_util_alloc_exfilename(const char * /* path */, const char * /* base */, ecl_file_type , bool /* fmt_file */ , int /*report_nr*/);
char         ** ecl_util_alloc_exfilelist(const char *, const char *,ecl_file_type , bool , int *);
char         ** ecl_util_alloc_filelist(const char *, const char *, ecl_file_type , bool , int , int );





#endif
