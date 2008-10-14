#ifndef __ECL_BLOCK_H__
#define __ECL_BLOCK_H__

#include <stdbool.h>
#include <ecl_kw.h>
#include <fortio.h>
#include <time.h>
#include <restart_kw_list.h>


typedef struct ecl_block_struct ecl_block_type;


ecl_block_type * ecl_block_safe_cast(const void * );
restart_kw_list_type * ecl_block_get_restart_kw_list(const ecl_block_type * );
void             ecl_block_set_sim_time_summary(ecl_block_type * , /*int time_index , int years_index , */ int , int , int );
void             ecl_block_set_sim_time_restart(ecl_block_type * );
void             ecl_block_set_sim_time(ecl_block_type * , time_t );
void             ecl_block_set_report_nr(ecl_block_type * , int );
time_t           ecl_block_get_sim_time(const ecl_block_type * );
int              ecl_block_get_report_nr(const ecl_block_type * );
bool             ecl_block_fseek(int , bool , bool , fortio_type * );
int              ecl_block_get_kw_size(const ecl_block_type *, const char *);
void             ecl_block_fread(ecl_block_type *, bool , fortio_type * , bool *); 
bool             ecl_block_has_kw(const ecl_block_type * , const char * );
ecl_kw_type    * ecl_block_get_kw(const ecl_block_type *, const char *);
ecl_kw_type    * ecl_block_iget_kw(const ecl_block_type *, const char *, int);
ecl_kw_type    * ecl_block_get_last_kw(const ecl_block_type *, const char*);
/* void           * ecl_block_get_data_ref(const ecl_block_type *, const char *); */
ecl_block_type * ecl_block_alloc(int );
ecl_block_type * ecl_block_fread_alloc(int ,  bool ,  fortio_type * , bool *);
ecl_block_type * ecl_block_alloc_copy(const ecl_block_type *);
void             ecl_block_free(ecl_block_type *);
bool             ecl_block_add_kw(ecl_block_type * , const ecl_kw_type *);
bool             ecl_block_add_kw_copy(ecl_block_type * , const ecl_kw_type *);
void             ecl_block_fwrite(ecl_block_type * , bool , fortio_type *);
void             ecl_block_set_fmt_file(ecl_block_type *, bool);
void 	         ecl_block_select_formatted(ecl_block_type *);
void 	         ecl_block_select_binary   (ecl_block_type *);
void             ecl_block_printf_kwlist(const ecl_block_type *);
ecl_kw_type    * ecl_block_detach_kw(ecl_block_type * , const char *);
void             ecl_block_free_kw(ecl_block_type *, const char *);
void             ecl_block_set_time_step(ecl_block_type * , int );
void             ecl_block_summarize(const ecl_block_type * );
#endif
