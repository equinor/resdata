#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <hash.h>
#include <sched_util.h>
#include <hist.h>
#include <util.h>
#include "rate_node.h"
#include "date_node.h"

typedef struct hist_node_struct hist_node_type;

struct hist_struct {
  int  		   size;
  int  		   alloc_size; 
  time_t           start_date;
  hist_node_type **data;
  hash_type       *well_hash;
  hash_type       *var_map;
};



struct hist_node_struct {
  hash_type      *data;
  date_node_type *date;
};



/*****************************************************************/

hist_node_type * hist_node_alloc() {
  hist_node_type * hist_node = malloc(sizeof * hist_node);
  hist_node->data = hash_alloc(4);
  return hist_node;
}


void hist_node_free(hist_node_type * hist_node) {
  hash_free(hist_node->data);
  date_node_free(hist_node->date);
  free(hist_node);
}


void hist_node_fwrite(const hist_node_type * hist_node , FILE *stream) {
  bool isNULL;
  if (hist_node == NULL)
    isNULL = true;
  else
    isNULL = false;
  
  fwrite(&isNULL , sizeof isNULL , 1 , stream);
  if (!isNULL) {
    char **well_list;
    int    wells , i ;
    date_node_fwrite(hist_node->date , stream);
    wells = hash_get_size(hist_node->data);
    well_list = hash_alloc_keylist(hist_node->data);
    fwrite(&wells , sizeof wells , 1 , stream);
    for (i = 0; i < wells; i++)
      rate_sched_fwrite(hash_get(hist_node->data , well_list[i]) , stream);

    hash_free_ext_keylist(hist_node->data , well_list);
  }
}
	

hist_node_type * hist_node_fread_alloc(const time_t * start_date , FILE *stream) {
  hist_node_type *hist_node = NULL;
  bool isNULL;
  fread(&isNULL , sizeof isNULL , 1 , stream);
  if (!isNULL) {
    int wells , i;
    rate_type *rate;
    bool stop;
    hist_node = hist_node_alloc();
    hist_node->date = date_node_fread_alloc(start_date , -1 , -1 , stream , &stop);
    fread(&wells , sizeof wells , 1 , stream);
    for (i=0; i < wells; i++) {
      rate = rate_sched_fread_alloc(stream);
      hash_insert_hash_owned_ref(hist_node->data , rate_get_well_ref(rate) , rate , rate_free__);
    }
  }
  return hist_node;
}
							 





/*****************************************************************/

/*
  Some pain, to ensure that all newly (re) allocated pointers 
  are initialised with NULL.
*/
static void hist_realloc_data(hist_type * hist , int alloc_size) {
  const int new_size = alloc_size;
  const int old_size = hist->alloc_size;
  int i;
  hist_node_type **new;

  new = malloc(new_size * sizeof *new);
  for (i=0; i < new_size; i++)
    new[i] = NULL;

  if (hist->data != NULL) {
    hist_node_type **old = hist->data;
    memcpy(new , old , old_size * sizeof *old);
    free(old);
  }
  
  hist->data       = new;
  hist->alloc_size = new_size;

}


static void hist_init_var_map(hist_type * hist) {
  hash_insert_int(hist->var_map , "OPR"   , __RATE_ORAT);
  hash_insert_int(hist->var_map , "ORAT"  , __RATE_ORAT);
  hash_insert_int(hist->var_map , "WOPR"  , __RATE_ORAT);
  hash_insert_int(hist->var_map , "WOPRH" , __RATE_ORAT);  /* History */

  hash_insert_int(hist->var_map , "GRAT" , __RATE_GRAT);
  hash_insert_int(hist->var_map , "GPR"  , __RATE_GRAT);
  hash_insert_int(hist->var_map , "WGPR" , __RATE_GRAT);

  hash_insert_int(hist->var_map , "WRAT" , __RATE_WRAT);
  hash_insert_int(hist->var_map , "WPR"  , __RATE_WRAT);
  hash_insert_int(hist->var_map , "WWPR" , __RATE_WRAT);

  hash_insert_int(hist->var_map , "WCT"  , __RATE_WCT);
  hash_insert_int(hist->var_map , "WWCT" , __RATE_WCT); 
  hash_insert_int(hist->var_map , "WWCTH", __RATE_WCT);

  hash_insert_int(hist->var_map , "GOR"   , __RATE_GOR);
  hash_insert_int(hist->var_map , "WGOR"  , __RATE_GOR);
  hash_insert_int(hist->var_map , "WGORH" , __RATE_GOR);

  hash_insert_int(hist->var_map , "BHP" , __RATE_BHP);
  hash_insert_int(hist->var_map , "THP" , __RATE_THP);
}



