#include <util.h>
#include <time.h>
#include <sched_file.h>



void sched_init_compdat__(const char * _schedule_file  , const int * schedule_file_len,
			  const char * _init_file      , const int * init_file_len    ,
			  const int  * endian_flip_int , const int * index_map) {

  char * schedule_file = util_alloc_cstring(_schedule_file , schedule_file_len);
  char * init_file     = util_alloc_cstring(_init_file     , init_file_len);
  bool   endian_flip   = util_intptr_2bool(endian_flip_int);
  
  sched_file_type *s = sched_file_alloc();
  sched_file_parse(s , schedule_file);
  
  sched_file_init_conn_factor(s , init_file , endian_flip , index_map );
  sched_file_free(s);

  free(init_file);
  free(schedule_file);
}



void sched_update_compdat__(const char  * _src_file       , const int * src_file_len,
			    const char  * _target_file    , const int * target_file_len,
			    const int   * last_date_nr    , 
			    const float * permx           , const int *dims, 
			    const int * index_map) {

  char * src_file       = util_alloc_cstring(_src_file    , src_file_len);
  char * target_file    = util_alloc_cstring(_target_file , target_file_len);
  
  sched_file_type *s = sched_file_alloc();
  sched_file_parse(s , src_file );
  
  sched_file_set_conn_factor(s , permx , dims , index_map );
  sched_file_fprintf(s , *last_date_nr , -1 , target_file);
  sched_file_free(s);
  
  free(src_file);
  free(target_file);
}
