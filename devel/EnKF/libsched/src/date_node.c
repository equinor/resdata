#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <util.h>
#include <list.h>
#include <hash.h>
#include <sched_util.h>
#include <stdbool.h>


#include "date_node.h"


struct date_node_struct {
  bool     TStep;
  time_t   time;
  time_t  *start_time;
  int      date_nr;
};


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


date_node_type * date_node_alloc(int date_nr , const char *line , const hash_type *month_hash) {
  date_node_type *date_node = malloc(sizeof *date_node);
  date_node->date_nr     = date_nr; 
  
  {
    int tokens;
    char **token_list;
    struct tm ts;
    sched_util_parse_line(line, &tokens , &token_list , 3 , NULL);
    ts.tm_sec    = 0;
    ts.tm_min    = 0;
    ts.tm_hour   = 0;
    ts.tm_mday   = atoi(token_list[0]);
    ts.tm_mon    = hash_get_int(month_hash , token_list[1]);
    ts.tm_year   = atoi(token_list[2]) - 1900;
    date_node->time = mktime( &ts );
  }
  date_node->TStep = false;
  return date_node;
}

void date_node_free(date_node_type *date) {
  free(date);
}


void date_node_free__(void *__date) {
  date_node_free((date_node_type *) __date);
}


void date_node_fprintf(const date_node_type * node, FILE *stream , int last_date_nr , time_t last_time , bool *stop) {
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

void date_node_fwrite(const date_node_type * date_node , FILE *stream) {
  fwrite(&date_node->time    , sizeof date_node->time    , 1 , stream);
  fwrite(&date_node->TStep   , sizeof date_node->TStep   , 1 , stream);
  fwrite(&date_node->date_nr , sizeof date_node->date_nr , 1 , stream);
}


date_node_type * date_node_fread_alloc(int last_date_nr , time_t last_time , FILE *stream, bool *stop) {
  date_node_type * node = malloc(sizeof *node); /* SKipping spesific alloc routine - UGGLY */
  fread(&node->time    , sizeof node->time    , 1 , stream);
  fread(&node->TStep   , sizeof node->TStep   , 1 , stream);
  fread(&node->date_nr , sizeof node->date_nr , 1 , stream);

  if (last_date_nr > 0) {
    if (node->date_nr >= last_date_nr)
      *stop = true;
  }
  
  if (last_time > 0) {
    if (node->time >= last_time)
      *stop = true;
  }
  return node;
}

int date_node_get_date_nr(const date_node_type * date_node) { 
  if (date_node == NULL)
    return 1;
  else
    return date_node->date_nr; 
}


void date_node_fprintf_rate_date(const date_node_type * date_node , const char * _obs_path , const char *date_file) {
  FILE *stream;
  char *obs_path = malloc(strlen(_obs_path) + 6);
  sprintf(obs_path , "%s/%04d" , _obs_path , date_node->date_nr);
  if (util_path_exists(obs_path)) {
    struct tm ts;
    char *file     = malloc(strlen(_obs_path) + strlen(date_file) + 7);
    localtime_r(&date_node->time , &ts);
    sprintf(file , "%s/%04d/%s" , _obs_path , date_node->date_nr , date_file);
    stream = util_fopen(file , false);
    fprintf(stream , "%02d \'%s\' %4d  / -- Dates keyword: %3d \n" , ts.tm_mday , month_table[ts.tm_mon] , ts.tm_year + 1900 , date_node->date_nr);
    fclose(stream);
    free(file);
  }
  free(obs_path);
}