hist_type * hist_alloc(time_t start_date) {
  hist_type * hist = malloc(sizeof *hist);
  hist->size       = 0;
  hist->well_hash  = hash_alloc(10);
  hist->var_map    = hash_alloc(10);
  hist->data       = NULL;
  hist->alloc_size = 0;
  hist->start_date = start_date;
  hist_realloc_data(hist , 5);
  hist_init_var_map(hist);
  return hist;
}



void hist_fwrite(const hist_type * hist , FILE *stream) {
  fwrite(&hist->size       , sizeof hist->size       , 1 , stream);
  fwrite(&hist->alloc_size , sizeof hist->alloc_size , 1 , stream);
  fwrite(&hist->start_date , sizeof hist->start_date , 1 , stream);
  {
    int i , wells;
    char **well_list;
    for (i=0; i < hist->size; i++)
      hist_node_fwrite(hist->data[i] , stream);
    
    fwrite(&wells , sizeof wells , 1 , stream);
    well_list = hash_alloc_keylist(hist->well_hash);
    for (i=0; i < wells; i++)
      util_fwrite_string(well_list[i] , stream);
    
    hash_free_ext_keylist(hist->well_hash , well_list);
  }
}


hist_type * hist_fread_alloc(FILE *stream) {
  hist_type * hist = hist_alloc(0);
 fread(&hist->size       , sizeof hist->size       , 1 , stream);
 fread(&hist->alloc_size , sizeof hist->alloc_size , 1 , stream);
 fread(&hist->start_date , sizeof hist->start_date , 1 , stream);
 hist_realloc_data(hist , hist->alloc_size);
 {
    int i , wells;
    char *well;
    for (i=0; i < hist->size; i++)
      hist->data[i] = hist_node_fread_alloc(&hist->start_date , stream);
    
    fread(&wells , sizeof wells , 1 , stream);
    for (i=0; i < wells; i++) {
      well = util_fread_alloc_string(stream);
      hash_insert_int(hist->well_hash , well , 1);
      free(well);
    }
  }
 return hist;
}


hist_node_type * hist_get_node(const hist_type * hist , int time_step) {
  return hist->data[time_step - 1];
}


hist_node_type * hist_add_node(hist_type * hist , int time_step) {
  hist->data[time_step-1] = hist_node_alloc();
  return hist->data[time_step - 1];
}


bool hist_has_well(const hist_type * hist , const char * well) {
  return hash_has_key(hist->well_hash , well);
}
    


static const rate_type * hist_get_rate_node(const hist_type * hist , int time_step, const char * well) {
  const rate_type * rate = NULL;

  if (time_step <= hist->size) {
    const hist_node_type * hist_node = hist_get_node(hist , time_step); 
    if (hist_node != NULL) {
      if (hash_has_key(hist_node->data , well)) 
	rate = hash_get(hist_node->data , well);
      else {
	if (!hist_has_well(hist , well)) {
	  fprintf(stderr,"%s: The well:%s does not exist in the history object - aborting \n",__func__ , well);
	  abort();
	}
      }
    } else {
      fprintf(stderr,"%s: asked for timestep:%d ... \n",__func__ , time_step);
      abort();
    }
  } else {
    fprintf(stderr,"%s tried to ask for nonexistning time_step:%d - aborting \n",__func__ , time_step);
    abort();
  }
  
  return rate;
}


static hist_node_type * hist_get_new_node(hist_type * hist , int date_nr) {
  hist_node_type * hist_node;
  
  if (date_nr > hist->alloc_size) 
    hist_realloc_data(hist , (hist->alloc_size + date_nr));
  
  hist_node = hist_get_node(hist , date_nr);
  if (hist_node == NULL)
    hist_node = hist_add_node(hist , date_nr);

  if (date_nr > hist->size)
    hist->size = date_nr;

  return hist_node;
}


void hist_add_date(hist_type * hist, const date_node_type * date) {
  int date_nr = date_node_get_date_nr(date);
  hist_node_type * hist_node = hist_get_new_node(hist , date_nr);
  hist_node->date = date_node_copyc(date);
}


