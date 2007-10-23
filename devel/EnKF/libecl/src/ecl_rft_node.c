#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <util.h>
#include <ecl_kw.h>
#include <ecl_block.h>
#include <ecl_fstate.h>
#include <ecl_rft_node.h>
#include <hash.h>
#include <time.h>


typedef enum { RFT = 1 , PLT = 2 , SEGMENT } ecl_rft_enum;

struct ecl_rft_node_struct {
  char   * well_name;
  int      size;
  float   *i , *j , *k;

  ecl_rft_enum data_type;
  time_t       recording_time;
  int          time3[3];
  float        double_time;
  double       *P , *SWAT , *SGAS, *DEPTH;
};



static ecl_rft_node_type * ecl_rft_node_alloc_empty(int size) {
  ecl_rft_node_type * rft_node = malloc(sizeof * rft_node);

  rft_node->i 	  = util_malloc(size * sizeof * rft_node->i    	, __func__);
  rft_node->j 	  = util_malloc(size * sizeof * rft_node->j    	, __func__);
  rft_node->k 	  = util_malloc(size * sizeof * rft_node->k    	, __func__);
  rft_node->P 	  = util_malloc(size * sizeof * rft_node->P    	, __func__);
  rft_node->SWAT  = util_malloc(size * sizeof * rft_node->SWAT 	, __func__);
  rft_node->SGAS  = util_malloc(size * sizeof * rft_node->SGAS 	, __func__);
  rft_node->DEPTH = util_malloc(size * sizeof * rft_node->DEPTH , __func__);
  rft_node->data_type = RFT;
  rft_node->size  = size;

  return rft_node;
}



ecl_rft_node_type * ecl_rft_node_alloc(const ecl_block_type * rft_block) {
  
  ecl_kw_type       * ecl_kw   = ecl_block_get_kw(rft_block , "CONIPOS");
  ecl_rft_node_type * rft_node = ecl_rft_node_alloc_empty(ecl_kw_get_size(ecl_kw));

  {
    char * tmp;
    tmp = ecl_kw_iget_ptr(ecl_block_get_kw(rft_block , "WELLETC") , 1);
    rft_node->well_name = util_alloc_strip_copy(tmp);
    
    tmp = ecl_kw_iget_ptr(ecl_block_get_kw(rft_block , "WELLETC") , 5);
    if (strchr(tmp , 'P') != NULL)
      rft_node->data_type = PLT;
    else if (strchr(tmp, 'R') != NULL)
      rft_node->data_type = RFT;
    else if (strchr(tmp , 'S') != NULL)
      rft_node->data_type = SEGMENT;
    else {
      fprintf(stderr,"%s: Could not determine type of RFT/PLT/SEGMENT data - aborting\n",__func__);
      abort();
    }
  }
  /*
    Should check type before allocating .... 
  */
  if (rft_node->data_type != RFT) {
    ecl_rft_node_free(rft_node);
    return NULL;
  } else {
    ecl_kw_get_memcpy_data(ecl_block_get_kw(rft_block , "CONIPOS") , rft_node->i);
    ecl_kw_get_memcpy_data(ecl_block_get_kw(rft_block , "CONJPOS") , rft_node->j);
    ecl_kw_get_memcpy_data(ecl_block_get_kw(rft_block , "CONKPOS") , rft_node->k);
    
    ecl_kw_get_data_as_double(ecl_block_get_kw(rft_block , "PRESSURE") , rft_node->P);
    ecl_kw_get_data_as_double(ecl_block_get_kw(rft_block , "SWAT")     , rft_node->SWAT);
    ecl_kw_get_data_as_double(ecl_block_get_kw(rft_block , "SGAS")     , rft_node->SGAS);
    ecl_kw_get_data_as_double(ecl_block_get_kw(rft_block , "DEPTH")    , rft_node->DEPTH);

    return rft_node;
  }
}


const char * ecl_rft_node_well_name_ref(const ecl_rft_node_type * rft_node) { return rft_node->well_name; }


void ecl_rft_node_free(ecl_rft_node_type * rft_node) {
  free(rft_node->i);	 
  free(rft_node->j);	 
  free(rft_node->k);	 
  free(rft_node->P);	 
  free(rft_node->SWAT);
  free(rft_node->SGAS);
  free(rft_node->DEPTH);

  free(rft_node);
}

void ecl_rft_node_free__(void * void_node) {
  ecl_rft_node_free((ecl_rft_node_type *) void_node);
}



void ecl_rft_node_block(const ecl_rft_node_type * rft_node , int size , const double * tvd , int * i, int * j , int *k) {
  const double epsilon = 10;
  int rft_index , tvd_index;
  for (tvd_index = 0; tvd_index < size; tvd_index++) {
    double min_diff       = 100000;
    int    min_diff_index = 0;
    for (rft_index = 0; rft_index < rft_node->size; rft_index++) {
      const double diff = fabs(tvd[tvd_index] - rft_node->DEPTH[rft_index]);
      if (diff < min_diff) {
	min_diff = diff;
	min_diff_index = rft_index;
      }
    }

    if (min_diff < epsilon) {
      i[tvd_index] = rft_node->i[min_diff_index];
      j[tvd_index] = rft_node->j[min_diff_index];
      k[tvd_index] = rft_node->k[min_diff_index];
    } else {
      i[tvd_index] = -1;
      j[tvd_index] = -1;
      k[tvd_index] = -1;
      fprintf(stderr,"%s: Warning: True Vertical Depth:%g could not be mapped to well_path for well:%s \n",__func__ , tvd[tvd_index] , rft_node->well_name);
    }

  }
}
