#include <stdlib.h>
#include <string.h>
#include <list.h>
#include <hash.h>
#include <time.h>
#include <util.h>
#include <sched_util.h>
#include <sched_kw_dates.h>
#include "date_node.h"

struct sched_kw_dates_struct {
  int *next_date_ptr;
  list_type *date_list;
};


/*****************************************************************/


void sched_kw_dates_add_line(sched_kw_dates_type *kw , const char *line , const hash_type *month_hash, bool TSTEP) {
  date_node_type * date_node = date_node_alloc(*kw->next_date_ptr , line , month_hash);
  list_append_list_owned_ref(kw->date_list , date_node , date_node_free__);
  (*kw->next_date_ptr) += 1;
}


void sched_kw_dates_fprintf(const sched_kw_dates_type *kw , FILE *stream , int last_date_nr , time_t last_time , bool *stop) {
  fprintf(stream,"DATES\n");
  {
    list_node_type *date_node = list_get_head(kw->date_list);
    while (date_node != NULL) {
      const date_node_type * date = list_node_value_ptr(date_node);
      date_node_fprintf(date , stream , last_date_nr , last_time , stop);
      if (*stop) 
	date_node = NULL;
      else
	date_node = list_node_get_next(date_node);
    }
    fprintf(stream , "/\n\n");
  }
}


sched_kw_dates_type * sched_kw_dates_alloc(int *next_date_ptr , const time_t * start_date){
  sched_kw_dates_type *dates = malloc(sizeof *dates);
  dates->date_list     = list_alloc();
  dates->next_date_ptr = next_date_ptr;
  return dates;
}

void sched_kw_dates_free(sched_kw_dates_type * kw) {
  list_free(kw->date_list);
  free(kw);
}



void sched_kw_dates_fwrite(const sched_kw_dates_type *kw , FILE *stream) {
  {
    int date_lines = list_get_size(kw->date_list);
    fwrite(&date_lines , sizeof date_lines , 1, stream);
  }
  {
    list_node_type *date_node = list_get_head(kw->date_list);
    while (date_node != NULL) {
      const date_node_type * date = list_node_value_ptr(date_node);
      date_node_fwrite(date ,  stream);
      date_node = list_node_get_next(date_node);
    }
  }
}



sched_kw_dates_type * sched_kw_dates_fread_alloc(int * next_date_ptr , const time_t * start_date , int last_date_nr ,time_t last_time , FILE *stream, bool *stop) {
  int lines , line_nr;
  sched_kw_dates_type *kw = sched_kw_dates_alloc(next_date_ptr , start_date) ;
  fread(&lines       , sizeof lines       , 1 , stream);
  line_nr = 0;
  while (!(*stop) && (line_nr < lines)) {
    date_node_type * date_node = date_node_fread_alloc(last_date_nr , last_time ,  stream , stop);
    list_append_list_owned_ref(kw->date_list , date_node , date_node_free__);
    line_nr++;
  } 
  return kw;
}


void sched_kw_dates_iterate_current(const sched_kw_dates_type * kw , date_node_type **current_date) {
  (*current_date)     = list_node_value_ptr(list_get_tail(kw->date_list));
}


