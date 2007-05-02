#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sched_kw_wconhist.h>
#include <list.h>
#include <util.h>
#include <list_node.h>
#include <sched_util.h>
#include <stdbool.h>

typedef enum {LRAT , ORAT , RESV}  well_control_type;       
typedef enum {OPEN , STOP , SHUT}  well_state_type;
static const double RATE_ERROR = -1.0;

struct sched_kw_wconhist_struct {
  int        kw_size;
  list_type *rate_list;
};


typedef struct  {
  char              *well;
  well_control_type cmode;
  well_state_type   state;   
  char             *state_string;
  char             *cmode_string;
  double 	    ORAT;         
  double 	    WRAT;    
  double 	    GRAT;    
  int               VFPTable;
  double            ALift;
  double 	    THP;	    
  double 	    BHP;	    
  double            WGASRAT;

  double 	    GOR;	    
  double 	    WCT;     
  
  int               kw_size;
  bool             *def;
} rate_type;


/*****************************************************************/


static void rate_sched_fprintf_dbl(const rate_type *rate , int index, double value , FILE *stream) {
  if (rate->def[index])
    fprintf(stream,"1*   ");
  else
    fprintf(stream , "%12.6f " , value);
}

static void rate_sched_fprintf_int(const rate_type *rate , int index, int value , FILE *stream) {
  if (rate->def[index])
    fprintf(stream,"1* ");
  else
    fprintf(stream , "%4d " , value);
}


static void rate_sched_fprintf(const rate_type * rate , FILE *stream) {
  fprintf(stream , "  \'%s\'  \'%s\'    \'%s\'" , rate->well , rate->state_string , rate->cmode_string);
  rate_sched_fprintf_dbl(rate , 3  , rate->ORAT     , stream);
  rate_sched_fprintf_dbl(rate , 4  , rate->WRAT     , stream);
  rate_sched_fprintf_dbl(rate , 5  , rate->GRAT     , stream);
  rate_sched_fprintf_int(rate , 6  , rate->VFPTable , stream);
  rate_sched_fprintf_dbl(rate , 7  , rate->ALift    , stream);
  rate_sched_fprintf_dbl(rate , 8  , rate->THP      , stream);
  rate_sched_fprintf_dbl(rate , 9  , rate->BHP      , stream);
  rate_sched_fprintf_dbl(rate , 10 , rate->WGASRAT  , stream);
  fprintf(stream , " /\n");
}


static void rate_set_from_string_dbl(double *rate , const char * token) {
  if (token == NULL)
    *rate = 0;
  else
    *rate = atof(token);
}


static void rate_set_from_string_int(int *rate , const char * token) {
  if (token == NULL)
    *rate = 0;
  else
    *rate = atoi(token);
}



static void rate_set_from_string(rate_type * node , int kw_size , const char **token_list , bool *well_shut) {
  node->def     = calloc(node->kw_size , sizeof *node->def);
  {
    int i;
    for (i=0; i < kw_size; i++) {
      if (token_list[i] == NULL)
	node->def[i] = true;
      else
	node->def[i] = false;
    }
  }
  node->well         = util_alloc_string_copy(token_list[0]);
  node->state_string = util_alloc_string_copy(token_list[1]);
  node->cmode_string = util_alloc_string_copy(token_list[2]);
  
  rate_set_from_string_dbl(&node->ORAT     , token_list[3] );
  rate_set_from_string_dbl(&node->WRAT     , token_list[4] );
  rate_set_from_string_dbl(&node->GRAT     , token_list[5] );
  rate_set_from_string_int(&node->VFPTable , token_list[6] );
  rate_set_from_string_dbl(&node->ALift    , token_list[7] );
  rate_set_from_string_dbl(&node->THP      , token_list[8] );
  rate_set_from_string_dbl(&node->BHP      , token_list[9] );
  rate_set_from_string_dbl(&node->WGASRAT  , token_list[10]);

  if (node->ORAT != 0.0)
    node->GOR = node->GRAT / node->ORAT;
  else
    node->GOR = RATE_ERROR;

  if ((node->ORAT + node->WRAT) != 0.0)
    node->WCT = node->WRAT / (node->ORAT + node->WRAT);
  else
    node->WCT = RATE_ERROR;
  
  if (strcmp(node->state_string , "OPEN") == 0) node->state = OPEN;
  if (strcmp(node->state_string , "STOP") == 0) node->state = STOP;
  if (strcmp(node->state_string , "SHUT") == 0) node->state = SHUT;

  if (strcmp(node->cmode_string , "RESV") == 0) node->cmode = RESV;
  if (strcmp(node->cmode_string , "LRAT") == 0) node->cmode = LRAT;
  if (strcmp(node->cmode_string , "ORAT") == 0) node->cmode = ORAT;
}


static rate_type * rate_alloc(int kw_size , const char **token_list) {
  rate_type *node = malloc(sizeof *node);
  bool well_shut;
  node->well = NULL;
  rate_set_from_string(node ,kw_size , token_list , &well_shut);
  return node;
}


static void rate_free(rate_type *rate) {
  free(rate->well);
  free(rate->def);
  free(rate->cmode_string);
  free(rate->state_string);
  free(rate);
}


static void rate_free__(void *__rate) {
  rate_type *rate = (rate_type *) __rate;
  rate_free(rate);
}



/*****************************************************************/


void sched_kw_wconhist_fprintf(const sched_kw_wconhist_type *kw , FILE *stream) {
  fprintf(stream , "WCONHIST\n");
  {
    list_node_type *rate_node = list_get_head(kw->rate_list);
    while (rate_node != NULL) {
      const rate_type * rate = list_node_value_ptr(rate_node);
      rate_sched_fprintf(rate , stream);
      rate_node = list_node_get_next(rate_node);
    }
  }
  fprintf(stream , "/\n\n");
}



sched_kw_wconhist_type * sched_kw_wconhist_alloc( ) {
  sched_kw_wconhist_type * kw = malloc(sizeof *kw);
  kw->rate_list = list_alloc();
  kw->kw_size   = 11;
  return kw;
}


void sched_kw_wconhist_add_line(sched_kw_wconhist_type * kw , const char * line) {
  int tokens;
  char **token_list;
  
  sched_util_parse_line(line , &tokens , &token_list , kw->kw_size);
  {
    rate_type * rate = rate_alloc(kw->kw_size , (const char **) token_list);
    list_append_list_owned_ref(kw->rate_list , rate , rate_free__);
  }
  sched_util_free_token_list(tokens , token_list);
  
}


void sched_kw_wconhist_free(sched_kw_wconhist_type * kw) {
  list_free(kw->rate_list);
  free(kw);
}












