#ifndef __ENKF_NODE_H__
#define __ENKF_NODE_H__
#include <stdlib.h>
#include <stdbool.h>
#include <enkf_util.h>
#include <enkf_types.h>

/**********************************/

typedef void   	      (serialize_ftype)      	(const void * , double * , size_t *);
typedef void   	      (de_serialize_ftype)      (void * , const double * , size_t *);
typedef void * 	      (alloc_ftype)             (const void *);
typedef void   	      (ecl_write_ftype)         (const void * , const char *);
typedef void   	      (ens_read_ftype)          (      void * , const char *);
typedef void   	      (ens_write_ftype)         (const void * , const char *);
typedef void          (swapin_ftype)      	(void * , const char *);
typedef char *        (swapout_ftype)     	(void * , const char *);
typedef void   	      (sample_ftype)     	(      void *);
typedef void   	      (free_ftype)       	(      void *);
typedef void   	      (clear_ftype)      	(      void *);
typedef void * 	      (copyc_ftype)      	(const void *);
typedef void   	      (isqrt_ftype)      	(      void *);
typedef void   	      (scale_ftype)      	(      void * , double);
typedef void   	      (iadd_ftype)       	(      void * , const void *);
typedef void   	      (imul_ftype)       	(      void * , const void *);
typedef void   	      (iaddsqr_ftype)    	(      void * , const void *);


typedef struct enkf_node_struct enkf_node_type;

typedef void          (enkf_node_ftype1)        (enkf_node_type *);

enkf_node_type * enkf_node_copyc(const enkf_node_type * );
enkf_node_type * enkf_node_alloc(const char *  	       ,  /*  1  */
				 enkf_var_type 	       ,  /*  2  */
				 const void *  	       ,  /*  3  */
				 alloc_ftype * 	       ,  /*  4  */    
				 ecl_write_ftype *     ,  /*  5  */
				 ens_read_ftype *      ,  /*  6  */
				 ens_write_ftype *     ,  /*  7  */
				 swapout_ftype *       ,  /*  8  */ 
				 swapin_ftype  *       ,  /*  9  */
				 copyc_ftype * 	       ,  /* 10  */
				 sample_ftype *	       ,  /* 11  */    
				 serialize_ftype    *  ,  /* 12  */
				 de_serialize_ftype *  ,  /* 13  */
				 free_ftype);             /* 14  */ 

void             enkf_node_free(enkf_node_type *enkf_node);
void             enkf_node_free__(void *);
void             enkf_sample    (enkf_node_type *);
bool             enkf_node_include_type(const enkf_node_type * , int );
void           * enkf_node_value_ptr(const enkf_node_type * );

void             enkf_node_ecl_write (const enkf_node_type *, const char *);
/*void             enkf_node_ecl_read  (enkf_node_type * , const char *);*/
void             enkf_node_sample(enkf_node_type *enkf_node);

void             enkf_node_ens_write (const enkf_node_type * , const char *);
void             enkf_node_serialize(enkf_node_type * , double * , size_t *);
void             enkf_node_clear     (enkf_node_type *);
void             enkf_node_ens_read  (enkf_node_type * , const char *);
const char     * enkf_node_get_key_ref(const enkf_node_type * );
bool enkf_node_swapped(const enkf_node_type *);
const char * enkf_node_get_swapfile(const enkf_node_type *);
void enkf_node_swapin(enkf_node_type *);
void enkf_node_swapout(enkf_node_type * , const char *);

void   enkf_node_scale(enkf_node_type *   , double );
void   enkf_node_iadd(enkf_node_type *    , const enkf_node_type * );
void   enkf_node_iaddsqr(enkf_node_type * , const enkf_node_type * );
void   enkf_node_imul(enkf_node_type *    , const enkf_node_type * );


#endif
