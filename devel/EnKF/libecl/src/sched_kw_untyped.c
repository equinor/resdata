#include <stdlib.h>
#include <stdio.h>
#include <sched_kw_untyped.h>
#include <list.h>
#include <list_node.h>
#include <util.h>
#include <stdbool.h>


struct sched_kw_untyped_struct {
  bool       one_line;
  char      *kw_name;
  list_type *line_list;
};


/*****************************************************************/


void sched_kw_untyped_fprintf(const sched_kw_untyped_type *kw , FILE *stream) {
  fprintf(stream , "%s\n" , kw->kw_name);
  {
    list_node_type *line_node = list_get_head(kw->line_list);
    while (line_node != NULL) {
      const char * line = list_node_value_ptr(line_node);
      fprintf(stream , "%s" , line);
      line_node = list_node_get_next(line_node);
    }

    if (kw->one_line)
      fprintf(stream,"\n");
    else
      fprintf(stream , "/\n\n");
    
  }
}


sched_kw_untyped_type * sched_kw_untyped_alloc(const char * kw_name , bool one_line) {
  sched_kw_untyped_type * kw = malloc(sizeof *kw);
  kw->line_list = list_alloc();
  kw->one_line  = one_line;
  kw->kw_name   = util_alloc_string_copy(kw_name);
  return kw;
}


void sched_kw_untyped_add_line(sched_kw_untyped_type * kw , const char *line) {
  list_append_string_copy(kw->line_list , line);
}


void sched_kw_untyped_free(sched_kw_untyped_type * kw) {
  list_free(kw->line_list);
  free(kw->kw_name);
  free(kw);
}














