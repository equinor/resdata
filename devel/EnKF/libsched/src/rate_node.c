#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <rate_node.h>
#include <sched_util.h>
#include <string.h>
#include <util.h>


typedef enum {LRAT , ORAT , RESV}  well_control_type;       
typedef enum {OPEN , STOP , SHUT}  well_state_type;
static const double RATE_ERROR = -1.0;

static const int ORAT_index = 3;
static const int WRAT_index = 4;
static const int GRAT_index = 5;
static const int THP_index  = 8;
static const int BHP_index  = 9;


struct rate_struct {
  char             *well;
  char             *state_string;
  char             *cmode_string;
  well_control_type cmode;
  well_state_type   state;   
  double 	    ORAT;         
  double 	    WRAT;    
  double 	    GRAT;    
  int               VFPTable;
  double            ALift;
  double 	    THP;	    
  double 	    BHP;	    
  double            WGASRAT;
  bool             *def;

  int               kw_size;
};


/*****************************************************************/

static rate_type * rate_alloc_empty(int kw_size) {
  rate_type *node    = malloc(sizeof *node);
  node->well         = NULL;
  node->state_string = NULL;
  node->cmode_string = NULL;
  node->def          = calloc(kw_size , sizeof *node->def);
  node->kw_size      = kw_size;
  return node;
}


const char * rate_get_well_ref(const rate_type * rate) {
  return rate->well;
}


void rate_sched_fwrite(const rate_type *rate , FILE *stream) {
  fwrite(&rate->kw_size	 , sizeof rate->kw_size , 1 , stream);

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
  fwrite(rate->def       , sizeof * rate->def     , rate->kw_size , stream);
  
}




rate_type * rate_sched_fread_alloc(FILE *stream) {
  rate_type *rate;
  int kw_size;

  fread(&kw_size  , sizeof kw_size , 1 , stream);
  rate  = rate_alloc_empty(kw_size);
  rate->well         = util_fread_alloc_string( stream ); 
  rate->state_string = util_fread_alloc_string( stream ); 
  rate->cmode_string = util_fread_alloc_string( stream ); 

  fread(&rate->cmode     , sizeof rate->cmode     , 1 , stream);
  fread(&rate->state 	 , sizeof rate->state 	  , 1 , stream);
  fread(&rate->ORAT  	 , sizeof rate->ORAT  	  , 1 , stream);
  fread(&rate->WRAT      , sizeof rate->WRAT  	  , 1 , stream);
  fread(&rate->GRAT      , sizeof rate->GRAT  	  , 1 , stream);
  fread(&rate->VFPTable  , sizeof rate->VFPTable  , 1 , stream);
  fread(&rate->ALift     , sizeof rate->ALift     , 1 , stream);
  fread(&rate->THP	 , sizeof rate->THP 	  , 1 , stream);
  fread(&rate->BHP	 , sizeof rate->BHP 	  , 1 , stream);
  fread(&rate->WGASRAT   , sizeof rate->WGASRAT   , 1 , stream);
  fread(rate->def        , sizeof * rate->def     , rate->kw_size , stream);
  
  return rate;
}




void rate_sched_fprintf(const rate_type * rate , FILE *stream) {
  fprintf(stream , "  ");
  sched_util_fprintf_qst(rate->def[0]  , rate->well          , 8 , stream);
  sched_util_fprintf_qst(rate->def[1]  , rate->state_string  , 4 , stream);
  sched_util_fprintf_qst(rate->def[2]  , rate->cmode_string  , 4 , stream);
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


void rate_sched_fprintf_rates(const rate_type * rate , FILE *stream) {
  fprintf(stream , "  %8s %15.3f %15.3f %15.3f ",rate->well , rate->ORAT , rate->WRAT , rate->GRAT);
  if (rate->def[THP_index])
    fprintf(stream , "%15.3f ",RATE_ERROR);
  else
    fprintf(stream , "%15.3f ",rate->THP);
  if (rate->def[BHP_index])
    fprintf(stream , "%15.3f ",RATE_ERROR);
  else
    fprintf(stream , "%15.3f ",rate->BHP);
  fprintf(stream,"\n");
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

  if (strcmp(node->state_string , "OPEN") == 0) node->state = OPEN;
  if (strcmp(node->state_string , "STOP") == 0) node->state = STOP;
  if (strcmp(node->state_string , "SHUT") == 0) node->state = SHUT;
  
  if (strcmp(node->cmode_string , "RESV") == 0) node->cmode = RESV;
  if (strcmp(node->cmode_string , "LRAT") == 0) node->cmode = LRAT;
  if (strcmp(node->cmode_string , "ORAT") == 0) node->cmode = ORAT;
}



rate_type * rate_alloc(int kw_size , const char **token_list) {
  rate_type *node = rate_alloc_empty(kw_size);
  rate_set_from_string(node ,kw_size , token_list);
  return node;
}


void rate_free(rate_type *rate) {
  free(rate->well);
  free(rate->def);
  free(rate->cmode_string);
  free(rate->state_string);
  free(rate);
}


void rate_free__(void *__rate) {
  rate_type *rate = (rate_type *) __rate;
  rate_free(rate);
}

rate_type * rate_copyc(const rate_type * src) {
  rate_type * new = rate_alloc_empty(src->kw_size);
  new->well = util_alloc_string_copy(src->well);
  new->state_string = util_alloc_string_copy(src->state_string);
  new->cmode_string = util_alloc_string_copy(src->cmode_string); 
  new->cmode = src->cmode;
  new->state = src->state;
  new->ORAT = src->ORAT;         
  new->WRAT = src->WRAT;    
  new->GRAT = src->GRAT;    
  new->VFPTable = src->VFPTable;
  new->ALift = src->ALift;
  new->THP = src->THP;	    
  new->BHP = src->BHP;	    
  new->WGASRAT = src->WGASRAT;
  memcpy(new->def , src->def , new->kw_size * sizeof * new->def);
  return new;
}

const void * rate_copyc__(const void *void_rate) {
  return rate_copyc((const rate_type *) void_rate);
}

/*****************************************************************/

const char * rate_node_get_well_ref(const rate_type * rate) { return rate->well; }

double rate_get_ORAT(const rate_type * rate) {
  return rate->ORAT;
}

double rate_get_GRAT(const rate_type * rate) {
  return rate->GRAT;
}

double rate_get_WRAT(const rate_type * rate) {
  return rate->WRAT;
}


double rate_get_GOR(const rate_type * rate, bool *error) {
  double GOR;
  if (rate->ORAT != 0.0) {
    GOR = rate->GRAT / rate->ORAT;
    *error = false;
  } else {
    GOR = RATE_ERROR;
    *error = true;
  }
  return GOR;
}


double rate_get_WCT(const rate_type * rate, bool *error) {
  double WCT;
  if ((rate->ORAT + rate->WRAT) != 0.0) {
    WCT = rate->WRAT / (rate->ORAT + rate->WRAT);
    *error = false;
  } else {
    WCT = RATE_ERROR;
    *error = true;
  }
  return WCT;
}
