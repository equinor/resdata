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
  int    *i , *j , *k;

  bool    vertical_well;
  ecl_rft_enum data_type;
  time_t       recording_time;
  /*float        double_time;*/
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
    int time3[3];
    ecl_kw_get_memcpy_data(ecl_block_get_kw(rft_block , "CONIPOS") , rft_node->i);
    ecl_kw_get_memcpy_data(ecl_block_get_kw(rft_block , "CONJPOS") , rft_node->j);
    ecl_kw_get_memcpy_data(ecl_block_get_kw(rft_block , "CONKPOS") , rft_node->k);
    
    ecl_kw_get_data_as_double(ecl_block_get_kw(rft_block , "PRESSURE") , rft_node->P);
    ecl_kw_get_data_as_double(ecl_block_get_kw(rft_block , "SWAT")     , rft_node->SWAT);
    ecl_kw_get_data_as_double(ecl_block_get_kw(rft_block , "SGAS")     , rft_node->SGAS);
    ecl_kw_get_data_as_double(ecl_block_get_kw(rft_block , "DEPTH")    , rft_node->DEPTH);

 
    ecl_kw_get_memcpy_data(ecl_block_get_kw(rft_block , "DATE") , time3);
    rft_node->recording_time = util_make_date(time3[0] , time3[1] , time3[2]);
    {
      int i;
      rft_node->vertical_well = true;
      double first_delta = rft_node->DEPTH[1] - rft_node->DEPTH[0];
      for (i = 1; i < (rft_node->size - 1); i++) {
	double delta = rft_node->DEPTH[i + 1] - rft_node->DEPTH[i];
	if (fabs(delta) > 0) {
	  if (first_delta * delta < 0)
	    rft_node->vertical_well = false;
	}
      }
    }
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
  free(rft_node->well_name);
  free(rft_node);
}

void ecl_rft_node_free__(void * void_node) {
  ecl_rft_node_free((ecl_rft_node_type *) void_node);
}



void ecl_rft_node_summarize(const ecl_rft_node_type * rft_node , bool print_cells) {
  int i;
  printf("--------------------------------------------------------------\n");
  printf("Well.............: %s \n",rft_node->well_name);
  printf("Completed cells..: %d \n",rft_node->size);
  printf("Vertical well....: %d \n",rft_node->vertical_well);
  {
    int day , month , year;
    util_set_date_values(rft_node->recording_time , &day , &month , &year);
    printf("Recording time...: %02d/%02d/%4d\n" , day , month , year);
  }
  printf("--------------------------------------------------------------\n");
  if (print_cells) {
    for (i=0; i < rft_node->size; i++) {
      printf("%d: %3d %3d %3d | %g %g \n",i,rft_node->i[i] , rft_node->j[i] , rft_node->k[i] , rft_node->DEPTH[i] , rft_node->P[i]);
    }
    printf("--------------------------------------------------------------\n");
  }
}




