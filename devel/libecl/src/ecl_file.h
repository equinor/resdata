#ifndef __ECL_FILE_H__
#define __ECL_FILE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <ecl_kw.h>
#include <fortio.h>


typedef struct ecl_file_struct ecl_file_type;

ecl_file_type * ecl_file_fread_alloc( const char * , bool);
void            ecl_file_free( ecl_file_type * );
void            ecl_file_free__(void * );
ecl_kw_type   * ecl_file_iget_named_kw( const ecl_file_type *  , const char * , int);
bool            ecl_file_has_kw( const ecl_file_type *  , const char * );
int             ecl_file_get_num_named_kw(const ecl_file_type * , const char * );
int             ecl_file_get_num_kw( const ecl_file_type * );
ecl_kw_type   * ecl_file_iget_kw( const ecl_file_type *  , int);
int             ecl_file_get_num_distinct_kw(const ecl_file_type * );
const char    * ecl_file_iget_distinct_kw(const ecl_file_type * , int );
int             ecl_file_iget_occurence( const ecl_file_type *  , int );

ecl_file_type * ecl_file_fread_alloc_restart_section(fortio_type * );
ecl_file_type * ecl_file_fread_alloc_summary_section(fortio_type * );
ecl_file_type * ecl_file_fread_alloc_RFT_section(fortio_type * );

void 		ecl_file_fwrite_fortio(const ecl_file_type *  , fortio_type * );
void 		ecl_file_fwrite(const ecl_file_type *  , const char * , bool , bool );

#ifdef __cplusplus
}
#endif 
#endif
