#ifndef __ENKF_NODE_H__
#define __ENKF_NODE_H__
#include <stdlib.h>
#include <stdbool.h>
#include <enkf_util.h>

/**********************************/

typedef void   (ecl_write_ftype)  (const void *);
typedef void   (ens_read_ftype)   (      void *);
typedef void   (ens_write_ftype)  (const void *);
typedef void   (sample_ftype)     (      void *);
typedef void   (free_ftype)       (      void *);
typedef void   (clear_ftype)      (      void *);
typedef void * (copyc_ftype)      (const void *);
typedef void   (isqrt_ftype)      (      void *);
typedef void   (scale_ftype)      (      void * , double);
typedef void   (iadd_ftype)       (      void * , const void *);
typedef void   (imul_ftype)       (      void * , const void *);
typedef void   (iaddsqr_ftype)    (      void * , const void *);


typedef struct enkf_node_struct enkf_node_type;

enkf_node_type * enkf_node_copyc(const enkf_node_type * );
enkf_node_type * enkf_node_alloc(const char * , enkf_var_type , void * , ecl_write_ftype * , ens_read_ftype , ens_write_ftype , copyc_ftype * , sample_ftype *, free_ftype);
void             enkf_node_free(enkf_node_type *enkf_node);
void             enkf_sample    (enkf_node_type *);
bool             enkf_node_include_type(const enkf_node_type * , int );
void           * enkf_node_value_ptr(const enkf_node_type * );

void             enkf_node_ecl_write (const enkf_node_type *);
void             enkf_node_ecl_read  (enkf_node_type *);
void             enkf_node_sample(enkf_node_type *enkf_node);

void             enkf_node_ens_write (const enkf_node_type *);
void             enkf_node_clear     (enkf_node_type *);
void             enkf_node_ens_read  (enkf_node_type *);
const char     * enkf_node_get_key_ref(const enkf_node_type * );


void enkf_node_scale(enkf_node_type *   , double );
void enkf_node_iadd(enkf_node_type *    , const enkf_node_type * );
void enkf_node_iaddsqr(enkf_node_type * , const enkf_node_type * );
void enkf_node_imul(enkf_node_type *    , const enkf_node_type * );


#endif
