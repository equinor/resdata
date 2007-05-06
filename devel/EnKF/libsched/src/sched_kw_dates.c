#include <stdlib.h>
#include <list.h>
#include <hash.h>
#include <time.h>
#include <util.h>
#include <sched_util.h>
#include <sched_kw_dates.h>


struct sched_kw_dates_struct {
  int *next_date_ptr;
  list_type *date_list;
};



typedef struct {
  time_t   time;
  int      date_nr;
} date_node_type;


static const char month_table[12][4] = {{"JAN\0"},
					{"FEB\0"},
					{"MAR\0"},
					{"APR\0"},
					{"MAY\0"},
					{"JUN\0"},
					{"JUL\0"},
					{"AUG\0"},
					{"SEP\0"},
					{"OCT\0"},
					{"NOV\0"},
					{"DEC\0"}};


/*****************************************************************/

static date_node_type * date_node_alloc(int date_nr , const char *line , const hash_type *month_hash) {
  date_node_type *date_node = malloc(sizeof *date_node);
  date_node->date_nr     = date_nr; 
  
  {
    int tokens;
    char **token_list;
    struct tm ts;
    sched_util_parse_line(line, &tokens , &token_list , 3);
    ts.tm_sec    = 0;
    ts.tm_min    = 0;
    ts.tm_hour   = 0;
    ts.tm_mday   = atoi(token_list[0]);
    ts.tm_mon    = hash_get_int(month_hash , token_list[1]);
    ts.tm_year   = atoi(token_list[2]) - 1900;
    date_node->time = mktime( &ts );
  }
  return date_node;
}

static void date_node_free(date_node_type *date) {
  free(date);
}


static void date_node_free__(void *__date) {
  date_node_free((date_node_type *) __date);
}


static void date_node_fprintf(const date_node_type * node, FILE *stream , int last_date_nr , time_t last_time , bool *stop) {
  struct tm ts;
  
  
  localtime_r(&node->time , &ts);
  fprintf(stream , "  %02d \'%s\' %4d  / -- Dates keyword: %3d \n" , ts.tm_mday , month_table[ts.tm_mon] , ts.tm_year + 1900 , node->date_nr);
  if (last_date_nr > 0) {
    if (node->date_nr >= last_date_nr)
      *stop = true;
  }

  if (last_time > 0) {
    if (node->time >= last_time)
      *stop = true;
  }

}

static void date_node_fwrite(const date_node_type * date_node , FILE *stream) {
  fwrite(&date_node->time    , sizeof date_node->time    , 1 , stream);
  fwrite(&date_node->date_nr , sizeof date_node->date_nr , 1, stream);
}


static date_node_type * date_node_fread_alloc(FILE *stream) {
  date_node_type * node = malloc(sizeof *node); /* SKipping spesific alloc routine - UGGLY */
  fread(&node->time    , sizeof node->time    , 1 , stream);
  fread(&node->date_nr , sizeof node->date_nr , 1 , stream);
  return node;
}

/*****************************************************************/


void sched_kw_dates_add_line(sched_kw_dates_type *kw , const char *line , const hash_type *month_hash) {
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


sched_kw_dates_type * sched_kw_dates_alloc(int *next_date_ptr){
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



sched_kw_dates_type * sched_kw_dates_fread_alloc(int * next_date_ptr , FILE *stream) {
  int lines , i;
  sched_kw_dates_type *kw = sched_kw_dates_alloc(next_date_ptr) ;
  fread(&lines       , sizeof lines       , 1 , stream);
  for (i=0; i < lines; i++) {
    date_node_type * date_node = date_node_fread_alloc( stream);
    list_append_list_owned_ref(kw->date_list , date_node , date_node_free__);
  } 
  return kw;
}


