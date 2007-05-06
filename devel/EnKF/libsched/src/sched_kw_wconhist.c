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
  
  bool             *def;
} rate_type;


/*****************************************************************/

static rate_type * rate_alloc_empty(int kw_size) {
  rate_type *node = malloc(sizeof *node);
  node->well         = NULL;
  node->state_string = NULL;
  node->cmode_string = NULL;
  node->def          = calloc(kw_size , sizeof *node->def);
  return node;
}


static void rate_sched_fwrite(const rate_type *rate , int kw_size , FILE *stream) {
  util_fwrite_string(rate->well , stream);
  util_fwrite_string(rate->state_string , stream);
  util_fwrite_string(rate->cmode_string , stream);
  
  
  fwrite(&rate->cmode 	 , sizeof rate->cmode 	  , 1 , stream);
  fwrite(&rate->state 	 , sizeof rate->state 	  , 1 , stream);
  fwrite(&rate->ORAT  	 , sizeof rate->ORAT  	  , 1 , stream);
  fwrite(&rate->WRAT     , sizeof rate->WRAT  	  , 1 , stream);
  fwrite(&rate->GRAT     , sizeof rate->GRAT  	  , 1 , stream);
  fwrite(&rate->VFPTable , sizeof rate->VFPTable  , 1 , stream);
  fwrite(&rate->ALift    , sizeof rate->ALift     , 1 , stream);
  fwrite(&rate->THP	 , sizeof rate->THP 	  , 1 , stream);
  fwrite(&rate->BHP	 , sizeof rate->BHP 	  , 1 , stream);
  fwrite(&rate->WGASRAT  , sizeof rate->WGASRAT   , 1 , stream);
  fwrite(&rate->GOR	 , sizeof rate->GOR 	  , 1 , stream);
  fwrite(&rate->WCT      , sizeof rate->WCT 	  , 1 , stream);
  fwrite(&rate->def      , sizeof rate->def       , kw_size , stream);
}


static rate_type * rate_sched_fread_alloc(int kw_size , FILE *stream) {
  rate_type *rate = rate_alloc_empty(kw_size);
  rate->well         = util_fread_alloc_string( stream );
  rate->state_string = util_fread_alloc_string( stream );
  rate->cmode_string = util_fread_alloc_string( stream );
  
  
  fread(&rate->cmode 	 , sizeof rate->cmode 	  , 1 , stream);
  fread(&rate->state 	 , sizeof rate->state 	  , 1 , stream);
  fread(&rate->ORAT  	 , sizeof rate->ORAT  	  , 1 , stream);
  fread(&rate->WRAT     , sizeof rate->WRAT  	  , 1 , stream);
  fread(&rate->GRAT     , sizeof rate->GRAT  	  , 1 , stream);
  fread(&rate->VFPTable , sizeof rate->VFPTable  , 1 , stream);
  fread(&rate->ALift    , sizeof rate->ALift     , 1 , stream);
  fread(&rate->THP	 , sizeof rate->THP 	  , 1 , stream);
  fread(&rate->BHP	 , sizeof rate->BHP 	  , 1 , stream);
  fread(&rate->WGASRAT  , sizeof rate->WGASRAT   , 1 , stream);
  fread(&rate->GOR	 , sizeof rate->GOR 	  , 1 , stream);
  fread(&rate->WCT      , sizeof rate->WCT 	  , 1 , stream);
  fread(&rate->def      , sizeof rate->def       , kw_size , stream);
  
  return rate;
}



static void rate_sched_fprintf(const rate_type * rate , FILE *stream) {
  fprintf(stream , "   ");
  sched_util_fprintf_qst(rate->def[0] , rate->well         , 8 , stream);
  sched_util_fprintf_qst(rate->def[1] , rate->state_string , 4 , stream);
  sched_util_fprintf_qst(rate->def[2] , rate->cmode_string , 4 , stream);
  sched_util_fprintf_dbl(rate->def[3]  , rate->ORAT     , 12 , 3 , stream);
  sched_util_fprintf_dbl(rate->def[4]  , rate->WRAT     , 12 , 3 , stream);
  sched_util_fprintf_dbl(rate->def[5]  , rate->GRAT     , 12 , 3 , stream);
  sched_util_fprintf_int(rate->def[6]  , rate->VFPTable , 4      , stream);
  sched_util_fprintf_dbl(rate->def[7]  , rate->ALift    , 12 , 3 , stream);
  sched_util_fprintf_dbl(rate->def[8]  , rate->THP      , 10 , 4 , stream);
  sched_util_fprintf_dbl(rate->def[9]  , rate->BHP      , 10 , 4 , stream);
  sched_util_fprintf_dbl(rate->def[10] , rate->WGASRAT  , 10 , 4 , stream);
  fprintf(stream , " /\n");
}



static void rate_set_from_string(rate_type * node , int kw_size , const char **token_list) {
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
  
  node->ORAT     = sched_util_atof( token_list[3] );
  node->WRAT     = sched_util_atof( token_list[4] );
  node->GRAT     = sched_util_atof( token_list[5] );
  node->VFPTable = sched_util_atoi( token_list[6] );
  node->ALift    = sched_util_atof( token_list[7] );
  node->THP      = sched_util_atof( token_list[8] );
  node->BHP      = sched_util_atof( token_list[9] );
  node->WGASRAT  = sched_util_atof( token_list[10]);

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
  rate_type *node = rate_alloc_empty(kw_size);
  rate_set_from_string(node ,kw_size , token_list);
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


void sched_kw_wconhist_fwrite(const sched_kw_wconhist_type *kw , FILE *stream) {
  fwrite(&kw->kw_size , sizeof kw->kw_size , 1 , stream);
  {
    int wconhist_lines = list_get_size(kw->rate_list);
    fwrite(&wconhist_lines , sizeof wconhist_lines , 1, stream);
  }
  {
    list_node_type *rate_node = list_get_head(kw->rate_list);
    while (rate_node != NULL) {
      const rate_type * rate = list_node_value_ptr(rate_node);
      rate_sched_fwrite(rate , kw->kw_size , stream);
      rate_node = list_node_get_next(rate_node);
    }
  }
}



sched_kw_wconhist_type * sched_kw_wconhist_fread_alloc(FILE *stream) {
  sched_kw_wconhist_type *kw = sched_kw_wconhist_alloc();
  int lines , i;
  fread(&kw->kw_size , sizeof kw->kw_size , 1 , stream);
  fread(&lines       , sizeof lines       , 1 , stream);
  for (i=0; i < lines; i++) {
    rate_type * rate = rate_sched_fread_alloc(kw->kw_size , stream);
    list_append_list_owned_ref(kw->rate_list , rate , rate_free__);
  } 
  return kw;
}
  