/* 
   For this function to work the ECLIPSE keyword COMPORD must be used.
*/
void ecl_rft_node_block(const ecl_rft_node_type * rft_node , double epsilon , int size , const double * tvd , int * i, int * j , int *k) {
  int rft_index , tvd_index;
  int last_rft_index   = 0;
  bool  * blocked      = util_malloc(rft_node->size * sizeof * blocked , __func__);

  if (!rft_node->vertical_well) {
    fprintf(stderr,"**************************************************************************\n");
    fprintf(stderr,"** WARNING: Trying to block horizontal well: %s from only tvd, this **\n", ecl_rft_node_well_name_ref(rft_node));
    fprintf(stderr,"** is extremely error prone - blocking should be carefully checked.     **\n");
    fprintf(stderr,"**************************************************************************\n");
  }
  ecl_rft_node_summarize(rft_node , true);
  for (tvd_index = 0; tvd_index < size; tvd_index++) {
    double min_diff       = 100000;
    int    min_diff_index = 0;
    if (rft_node->vertical_well) {
      for (rft_index = last_rft_index; rft_index < rft_node->size; rft_index++) {      
	double diff = fabs(tvd[tvd_index] - rft_node->DEPTH[rft_index]);
	if (diff < min_diff) {
	  min_diff = diff;
	  min_diff_index = rft_index;
	}
      }
    } else {
      for (rft_index = last_rft_index; rft_index < (rft_node->size - 1); rft_index++) {      
	int next_rft_index = rft_index + 1;
	if ((rft_node->DEPTH[next_rft_index] - tvd[tvd_index]) * (tvd[tvd_index] - rft_node->DEPTH[rft_index]) > 0) {
	  /*
	    We have bracketing ... !!
	  */
	  min_diff_index = rft_index;
	  min_diff = 0;
	  printf("Bracketing:  %g  |  %g  | %g \n",rft_node->DEPTH[next_rft_index] , tvd[tvd_index] , rft_node->DEPTH[rft_index]);
	  break;
	}
      }
    }

    if (min_diff < epsilon) {
      i[tvd_index] = rft_node->i[min_diff_index];
      j[tvd_index] = rft_node->j[min_diff_index];
      k[tvd_index] = rft_node->k[min_diff_index];
      
      blocked[min_diff_index] = true;
      last_rft_index          = min_diff_index;
    } else {
      i[tvd_index] = -1;
      j[tvd_index] = -1;
      k[tvd_index] = -1;
      fprintf(stderr,"%s: Warning: True Vertical Depth:%g (Point:%d/%d) could not be mapped to well_path for well:%s \n",__func__ , tvd[tvd_index] , tvd_index+1 , size , rft_node->well_name);
    }
  }
  free(blocked);
}



/*
  Faen - dette er jeg egentlig *ikke* interessert i ....
*/
static double * ecl_rft_node_alloc_well_md(const ecl_rft_node_type * rft_node , int fixed_index , double md_offset) {
  double * md = util_malloc(rft_node->size * sizeof * md , __func__);
  
  
    
  return md;
}


void ecl_rft_node_block_static(const ecl_rft_node_type * rft_node , int rft_index_offset , int md_size , int md_index_offset , const double * md , int * i , int * j , int * k) {
  const double md_offset = md[md_index_offset];
  double *well_md        = ecl_rft_node_alloc_well_md(rft_node , rft_index_offset , md_offset);
  int index;
  for (index = 0; index < md_size; index++) {
    i[index] = -1;
    j[index] = -1;
    k[index] = -1;
  }
  i[md_index_offset] = rft_node->i[rft_index_offset];
  j[md_index_offset] = rft_node->j[rft_index_offset];
  k[md_index_offset] = rft_node->k[rft_index_offset];
  

  free(well_md);
}




void ecl_rft_node_block2(const ecl_rft_node_type * rft_node , int tvd_size , const double * md , const double * tvd , int * i, int * j , int *k) {
  const double epsilon = 1.0;
  int rft_index ;
  
  ecl_rft_node_summarize(rft_node , true);
  {
    double min_diff       = 100000;
    int    min_diff_index = 0;  
    int    tvd_index      = 0;
    double local_tvd_peak = 99999999;
    {
      int index;
      for (index = 1; index < (tvd_size - 1); index++) 
	if (tvd[index] < tvd[index+1] && tvd[index] < tvd[index-1]) 
	  local_tvd_peak = util_double_min(tvd[index] , local_tvd_peak);
    }
    
    
    while (min_diff > epsilon) {
      for (rft_index = 0; rft_index < rft_node->size; rft_index++) {      
	double diff = fabs(tvd[tvd_index] - rft_node->DEPTH[rft_index]);
	if (diff < min_diff) {
	  min_diff = diff;
	  min_diff_index = rft_index;
	}
      }
      if (min_diff > epsilon) {
	tvd_index++;
	if (tvd_index == tvd_size) {
	  fprintf(stderr,"%s: could not map tvd:%g to well-path depth - aborting \n",__func__ , tvd[tvd_index]);
	  abort();
	}

	if (tvd[tvd_index] > local_tvd_peak) {
	  fprintf(stderr,"%s: could not determine offset before well path becomes multivalued - aborting\n",__func__);
	  abort();
	}
      }
    }
    ecl_rft_node_block_static(rft_node , min_diff_index , tvd_size , tvd_index , md , i , j , k);
  }
}





