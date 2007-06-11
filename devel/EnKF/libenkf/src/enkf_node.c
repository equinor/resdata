#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <enkf_node.h>
#include <util.h>


struct enkf_node_struct {
  alloc_ftype         *alloc;
  ecl_write_ftype     *ecl_write;
  alloc_ensfile_ftype *alloc_ensfile;
  ens_read_ftype      *ens_read;
  ens_write_ftype     *ens_write;
  swapin_ftype    *swapin;
  swapout_ftype   *swapout;
  
  sample_ftype    *sample;
  free_ftype      *freef;
  clear_ftype     *clear;
  copyc_ftype     *copyc;
  scale_ftype     *scale;
  iadd_ftype      *iadd;
  imul_ftype      *imul;
  isqrt_ftype     *isqrt;
  iaddsqr_ftype   *iaddsqr;

  char            *swapfile;
  char            *node_key;
  void            *data;
  const void      *config;
  enkf_var_type    var_type;
};



enkf_node_type * enkf_node_alloc(const char *node_key, 
				 enkf_var_type var_type , 
				 const void * config, 
				 alloc_ftype     * alloc     , 
				 ecl_write_ftype * ecl_write , 
				 alloc_ensfile_ftype * alloc_ensfile , 
				 ens_read_ftype  * ens_read  , 
				 ens_write_ftype * ens_write , 
				 swapout_ftype   * swapout   , 
				 swapin_ftype    * swapin    ,
				 copyc_ftype     * copyc     ,
				 sample_ftype    * sample    , 
				 free_ftype      * freef) {
  
  enkf_node_type *node = malloc(sizeof *node);
  node->alloc     = alloc;
  node->ecl_write = ecl_write;
  node->alloc_ensfile = alloc_ensfile;
  node->ens_read  = ens_read;
  node->ens_write = ens_write;
  node->swapin    = swapin;
  node->swapout   = swapout;
  node->sample    = sample;
  node->freef     = freef;
  node->copyc     = copyc;
  node->node_key  = util_alloc_string_copy(node_key);
  node->var_type  = var_type;
  node->config    = config;
  node->swapfile  = NULL;
  node->data      = node->alloc(node->config);
  return node;
}



enkf_node_type * enkf_node_copyc(const enkf_node_type * src) {
  if (src->copyc == NULL) {
    printf("Har ikke copyc funksjon\n");
    abort();
  }
  {
    enkf_node_type * new = enkf_node_alloc(enkf_node_get_key_ref(src) , 
					   src->var_type , 
					   src->config,
					   src->alloc,
					   src->ecl_write,
					   src->alloc_ensfile,
					   src->ens_read,
					   src->ens_write, 
					   src->swapout, 
					   src->swapin,
					   src->copyc,
					   src->sample,
					   src->freef);
  return new;
  }
}



bool enkf_node_include_type(const enkf_node_type * enkf_node, int mask) {
  if (enkf_node->var_type & mask)
    return true;
  else
    return false;
}

bool enkf_node_swapped(const enkf_node_type *enkf_node) {
  if (enkf_node->swapfile == NULL)
    return false;
  else
    return true;
}

const char * enkf_node_get_swapfile(const enkf_node_type * enkf_node) {
  return enkf_node->swapfile;
}


#define FUNC_ASSERT(func,func_name) if (func == NULL) { fprintf(stderr,"%s: function handler: %s not registered when writing node:%s - aborting\n",__func__ , func_name , enkf_node->node_key); abort(); }

void * enkf_node_value_ptr(const enkf_node_type * enkf_node) { return enkf_node->data; }


void enkf_node_ecl_write(const enkf_node_type *enkf_node , const char *path) {
  FUNC_ASSERT(enkf_node->ecl_write , "ecl_write");
  enkf_node->ecl_write(enkf_node->data , path);
}

void enkf_node_ens_write(const enkf_node_type *enkf_node , const char * path) {
  FUNC_ASSERT(enkf_node->ens_write , "ens_write");
  enkf_node->ens_write(enkf_node->data , path);
}

void enkf_node_ens_read(enkf_node_type *enkf_node , const char * path) {
  FUNC_ASSERT(enkf_node->ens_read , "ens_read");
  enkf_node->ens_read(enkf_node->data , path);
}

void enkf_node_ens_clear(enkf_node_type *enkf_node) {
  FUNC_ASSERT(enkf_node->clear , "clear");
  enkf_node->clear(enkf_node->data);
}

void enkf_node_sqrt(enkf_node_type *enkf_node) {
  FUNC_ASSERT(enkf_node->isqrt , "sqrt");
  enkf_node->isqrt(enkf_node->data);
}

void enkf_node_scale(enkf_node_type *enkf_node , double scale_factor) {
  FUNC_ASSERT(enkf_node->scale , "scale");
  enkf_node->scale(enkf_node->data , scale_factor);
}

void enkf_node_iadd(enkf_node_type *enkf_node , const enkf_node_type * delta_node) {
  FUNC_ASSERT(enkf_node->iadd , "iadd");
  enkf_node->iadd(enkf_node->data , delta_node->data);
}

void enkf_node_iaddsqr(enkf_node_type *enkf_node , const enkf_node_type * delta_node) {
  FUNC_ASSERT(enkf_node->iaddsqr , "iaddsqr");
  enkf_node->iaddsqr(enkf_node->data , delta_node->data);
}

void enkf_node_imul(enkf_node_type *enkf_node , const enkf_node_type * delta_node) {
  FUNC_ASSERT(enkf_node->imul , "imul");
  enkf_node->imul(enkf_node->data , delta_node->data);
}


void enkf_node_sample(enkf_node_type *enkf_node) {
  FUNC_ASSERT(enkf_node->sample , "sample");
  enkf_node->sample(enkf_node->data);
}

void enkf_node_swapin(enkf_node_type *enkf_node) {
  FUNC_ASSERT(enkf_node->swapin , "swapin");
  if (enkf_node->swapfile == NULL) {
    fprintf(stderr,"%s: swapfile == NULL - probably forgot to call swapout first - aborting \n",__func__);
    abort();
  }
  enkf_node->swapin(enkf_node->data , enkf_node->swapfile);
  free(enkf_node->swapfile);
  enkf_node->swapfile = NULL;
}

void enkf_node_swapout(enkf_node_type *enkf_node, const char *path) {
  FUNC_ASSERT(enkf_node->swapout , "swapout");
  enkf_node->swapfile = enkf_node->swapout(enkf_node->data , path);
}

void enkf_node_clear(enkf_node_type *enkf_node) {
  FUNC_ASSERT(enkf_node->clear , "clear");
  enkf_node->clear(enkf_node->data);
}

char * enkf_node_alloc_ensfile(const enkf_node_type *enkf_node , const char * path) {
  FUNC_ASSERT(enkf_node->alloc_ensfile , "alloc_ensfile");
  return enkf_node->alloc_ensfile(enkf_node->data , path);
}

void enkf_node_free(enkf_node_type *enkf_node) {
  if (enkf_node->freef != NULL)
    enkf_node->freef(enkf_node->data);
  free(enkf_node->node_key);
  if (enkf_node->swapfile != NULL) free(enkf_node->swapfile);
  free(enkf_node);
}

const char *enkf_node_get_key_ref(const enkf_node_type * enkf_node) { return enkf_node->node_key; }

#undef FUNC_ASSERT




