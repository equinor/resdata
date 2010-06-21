#ifndef __ECL_UTIL_H__
#define __ECL_UTIL_H__
#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>
#include <time.h>
#include <stringlist.h>

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

/*
  Character data in ECLIPSE files comes as an array of fixed-length
  string. Each of these strings is 8 characters long. The type name,
  i.e. 'REAL', 'INTE', ... , come as 4 character strings.
*/


#define ECL_STRING_LENGTH 8
#define ECL_TYPE_LENGTH   4



/*****************************************************************/
/* 
   Observe that these type identidiers are (ab)used in both the rms and
   ert/enkf libraries in situations where ECLIPSE is not at all involved.
*/

typedef enum {
  ECL_CHAR_TYPE   = 0, 
  ECL_FLOAT_TYPE  = 1, 
  ECL_DOUBLE_TYPE = 2, 
  ECL_INT_TYPE    = 3, 
  ECL_BOOL_TYPE   = 4, 
  ECL_MESS_TYPE   = 5
} ecl_type_enum;





int              ecl_util_get_sizeof_ctype(ecl_type_enum );
ecl_type_enum    ecl_util_get_type_from_name( const char * type_name );
const char     * ecl_util_get_type_name( ecl_type_enum ecl_type );

/*****************************************************************/

void            ecl_util_init_stdin(const char * , const char *);
const char    * ecl_util_file_type_name( ecl_file_enum file_type );
char          * ecl_util_alloc_base_guess(const char *);
int             ecl_util_filename_report_nr(const char *);
void            ecl_util_get_file_type(const char * , ecl_file_enum * , bool * , int * );
char          * ecl_util_alloc_filename(const char * /* path */, const char * /* base */, ecl_file_enum , bool /* fmt_file */ , int /*report_nr*/);
char          * ecl_util_alloc_exfilename(const char * /* path */, const char * /* base */, ecl_file_enum , bool /* fmt_file */ , int /*report_nr*/);
void            ecl_util_memcpy_typed_data(void *, const void * , ecl_type_enum , ecl_type_enum , int );
void            ecl_util_escape_kw(char * kw);
bool            ecl_util_alloc_summary_files(const char * , const char * , char ** , stringlist_type * );
void            ecl_util_alloc_summary_data_files(const char * path , const char * base , bool fmt_file , stringlist_type * filelist);
void            ecl_util_alloc_restart_files(const char *  , const char *  , char *** , int *  , bool * , bool *);
time_t          ecl_util_get_start_date(const char * );
int             ecl_util_get_num_cpu(const char * data_file);
bool            ecl_util_fmt_file(const char *);
char          * ecl_util_alloc_exfilename_anyfmt(const char * path, const char * base , ecl_file_enum file_type , bool start_fmt , int report_nr);
int             ecl_util_get_month_nr(const char * month_name);
int             ecl_util_fname_report_cmp(const void *f1, const void *f2);
int             ecl_util_select_filelist( const char * path , const char * base , ecl_file_enum file_type , bool fmt_file , stringlist_type * filelist);
#ifdef __cplusplus
}
#endif
#endif
