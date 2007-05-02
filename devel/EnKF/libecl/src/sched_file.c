#include <stdlib.h>
#include <stdio.h>
#include <hash.h>
#include <list.h>
#include <sched_util.h>
#include <sched_kw.h>
#include <sched_file.h>


struct sched_file_struct {
  hash_type  *month_hash;
  hash_type  *one_line_kw;
  hash_type  *kw_types;
  list_type  *kw_list;
  int         next_date_nr;
};



void sched_file_parse(sched_file_type * sched_file , const char * filename) {
  FILE *stream = fopen(filename , "r");

  flcose(stream);
}


sched_file_type * sched_file_alloc() {
  sched_file_type * sched_file = malloc(sizeof *sched_file);
  {
    hash_type *month_hash = hash_alloc(24);
    hash_insert_int(month_hash , "JAN" , 0);
    hash_insert_int(month_hash , "FEB" , 1);
    hash_insert_int(month_hash , "MAR" , 2);
    hash_insert_int(month_hash , "APR" , 3);
    hash_insert_int(month_hash , "MAI" , 4);
    hash_insert_int(month_hash , "JUN" , 5);
    hash_insert_int(month_hash , "JUL" , 6);
    hash_insert_int(month_hash , "AUG" , 7);
    hash_insert_int(month_hash , "SEP" , 8);
    hash_insert_int(month_hash , "OCT" , 9);
    hash_insert_int(month_hash , "NOV" ,10);
    hash_insert_int(month_hash , "DEC" ,11);
    sched_file->month_hash = month_hash;
  }
  {
    hash_type * one_line_kw = hash_alloc(10);
    hash_insert_int(one_line_kw , "INCLUDE" , 1);
    sched_file->one_line_kw = one_line_kw;
  }
  {
    hash_type * kw_types = hash_alloc(10);
    hash_insert_int(kw_types , "DATES"    , DATES);
    hash_insert_int(kw_types , "WCONHIST" , WCONHIST);
    sched_file->kw_types = kw_types;
  }

  sched_file->next_date_nr = 0;
  sched_file->kw_list      = list_alloc();
  
  return sched_file;
}


sched_kw_type * sched_file_add_kw(sched_file_type *sched_file , const char *kw_name) {
  sched_type_enum type;
  bool            one_line_kw = false;
  
  if (hash_has_key(sched_file->kw_types , kw_name)) 
    type = hash_get_int(sched_file->kw_types , kw_name);
  else {
    type = UNTYPED;
    if (hash_get_int(sched_file->one_line_kw , kw_name))
      one_line_kw = true;
    else
      one_line_kw = false;
  }
  
  sched_kw_type * kw = sched_kw_alloc(kw_name , type , one_line_kw , &sched_file->next_date_nr);
}
  




void sched_file_free(sched_file_type *sched_file) {
  list_free(sched_file->kw_list);
  hash_free(sched_file->month_hash);
  hash_free(sched_file->one_line_kw);
  free(sched_file);
}


int main (void) {
  sched_file_type *s = sched_file_alloc();
  
  
  
  
  sched_file_free(s);
}
