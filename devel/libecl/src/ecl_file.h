#ifndef __ECL_FILE_H__
#define __ECL_FILE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <ecl_kw.h>
#include <fortio.h>
#include <time.h>

typedef struct ecl_file_struct ecl_file_type;

ecl_file_type * ecl_file_fread_alloc( const char * filename );

void            ecl_file_free( ecl_file_type * ecl_file );
void            ecl_file_free__(void * arg);
ecl_kw_type   * ecl_file_iget_named_kw( const ecl_file_type *  ecl_file , const char * kw , int ith);
ecl_kw_type   * ecl_file_icopy_named_kw( const ecl_file_type * ecl_file , const char * kw, int ith);
ecl_kw_type   * ecl_file_icopy_kw( const ecl_file_type * ecl_file , int index);
bool            ecl_file_has_kw( const ecl_file_type * ecl_file , const char * kw);
int             ecl_file_get_num_named_kw(const ecl_file_type * ecl_file , const char * kw);
int             ecl_file_get_num_kw( const ecl_file_type * ecl_fil );
ecl_kw_type   * ecl_file_iget_kw( const ecl_file_type * ecl_file  , int index);
int             ecl_file_get_num_distinct_kw(const ecl_file_type * ecl_file);
const char    * ecl_file_iget_distinct_kw(const ecl_file_type * ecl_file , int index);
int             ecl_file_iget_occurence( const ecl_file_type *  ecl_file , int index);
time_t          ecl_file_iget_restart_sim_date( const ecl_file_type * restart_file , int occurence );

ecl_file_type * ecl_file_fread_alloc_unsmry_section(const char * filename , int index );
ecl_file_type * ecl_file_fread_alloc_unrst_section(const char * filename , int report_step);
ecl_file_type * ecl_file_fread_alloc_restart_section(fortio_type * fortio);
ecl_file_type * ecl_file_fread_alloc_summary_section(fortio_type * fortio);
ecl_file_type * ecl_file_fread_alloc_RFT_section(fortio_type *     fortio);

void            ecl_file_insert_kw( ecl_file_type * ecl_file , ecl_kw_type * ecl_kw , bool after , const char * neighbour_name , int neighbour_occurence );
void 		ecl_file_fwrite_fortio(const ecl_file_type * ec_file  , fortio_type * fortio , int offset);
void 		ecl_file_fwrite(const ecl_file_type * ecl_file , const char * , bool fmt_file );

#ifdef __cplusplus
}
#endif 
#endif