void hist_add_rate(hist_type * hist , int date_nr , const rate_type * rate) {
  hist_node_type * hist_node = hist_get_new_node(hist , date_nr);
  if (hash_has_key(hist_node->data , rate_node_get_well_ref(rate))) {
    fprintf(stderr,"%s: Warning: tried to add well rate for:%s twice - only last instance stored\n",__func__ , rate_node_get_well_ref(rate));
    /*
      fprintf(stderr,"%s: INTERNAL error - tried adding the same well:%s rate twice - aborting \n",__func__ , );
      abort();
    */
    hash_del(hist_node->data , rate_node_get_well_ref(rate));
  }
  
  hash_insert_copy(hist_node->data , rate_node_get_well_ref(rate) , rate , rate_copyc__ , rate_free__);
  {
    const char * well = rate_get_well_ref(rate);
    if (!hash_has_key(hist->well_hash , well))
      hash_insert_int(hist->well_hash , well , 1);
  }

}


/*****************************************************************/


double hist_get_ORAT(const hist_type * hist , int time_step , const char * well, bool *def) {
  const rate_type *rate = hist_get_rate_node(hist , time_step , well);
  if (rate == NULL)
    return 0.0;
  else 
    return rate_get_ORAT(rate , def);
}

double hist_get_GRAT(const hist_type * hist , int time_step , const char * well, bool *def) {
  const rate_type *rate = hist_get_rate_node(hist , time_step , well);
  if (rate == NULL)
    return 0.0;
  else 
    return rate_get_GRAT(rate , def);
}

double hist_get_WRAT(const hist_type * hist , int time_step , const char * well, bool *def) {
  const rate_type *rate = hist_get_rate_node(hist , time_step , well);
  if (rate == NULL)
    return 0.0;
  else 
    return rate_get_WRAT(rate , def);
}


double hist_get_GOR(const hist_type * hist , int time_step , const char * well , bool *error, bool *def) {
  const rate_type *rate = hist_get_rate_node(hist , time_step , well);
  if (rate == NULL)
    return 0.0;
  else 
    return rate_get_GOR(rate , error , def);
}


double hist_get_WCT(const hist_type * hist , int time_step , const char * well , bool *error, bool *def) {
  const rate_type *rate = hist_get_rate_node(hist , time_step , well);
  if (rate == NULL)
    return 0.0;
  else 
    return rate_get_WCT(rate , error , def);
}


double hist_iget(const hist_type * hist , int time_step , const char * well , int var_index , bool *error , bool *def) {
  const rate_type * rate = hist_get_rate_node(hist , time_step , well);
  if (rate == NULL)
    return 0.0;
  else 
    return rate_iget(rate , var_index , error , def);
}



int hist_get_var_index(const hist_type * hist , const char * var) {
  if (hash_has_key(hist->var_map , var))
    return hash_get_int(hist->var_map , var);
  else {
    fprintf(stderr,"%s: variable: %s not recognized - available variables are: \n",__func__ , var);
    {
      char **keyList = hash_alloc_keylist(hist->var_map);
      int i;
      
      for (i=0; i < hash_get_size(hist->var_map); i++) 
	printf("%s \n",keyList[i]);

      hash_free_ext_keylist(hist->var_map , keyList);
    }
    fprintf(stderr,"aborting \n");
    abort();
  }
}



double hist_get(const hist_type * hist , int report_step , const char * well , const char * var) {
  bool error;
  bool def;
  
  return hist_iget(hist , report_step , well , hist_get_var_index(hist , var) , &error , &def);
}
  

char ** hist_alloc_well_list(const hist_type *hist,  int report_step) {
  int Nwells              = hash_get_size(hist->well_hash);
  char ** total_well_list = hash_alloc_keylist(hist->well_hash);
  char ** well_list;
  int active = 0;
  int iw;
  for (iw = 0; iw < Nwells; iw++) {
    if (hist_get_rate_node(hist , report_step , total_well_list[iw])) 
      active++;
  }
  well_list = malloc(active * sizeof * well_list);

  active = 0;
  for (iw = 0; iw < Nwells; iw++) {
    if (hist_get_rate_node(hist , report_step , total_well_list[iw])) {
      well_list[active] = util_alloc_string_copy(total_well_list[iw]);
      active++;
    }
  }

  hash_free_ext_keylist(hist->well_hash , total_well_list);
  return well_list;
}



void hist_free(hist_type *hist) {
  int i;
  for (i=0; i < hist->size; i++) {
    if (hist->data[i] != NULL)
      hist_node_free(hist->data[i]);
  }
  
  
  free(hist->data);
  hash_free(hist->var_map);
  hash_free(hist->well_hash);
  free(hist);
}