void ecl_rft_node_fprintf_rft_obs(const ecl_rft_node_type * rft_node , double epsilon , const char * tvd_file , const char * target_file , double p_std) {
  FILE * input_stream  = util_fopen(tvd_file    , "r" );
  int size    	       = util_count_file_lines(input_stream);
  double *p   	       = util_malloc(size * sizeof * p,   __func__);
  double *tvd 	       = util_malloc(size * sizeof * tvd, __func__);
  double *md           = util_malloc(size * sizeof * md,  __func__);
  int    *i   	       = util_malloc(size * sizeof * i,   __func__);
  int    *j   	       = util_malloc(size * sizeof * j,   __func__);
  int    *k   	       = util_malloc(size * sizeof * k,   __func__);

  {
    double *arg1 , *arg2 , *arg3;
    int line;
    arg1 = p;
    arg2 = tvd;
    arg3 = md;
    for (line = 0; line < size; line++)
      if (fscanf(input_stream , "%lg  %lg", &arg1[line] , &arg2[line]) != 2) {
	fprintf(stderr,"%s: something wrong when reading: %s - aborting \n",__func__ , tvd_file);
	abort();
      }
  }
  fclose(input_stream);
  ecl_rft_node_block(rft_node , epsilon , size , tvd , i , j , k);

  {
    int active_lines = 0;
    int line;
    for (line = 0; line < size; line++)
      if (i[line] != -1)
	active_lines++;
    
    if (active_lines > 0) {
      FILE * output_stream = util_fopen(target_file , "w" );
      fprintf(output_stream,"%d\n" , active_lines);
      for (line = 0; line < size; line++)
	if (i[line] != -1)
	  fprintf(output_stream , "%3d %3d %3d %g %g\n",i[line] , j[line] , k[line] , p[line] , p_std);
      fclose(output_stream);
    } else 
      fprintf(stderr,"%s: Warning found no active cells when blocking well:%s to data_file:%s \n",__func__ , rft_node->well_name , tvd_file);
  }

  free(md);
  free(p);
  free(tvd);
  free(i);
  free(j);
  free(k);
}


void ecl_rft_node_export_DEPTH(const ecl_rft_node_type * rft_node , const char * path) {
  FILE * stream;
  char * full;
  char * filename = util_alloc_string_copy(ecl_rft_node_well_name_ref(rft_node));
  int i;
  
  filename = util_strcat_realloc(filename , ".DEPTH");
  full = util_alloc_full_path(path , filename);

  stream = fopen(full , "w");
  for (i=0; i < rft_node->size; i++) 
    fprintf(stream , "%d  %g \n",i , rft_node->DEPTH[i]);
  fclose(stream);

  /*
    free(full);
    free(filename);
  */
}


int         ecl_rft_node_get_size(const ecl_rft_node_type * rft_node) { return rft_node->size; }
const int * ecl_rft_node_get_i (const ecl_rft_node_type * rft_node) { return rft_node->i; }
const int * ecl_rft_node_get_j (const ecl_rft_node_type * rft_node) { return rft_node->j; }
const int * ecl_rft_node_get_k (const ecl_rft_node_type * rft_node) { return rft_node->k; }
time_t      ecl_rft_node_get_recording_time(const ecl_rft_node_type * rft_node) { return rft_node->recording_time; }
