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
ecl_kw_type   * ecl_file_iget_kw( const ecl_file_type *  , const char * , int);

#ifdef __cplusplus
}
#endif 
#endif
