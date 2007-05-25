#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <enkf_util.h>



void * enkf_util_malloc(int byte_size , const char * caller) {
  void *tmp = malloc(byte_size);
  if (tmp == NULL) {
    fprintf(stderr,"%s: failed to allocate %d bytes - aborting \n",caller , byte_size);
    abort();
  }
  return tmp;
}

void * enkf_util_calloc(int elements , int element_size , const char * caller) {
  void *tmp = calloc(elements , element_size);
  if (tmp == NULL) {
    fprintf(stderr,"%s: failed to allocate %d bytes - aborting \n",caller , elements * element_size);
    abort();
  }
  return tmp;
}


void * enkf_util_realloc(void *ptr , int byte_size , const char * caller) {
  void *tmp = realloc(ptr , byte_size);
  if (tmp == NULL) {
    fprintf(stderr,"%s: failed to allocate %d bytes - aborting \n",caller , byte_size);
    abort();
  }
  return tmp;
}



static FILE * enkf_util_fopen(const char * filename , const char * mode , const char * text_mode , const char * caller) {
  FILE *stream = fopen(filename , mode);
  if (stream == NULL) {
    fprintf(stderr,"%s: failed to open:%s for %s - aborting \n",caller , filename , text_mode);
    abort();
  }
  return stream;
}
  
FILE * enkf_util_fopen_w(const char * filename , const char * caller) {
  return enkf_util_fopen(filename , "w" , "writing" , caller);
}

FILE * enkf_util_fopen_r(const char * filename , const char * caller) {
  return enkf_util_fopen(filename , "r" , "reading" , caller);
}


void enkf_util_fwrite(const void *ptr , int item_size, int items , FILE *stream , const char * caller) {
  printf("%s:  item_size:%d  items:%d  caller:%s \n",__func__ , item_size , items , caller);
  if (fwrite(ptr , item_size , items , stream) != items) {
    fprintf(stderr,"%s: failed to write : %d bytes - aborting \n",caller , (item_size * items));
    abort();
  }
}

void enkf_util_fread(void *ptr , int item_size, int items , FILE *stream , const char * caller) {
  if (fread(ptr , item_size , items , stream) != items) {
    fprintf(stderr,"%s: failed to read : %d bytes - aborting \n",caller , (item_size * items));
    abort();
  }
}


/*****************************************************************/

static void enkf_util_rand_dbl(int N , double max , double *R) {
  int i;
  for (i=0; i < N; i++) 
    R[i] = rand() * max / RAND_MAX;
}


double enkf_util_rand_normal(double mean , double std) {
  const double pi = 3.141592653589;
  double R[2];
  enkf_util_rand_dbl(2 , 1.0 , R);
  return mean + std * sqrt(-2.0 * log(R[0])) * cos(2.0 * pi * R[1]);
}
