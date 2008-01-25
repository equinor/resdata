#include <stdlib.h>
#include <string.h>
#include <restart_kw_list.h>
#include <util.h>

typedef enum {initialized , writing , reading} _mode_enum;

struct restart_kw_list_struct {
  _mode_enum   mode;
  int  	       buffer_size;        /* The total number of char* elements we have allocated space for. */
  int  	       current_kw_index;   /* The keyword index we are currently going to read / write. */
  int  	       active_elements;    /* The number of elements which have been added */
  char 	    ** kw_list;            /* The actual keywords */
};


static void restart_kw_list_realloc(restart_kw_list_type * kw_list , int new_size) {
  int old_size = kw_list->buffer_size;
  kw_list->kw_list = util_realloc(kw_list->kw_list , new_size * sizeof * kw_list->kw_list , __func__);
  {
    int i;
    for (i=old_size; i < new_size; i++)
      kw_list->kw_list[i] = NULL;
  }
  kw_list->buffer_size = new_size;
}


void restart_kw_list_reset(restart_kw_list_type * kw_list) {
  kw_list->current_kw_index = 0;
  kw_list->mode             = initialized;
}


restart_kw_list_type * restart_kw_list_alloc() {
  int def_size = 2; 
  restart_kw_list_type * kw_list = util_malloc(sizeof *kw_list , __func__);
  kw_list->buffer_size      = 0;
  kw_list->current_kw_index = 0;
  kw_list->active_elements  = 0;
  kw_list->kw_list          = NULL;
  restart_kw_list_realloc(kw_list , def_size);
  restart_kw_list_reset(kw_list);

  return kw_list;
}


void restart_kw_list_add(restart_kw_list_type * kw_list , const char * kw) {
  if (kw_list->mode == reading) {
    fprintf(stderr,"%s: restart_kw_list object is in reading mode - must switch with restart_kw_list_reset() - aborting \n",__func__);
    abort();
  }
  kw_list->mode = writing;
  if (kw_list->current_kw_index == kw_list->buffer_size) 
    restart_kw_list_realloc(kw_list , kw_list->buffer_size * 2 + 2);
  kw_list->kw_list[kw_list->current_kw_index] = util_realloc_string_copy(kw_list->kw_list[kw_list->current_kw_index] , kw );
  kw_list->current_kw_index +=  1;
  kw_list->active_elements   =  kw_list->current_kw_index;
}



const char * restart_kw_list_get_next(restart_kw_list_type * kw_list) {
  if (kw_list->mode == writing) {
    fprintf(stderr,"%s: restart_kw_list object is in writing  mode - must switch with restart_kw_list_reset() - aborting \n",__func__);
    abort();
  }
  kw_list->mode = reading;
  if (kw_list->current_kw_index == kw_list->active_elements)
    return NULL;
  else {
    const char * return_kw = kw_list->kw_list[kw_list->current_kw_index];
    kw_list->current_kw_index += 1;
    return return_kw;
  }
}


const char * restart_kw_list_get_first(restart_kw_list_type * kw_list) {
  restart_kw_list_reset(kw_list);
  return restart_kw_list_get_next(kw_list);
}


void restart_kw_list_free(restart_kw_list_type * kw_list) {
  int i;
  for (i=0; i < kw_list->buffer_size; i++) 
    if (kw_list->kw_list[i] != NULL)
      free(kw_list->kw_list[i]);
  free(kw_list->kw_list);
  free(kw_list);
  kw_list = NULL;
}


void restart_kw_list_memcpy(restart_kw_list_type * src , restart_kw_list_type * target) {
  const char * kw;

  restart_kw_list_reset(target);
  restart_kw_list_reset(src);
  kw = restart_kw_list_get_first(src);
  while (kw != NULL) 
    restart_kw_list_add(target , kw);
  
}
