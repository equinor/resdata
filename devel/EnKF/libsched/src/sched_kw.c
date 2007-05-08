#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <hash.h>
#include <sched_kw.h>
#include <sched_kw_compdat.h>
#include <sched_kw_wconhist.h>
#include <sched_kw_dates.h>
#include <sched_kw_untyped.h>





struct sched_kw_struct {
  sched_type_enum    type;
  void              *data;
};




/*****************************************************************/

void * sched_kw_get_data_ref(const sched_kw_type * kw) {return kw->data; }
sched_type_enum sched_kw_get_type(const sched_kw_type * kw) { return kw->type; }

sched_kw_type * sched_kw_alloc(const char * kw_name , sched_type_enum type, bool one_line_kw, int * next_date_nr) {
  
  sched_kw_type *kw = malloc(sizeof *kw);
  kw->type = type;
  
  switch (kw->type) {
  case(COMPDAT):
    kw->data = sched_kw_compdat_alloc();
      break;
  case(WCONHIST):
    kw->data = sched_kw_wconhist_alloc();
    break;
  case(DATES):
    kw->data = sched_kw_dates_alloc(next_date_nr);
    break;
  case(UNTYPED):
    kw->data = sched_kw_untyped_alloc(kw_name , one_line_kw);
    break;
  }
  return kw;
}



void sched_kw_free(sched_kw_type * kw) {
  switch (kw->type) {
  case(COMPDAT):
    kw->data = sched_kw_compdat_alloc();
    break;
  case(WCONHIST):
    sched_kw_wconhist_free(kw->data);
    break;
  case(DATES):
    sched_kw_dates_free(kw->data);
    break;
  case(UNTYPED):
    sched_kw_untyped_free(kw->data);
    break;
  }
  free(kw);
}


void sched_kw_free__(void * void_kw) {
  sched_kw_free( (sched_kw_type *) void_kw);
}



void sched_kw_add_line(sched_kw_type * kw, const char * line, const hash_type *month_hash) {
  switch (kw->type) {
  case(COMPDAT):
    sched_kw_compdat_add_line(kw->data , line);
    break;
  case(WCONHIST):
    sched_kw_wconhist_add_line(kw->data , line);
    break;
  case(DATES):
    sched_kw_dates_add_line(kw->data , line , month_hash);
    break;
  case(UNTYPED):
    sched_kw_untyped_add_line(kw->data , line);
    break;
  }
}




void sched_kw_fprintf(const sched_kw_type * kw , int last_date_nr , time_t last_time , FILE *stream , bool *stop) {
  switch (kw->type) {
  case(COMPDAT):
    sched_kw_compdat_fprintf(kw->data , stream);
    break;
  case(WCONHIST):
    sched_kw_wconhist_fprintf(kw->data , stream);
    break;
  case(DATES):
    sched_kw_dates_fprintf(kw->data , stream, last_date_nr , last_time , stop);
    break;
  case(UNTYPED):
    sched_kw_untyped_fprintf(kw->data , stream);
    break;
  }
}


void sched_kw_fwrite(const sched_kw_type *kw , FILE *stream) {
  fwrite(&kw->type , sizeof kw->type , 1 , stream);
  switch (kw->type) {
  case(COMPDAT):
    sched_kw_compdat_fwrite(kw->data , stream);
    break;
  case(WCONHIST):
    sched_kw_wconhist_fwrite(kw->data , stream);
    break;
  case(DATES):
    sched_kw_dates_fwrite(kw->data , stream);
    break;
  case(UNTYPED):
    sched_kw_untyped_fwrite(kw->data , stream);
    break;
  }
}


sched_kw_type * sched_kw_fread_alloc(int *next_date_nr , int last_date_nr , time_t last_time , FILE *stream , bool *at_eof, bool *stop) {
  sched_kw_type * kw = malloc(sizeof *kw);
  fread(&kw->type  , sizeof kw->type , 1 , stream);

  switch (kw->type) {
  case(COMPDAT):
    kw->data = sched_kw_compdat_fread_alloc(stream);
    break;
  case(WCONHIST):
    kw->data = sched_kw_wconhist_fread_alloc(stream);
    break;
  case(DATES):
    kw->data = sched_kw_dates_fread_alloc(next_date_nr, last_date_nr , last_time , stream , stop);
    break;
  case(UNTYPED):
    kw->data = sched_kw_untyped_fread_alloc(stream);
    break;
  }

  {
    char next_c = fgetc(stream);
    if (feof(stream)) 
      *at_eof = true;
    else {
      *at_eof = false;
      ungetc(next_c , stream);
    }
  }
  
  return kw;
}


void sched_kw_fprintf_rates(const sched_kw_type * kw , const char *obs_path , const char * obs_file , int * current_date_nr) {
  if (kw->type == WCONHIST)
    sched_kw_wconhist_fprintf_rates(kw->data , obs_path , obs_file , *current_date_nr);
  else if (kw->type == DATES)
    sched_kw_dates_iterate_current(kw->data , current_date_nr);
}





/*****************************************************************/















