#include <stdlib.h>
#include <util.h>
#include <time.h>
#include <sched_file.h>



static hist_type *GLOBAL_HIST = NULL;

/*****************************************************************/


void sched_parse_wconhist__(const char * _schedule_dump_file , const int * schedule_dump_len , 
			    const char * _obs_path           , const int * obs_path_len      ,
			    const char * _obs_file           , const int * obs_file_len) {
  sched_file_type *s;
  char * schedule_file = util_alloc_cstring(_schedule_dump_file , schedule_dump_len);
  char * obs_path      = util_alloc_cstring(_obs_path , obs_path_len);
  char * obs_file      = util_alloc_cstring(_obs_file , obs_file_len);
  
  {
    FILE * stream = util_fopen(schedule_file , "r");
    s = sched_file_fread_alloc( stream , -1  , -1 , -1.0);
    fclose(stream);
  }
  sched_file_fprintf_rates(s , obs_path , obs_file);  
  sched_file_free(s);
  
  free(schedule_file);
  free(obs_path);
  free(obs_file);
}


void sched_update_compdat_fprintf_static(const char * _schedule_dump_file , const int * schedule_dump_len , 
					 const char * _schedule_file      , const int * schedule_file_len,
					 const char * _last_date_str      , const int * last_date_len , 
					 const float * permx , const float * permz, const int * index_map) {
  
  sched_file_type *s;
  char * schedule_file      = util_alloc_cstring(_schedule_file , schedule_file_len);
  char * schedule_dump_file = util_alloc_cstring(_schedule_dump_file , schedule_dump_len);
  char * last_date_str      = util_alloc_cstring(_last_date_str      , last_date_len);

  {
    FILE * stream = util_fopen(schedule_dump_file , "r");
    s = sched_file_fread_alloc( stream , atoi(last_date_str)  , -1 , -1.0);
    fclose(stream);
  }
  if (permx != NULL) 
    sched_file_set_conn_factor(s , permx , permz , index_map );
  
  sched_file_fprintf(s , atoi(last_date_str) , -1 , -1.0 , schedule_file);
  sched_file_free(s);

  free(last_date_str);
  free(schedule_dump_file);
  free(schedule_file);
}

     




void sched_fprintf__(const char * _schedule_dump_file , const int * schedule_dump_len , 
		     const char * _schedule_file      , const int * schedule_file_len,
		     const char * _last_date_str      , const int * last_date_len) {

  sched_update_compdat_fprintf_static(   _schedule_dump_file ,   schedule_dump_len , 
					 _schedule_file      ,   schedule_file_len,
					 _last_date_str      ,   last_date_len , 
					 NULL , NULL , NULL);
  

}


void sched_update_compdat_fprintf__(const char * _schedule_dump_file , const int * schedule_dump_len , 
				    const char * _schedule_file      , const int * schedule_file_len,
				    const char * _last_date_str      , const int * last_date_len , 
				    const float * permx , const float * permy, const int * index_map) {

  sched_update_compdat_fprintf_static(   _schedule_dump_file ,   schedule_dump_len , 
					 _schedule_file      ,   schedule_file_len,
					 _last_date_str      ,   last_date_len , 
					 permx ,    permy,   index_map);
  
}




void sched_init__(const char * _schedule_file  	   , const int * schedule_file_len,
		  const char * _schedule_dump_file , const int * schedule_dump_len,
		  const char * _init_file          , const int * init_file_len,
		  const int  * start_date , 
		  const int  * endian_flip_int , 
		  const int  * index_map) {

  char * schedule_file      = util_alloc_cstring(_schedule_file , schedule_file_len);
  char * schedule_dump_file = util_alloc_cstring(_schedule_dump_file , schedule_dump_len);
  bool   update             = true;

  if (util_file_exists(schedule_dump_file)) 
    update = util_file_update_required(schedule_file , schedule_dump_file);
  
  update = true;
  if (update) {
    char * init_file   = util_alloc_cstring(_init_file , init_file_len);
    bool   endian_flip = util_intptr_2bool(endian_flip_int);
    sched_file_type *s = sched_file_alloc(start_date);
    sched_file_parse(s  , schedule_file);
    sched_file_init_conn_factor(s , init_file , endian_flip , index_map);
    sched_file_fprintf_days_dat(s , "days.dat");

    {
      FILE *stream = util_fopen(schedule_dump_file , "w");
      sched_file_fwrite(s , stream);
      fclose(stream);
    }
    GLOBAL_HIST = sched_file_alloc_hist(s);
    sched_file_free(s);
    free(init_file);
  }
  
  free(schedule_dump_file);
  free(schedule_file);
}
		  

  







