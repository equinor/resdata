#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <hash.h>
#include <sched_kw.h>
#include <sched_kw_wconhist.h>
#include <sched_kw_dates.h>
#include <sched_kw_untyped.h>





struct sched_kw_struct {
  sched_type_enum    type;
  void              *data;
};


/*****************************************************************/


sched_kw_type * sched_kw_alloc(const char * kw_name , sched_type_enum type, bool one_line_kw, int * next_date_nr) {
  
  sched_kw_type *kw = malloc(sizeof *kw);
  kw->type = type;
  
  switch (kw->type) {
    /*case(COMPDAT):
      kw->data = sched_kw_compdat_alloc();
      break;
    */
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
    /*case(COMPDAT):
      kw->data = sched_kw_compdat_alloc();
      break;
    */
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



void sched_kw_add_line(sched_kw_type * kw, const char * line, const hash_type *month_hash) {
  switch (kw->type) {
    /*case(COMPDAT):
      kw->data = sched_kw_compdat_alloc();
      break;
    */
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




void sched_kw_fprintf(const sched_kw_type * kw , FILE *stream) {
  switch (kw->type) {
    /*case(COMPDAT):
      kw->data = sched_kw_compdat_alloc();
      break;
    */
  case(WCONHIST):
    sched_kw_wconhist_fprintf(kw->data , stream);
    break;
  case(DATES):
    sched_kw_dates_fprintf(kw->data , stream);
    break;
  case(UNTYPED):
    sched_kw_untyped_fprintf(kw->data , stream);
    break;
  }
}





/*****************************************************************/















