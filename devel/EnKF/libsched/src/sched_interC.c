#include <stdlib.h>
#include <util.h>
#include <time.h>
#include <sched_file.h>



void sched_init_compdat__(const char * _schedule_file  	   , const int * schedule_file_len,
			  const char * _init_file      	   , const int * init_file_len    ,
			  const char * _schedule_dump_file , const int * schedule_dump_len , 
			  const int  * endian_flip_int     , const int * index_map) {

  char * schedule_file      = util_alloc_cstring(_schedule_file , schedule_file_len);
  char * init_file          = util_alloc_cstring(_init_file     , init_file_len);
  char * schedule_dump_file = util_alloc_cstring(_schedule_dump_file , schedule_dump_len);
  bool   endian_flip        = util_intptr_2bool(endian_flip_int);
  
  sched_file_type *s = sched_file_alloc();
  sched_file_parse(s , schedule_file);
  
  sched_file_init_conn_factor(s , init_file , endian_flip , index_map );
  {
    FILE *stream = fopen(schedule_dump_file , "w");
    if (stream == NULL) {
      fprintf(stderr,"%s: failed to open: %s for writing - aborting \n",__func__ , schedule_dump_file);
      abort();
    }
    sched_file_fwrite(s , stream);
    fclose(stream);
  }
  sched_file_free(s);


  free(schedule_dump_file);
  free(init_file);
  free(schedule_file);
}



void sched_update_compdat__(const char  * _src_dump_file  , const int * src_dump_file_len,
			    const char  * _target_file    , const int * target_file_len,
			    const int   * last_date_nr    , 
			    const float * permx           , const int *dims, 
			    const int   * index_map) {
  sched_file_type *s;
  char * src_dump_file  = util_alloc_cstring(_src_dump_file    , src_dump_file_len);
  char * target_file    = util_alloc_cstring(_target_file , target_file_len);
  FILE * stream         = fopen(src_dump_file , "r");
  if (stream == NULL) {
    fprintf(stderr,"%s: failed to open %s for reading - aborting \n",__func__ , src_dump_file);
    abort();
  }

  s = sched_file_fread_alloc(*last_date_nr , -1 , stream);
  fclose(stream);
  
  sched_file_set_conn_factor(s , permx , dims , index_map );
  sched_file_fprintf(s , *last_date_nr , -1 , target_file);
  sched_file_free(s);
  
  free(src_dump_file);
  free(target_file);
}





