#include <stdio.h>
#include <string.h>
#include <math.h>
#include <fortio.h>
#include <ecl_kw.h>
#include <ecl_block.h>
#include <errno.h>
#include <hash.h>
#include <util.h>
#include <time.h>
#include <restart_kw_list.h>


#define ECL_BLOCK_ID 6708999

typedef struct ecl_block_node_struct ecl_block_node_type;


struct ecl_block_node_struct {
  int alloc_size;      /* The allocated size of the ecl_kw field - can be different from size. */
  int size;            /* The number of ecl_kw instances added to this node. */
  ecl_kw_type **ecl_kw;
};



struct ecl_block_struct {
  int          __id;             /* Integer identifier used to run-time check a cast. */

  /*
    bool         fmt_file;
    bool         endian_convert;
  */

  /*
    This code is programmed in terms of "report steps", in ECLIPSE
    speak, it has no understanding of the socalled ministeps, and will
    probably break badly if exposed to them.
  */
  
  int           report_nr;
  int           size;

  /*
    NB!

    As of 19.08.2008, kw_hash now contains ecl_block_node_type items,
    *NOT* ecl_kw_type items! This is to support multiple occurences
    of one kw in a single block, e.g. aquifers.
  */


  hash_type             *kw_hash;    /* A hash table of ecl_block_node instances. */
  time_t                 sim_time;
  restart_kw_list_type  *kw_list;    /* A simple list implemented to remember the order of the ecl_kw_instances (ordering is lost in the hash table). */
  char                  *src_file;   /* The src_file this block is loaded from - just for information. */
};




static void ecl_block_node_realloc(ecl_block_node_type * node , int new_alloc_size) {
  node->alloc_size = new_alloc_size;
  node->ecl_kw     = util_realloc(node->ecl_kw , new_alloc_size * sizeof * node->ecl_kw, __func__); 
}


static ecl_block_node_type * ecl_block_node_alloc_empty()
{
  ecl_block_node_type * node = util_malloc(sizeof * node, __func__);
  node->size   = 0;
  node->ecl_kw = NULL;
  ecl_block_node_realloc(node , 1);  /* Default allocates storage for one kw instance.*/
  return node;
}


/**
   Observe that the input ecl_kw instance is *copied* before it is
   inserted.
*/

static void ecl_block_node_add_kw(ecl_block_node_type * node, const ecl_kw_type * ecl_kw)
{
  if (node->alloc_size == node->size)
    ecl_block_node_realloc(node , node->alloc_size * 2);
  node->ecl_kw[node->size] = ecl_kw_alloc_copy(ecl_kw);
  node->size++;
}



static ecl_block_node_type * ecl_block_node_alloc_copy(const ecl_block_node_type * node)
{
  int i;
  ecl_block_node_type * new_node = ecl_block_node_alloc_empty();
  for(i=0; i < node->size; i++)
    ecl_block_node_add_kw(new_node , node->ecl_kw[i]);
  
  return new_node;
}



static void ecl_block_node_free(ecl_block_node_type * node)
{
  int i;

  for(i=0; i<node->size; i++)
    ecl_kw_free(node->ecl_kw[i]);

  free(node->ecl_kw);
}


static void ecl_block_node_free__(void * node)
{
  ecl_block_node_free( (ecl_block_node_type *) node);
}





static int ecl_block_node_get_size(const ecl_block_node_type * node)
{
  return node->size;
}



int ecl_block_get_kw_size(const ecl_block_type * ecl_block, const char * kw)
{
  char * kw_s = util_alloc_strip_copy(kw);
  if(!hash_has_key(ecl_block->kw_hash, kw_s)) {
    ecl_block_summarize(ecl_block);
    util_abort("%s: could not locate kw:[%s / %s] in block - aborting. \n",__func__ , kw , kw_s);
  }
  ecl_block_node_type * node = hash_get(ecl_block->kw_hash, kw);
  free(kw_s);
  return ecl_block_node_get_size(node);
}



bool ecl_block_add_kw(ecl_block_type * ecl_block , const ecl_kw_type * ecl_kw) {
  /*
    Will return true if this is the first time the KW is added.
  */
  char * strip_kw = ecl_kw_alloc_strip_header(ecl_kw);

  if (ecl_block_has_kw(ecl_block , strip_kw)) {
    ecl_block_node_type * node = hash_get(ecl_block->kw_hash, strip_kw);
    ecl_block_node_add_kw(node, ecl_kw);

    restart_kw_list_add(ecl_block->kw_list , strip_kw);
    free(strip_kw);
    return false;

  } else {
    ecl_block_node_type * node = ecl_block_node_alloc_empty();
    ecl_block_node_add_kw(node, ecl_kw);

    hash_insert_hash_owned_ref(ecl_block->kw_hash , strip_kw , node , ecl_block_node_free__);
    ecl_block->size++;

    restart_kw_list_add(ecl_block->kw_list , strip_kw);
    free(strip_kw);
    return true;
  }

}



bool ecl_block_has_kw(const ecl_block_type * ecl_block, const char * kw) {
  return hash_has_key(ecl_block->kw_hash , kw);
}




/*
ecl_kw_type * ecl_block_get_first_kw(const ecl_block_type * src) {
  const char * kw = restart_kw_list_get_first(src->kw_list);
  if (kw != NULL) 
    return ecl_block_get_kw(src , kw);
  else
    return NULL;
}
*/
  


/*
ecl_kw_type * ecl_block_get_next_kw(const ecl_block_type * ecl_block) {
  const char * kw = restart_kw_list_get_next(ecl_block->kw_list);
  if (kw != NULL) 
    return ecl_block_get_kw(ecl_block, kw);
  else
    return NULL;
}
*/



ecl_block_type * ecl_block_alloc_copy(const ecl_block_type *src) {
  ecl_block_type * copy;
  copy = ecl_block_alloc(src->report_nr , src->fmt_file , src->endian_convert);
  hash_lock( src->kw_hash );
  {
    char ** key_list = hash_alloc_keylist(src->kw_hash);
    int i;

    for (i = 0; i < hash_get_size( src->kw_hash ); i++) {
      ecl_block_node_type * node = hash_get(src->kw_hash , key_list[i]);
      ecl_block_node_type * node_copy = ecl_block_node_alloc_copy(node);
      hash_insert_hash_owned_ref(copy->kw_hash , key_list[i] , node_copy , ecl_block_node_free__);
    }
    util_free_stringlist( key_list , hash_get_size( src->kw_hash ));
  }
  hash_unlock( src->kw_hash );
  return copy;
}



void ecl_block_set_sim_time(ecl_block_type * block , time_t sim_time) {
  block->sim_time = sim_time;
}



void ecl_block_set_sim_time_restart(ecl_block_type * block) {
  int *date;
  ecl_kw_type *intehead_kw = ecl_block_iget_kw(block , "INTEHEAD" , 0);
  
  if (intehead_kw == NULL) 
    util_abort("%s: fatal error - could not locate INTEHEAD keyword in restart file - aborting \n",__func__);
  

  date = ecl_kw_iget_ptr(intehead_kw , 64);
  ecl_block_set_sim_time(block , util_make_date(date[0] , date[1] , date[2]));
}



void ecl_block_set_sim_time_summary(ecl_block_type * block , /*int time_index , int years_index , */ int day_index , int month_index , int year_index) {
  float *date;
  ecl_kw_type * param_kw = ecl_block_get_last_kw(block , "PARAMS");
  date = ecl_kw_iget_ptr(param_kw , 0);


  {
    int sec  = 0;
    int min  = 0;
    int hour = 0;

    int day   = roundf(date[day_index]);
    int month = roundf(date[month_index]);
    int year  = roundf(date[year_index]);
    ecl_block_set_sim_time(block , util_make_datetime(sec , min , hour , day , month , year));
  }
}



void ecl_block_set_report_nr(ecl_block_type * block , int report_nr) {
  block->report_nr      = report_nr;
}



int ecl_block_get_report_nr(const ecl_block_type * block) {
  return block->report_nr;
}



time_t ecl_block_get_sim_time(const ecl_block_type * block) {
  return block->sim_time;
}


ecl_block_type * ecl_block_safe_cast(const void * __block) {
  ecl_block_type * ecl_block = (ecl_block_type * ) __block;
  if (ecl_block->__id != ECL_BLOCK_ID)
    util_abort("%s: runtime cast failed - aborting \n",__func__);
  return ecl_block;
}


ecl_block_type * ecl_block_alloc(int report_nr , bool fmt_file , bool endian_convert) {
  ecl_block_type *ecl_block;
  
  
  ecl_block = util_malloc(sizeof *ecl_block , __func__);
  ecl_block->src_file       = NULL;
  ecl_block->fmt_file       = fmt_file;
  ecl_block->endian_convert = endian_convert;
  ecl_block->size           = 0;
  ecl_block->kw_list        = restart_kw_list_alloc();
  ecl_block->__id           = ECL_BLOCK_ID;
  ecl_block->kw_hash  = hash_alloc(10);
  ecl_block->sim_time = -1;
  ecl_block_set_report_nr(ecl_block , report_nr);
  return ecl_block;
}



ecl_block_type * ecl_block_fread_alloc(int report_nr , bool fmt_file , bool endian_convert , fortio_type * fortio, bool *at_eof) {
  ecl_block_type * ecl_block = ecl_block_alloc(report_nr , fmt_file , endian_convert);
  ecl_block_fread(ecl_block , fortio , at_eof);
  return ecl_block;
}



ecl_kw_type * ecl_block_get_kw(const ecl_block_type *ecl_block , const char *kw) {
  char * kw_s = util_alloc_strip_copy(kw);

  if (hash_has_key(ecl_block->kw_hash , kw_s)){ 
    ecl_block_node_type * node = hash_get(ecl_block->kw_hash, kw_s);
    if(node->size > 1){
      util_abort("%s: keyword:%s has more than one occurence, must use ecl_block_iget_kw.\n", __func__ , kw);
      return NULL;
    }
    else{
      free(kw_s);
      return node->ecl_kw[0];
    }
  }
  else {
    ecl_block_summarize(ecl_block);
    util_abort("%s: could not locate kw:[%s/%s] in block - aborting. \n",__func__ , kw , kw_s);
    return NULL;
  }
}



ecl_kw_type * ecl_block_iget_kw(const ecl_block_type * ecl_block, const char * kw, int i)
{
  char * kw_s = util_alloc_strip_copy(kw);

  if (hash_has_key(ecl_block->kw_hash , kw_s)){ 
    ecl_block_node_type * node = hash_get(ecl_block->kw_hash, kw_s);
    if(i >= node->size){
      util_abort("%s: keyword:%s has only %i occurence(s), trying to access number %i (0 based).\n", __func__, kw , node->size, i);
      return NULL;
    }
    else{
      free(kw_s);
      return node->ecl_kw[i];
    }
  }
  else {
    ecl_block_summarize(ecl_block);
    util_abort("%s: could not locate kw:[%s/%s] in block - aborting. \n",__func__ , kw , kw_s);
    return NULL;
  }
}



ecl_kw_type * ecl_block_get_last_kw(const ecl_block_type * ecl_block, const char * kw)
{
  int kw_size = ecl_block_get_kw_size(ecl_block, kw);
  return ecl_block_iget_kw(ecl_block, kw, kw_size-1);
}



restart_kw_list_type * ecl_block_get_restart_kw_list(const ecl_block_type * ecl_block) {
  return ecl_block->kw_list;
}



bool ecl_block_fseek(int istep , bool fmt_file , bool abort_on_error , fortio_type * fortio) {
  if (istep == 0) 
    return true;
  else {
    ecl_kw_type *tmp_kw = ecl_kw_alloc_empty();
    FILE *stream        = fortio_get_FILE(fortio);
    long int init_pos   = ftell(stream);
    char *first_kw;
    int   step_nr;
    bool block_found;
    step_nr = 1;
    
    if (ecl_kw_fread_header(tmp_kw , fmt_file , fortio)) {
      first_kw = util_alloc_string_copy(ecl_kw_get_header_ref(tmp_kw));
      block_found = true;
      do {
        block_found = ecl_kw_fseek_kw(first_kw , fmt_file , false , false , fortio);
        step_nr++;
      } while (block_found && (step_nr < istep));
    } else block_found = false;
      ecl_kw_free(tmp_kw);

    if (!block_found) {
      fseek(stream , init_pos , SEEK_SET);
      if (abort_on_error) 
        util_abort("%s: failed to locate block number:%d - aborting \n",__func__ , istep);
    }
    return block_found;
  }
}



static void ecl_block_set_src_file(ecl_block_type * ecl_block , const char * src_file) {
  ecl_block->src_file = util_realloc_string_copy(ecl_block->src_file , src_file);
}


void ecl_block_fread(ecl_block_type *ecl_block, fortio_type *fortio , bool *_at_eof) {
  ecl_kw_type *ecl_kw    = ecl_kw_alloc_empty();
  bool fmt_file      = ecl_block->fmt_file;
  bool read_next_kw  = true;
  bool is_first_kw   = true;
  bool   at_eof      = false;
  char * first_kw = NULL;
  
  do {
    if (ecl_kw_fread_realloc(ecl_kw , fmt_file , fortio)) {
      if(is_first_kw)
      {
        first_kw = util_alloc_string_copy(ecl_kw_get_header_ref(ecl_kw));
      }

      if (ecl_kw_header_eq(ecl_kw , first_kw) && !is_first_kw)
      {
        at_eof = false;
        read_next_kw = false;
        ecl_kw_rewind(ecl_kw , fortio);
      }
      else 
        ecl_block_add_kw(ecl_block , ecl_kw);

    } else {
      read_next_kw   = false;
      at_eof         = true;
    }

    is_first_kw = false;
  } while(read_next_kw);
  
  util_safe_free(first_kw);
  ecl_kw_free(ecl_kw);
  ecl_block_set_src_file(ecl_block , fortio_filename_ref(fortio));
  if (_at_eof != NULL)
    *_at_eof = at_eof;
}




void ecl_block_summarize(const ecl_block_type * ecl_block) {
  FILE * stream = stdout;
  
  if (ecl_block->src_file != NULL)
    fprintf(stream , "Source file: %s \n", ecl_block->src_file);
  
  fprintf(stream , "-----------------------------------------------------------------\n");

  {
    int i;
    char ** key_list = hash_alloc_keylist(ecl_block->kw_hash);
    hash_lock(ecl_block->kw_hash );
    for(i=0; i<hash_get_size(ecl_block->kw_hash); i++)
    {
      int j;
      int kw_size = ecl_block_get_kw_size(ecl_block, key_list[i]);
      for(j=0; j<kw_size; j++)
      {
        ecl_kw_summarize(ecl_block_iget_kw(ecl_block, key_list[i], j));
      }
    }
    hash_unlock(ecl_block->kw_hash);
    util_free_stringlist(key_list, hash_get_size(ecl_block->kw_hash));
  }  
  fprintf(stream , "-----------------------------------------------------------------\n\n");
}


/*
static bool ecl_block_include_kw(const ecl_kw_type *ecl_kw , int N_kw, const char **kwlist) {
  const char *kw = ecl_kw_get_header_ref(ecl_kw);
  bool inc = false;
  int i;
  
  for (i=0; i < N_kw; i++) {
    if (strcmp(kwlist[i] , kw) == 0) {
      inc = true;
      break;
    }
  }
  return inc;
}
*/



void ecl_block_set_fmt_file(ecl_block_type *ecl_block , bool fmt_file) {
  ecl_block->fmt_file = fmt_file;
}



void ecl_block_select_formatted(ecl_block_type *ecl_block) { ecl_block_set_fmt_file(ecl_block , true ); }
void ecl_block_select_binary(ecl_block_type *ecl_block)    { ecl_block_set_fmt_file(ecl_block , false); }



/*
void ecl_block_fread_kwlist(ecl_block_type *ecl_block , fortio_type *fortio , int N_kw, const char **kwlist) {
  ecl_kw_type *ecl_kw  = ecl_kw_alloc_empty(ecl_block->fmt_file , ecl_block->endian_convert);
  hash_type   *kw_hash = hash_alloc(N_kw * 2);
  
  while (ecl_kw_fread_header(ecl_kw , fortio)) {
    if (ecl_block_include_kw(ecl_kw , N_kw , kwlist)) {
      ecl_kw_alloc_data(ecl_kw);
      ecl_kw_fread_data(ecl_kw , fortio);
      ecl_block_add_kw(ecl_block , ecl_kw);
    } else 
      ecl_kw_fskip_data(ecl_kw , fortio);
  }
  ecl_kw_free(ecl_kw);
  hash_free(kw_hash);
}
*/



void ecl_block_fwrite(ecl_block_type *ecl_block , fortio_type *fortio) {
  hash_type  * kw_counter = hash_alloc();
  bool fmt_file = ecl_block->fmt_file;
  const char * kw;
  restart_kw_list_reset(ecl_block->kw_list);
  kw = restart_kw_list_get_first(ecl_block->kw_list);

  while (kw != NULL) {
    /*
      This is semi-ugly since we need to take care of multiple occurences of some kw's.
    */
    if(!hash_has_key(kw_counter, kw))
    {
      hash_insert_int(kw_counter, kw, 0);
    }
    else
    {
      int counter = hash_get_int(kw_counter, kw);
      counter++;
      hash_insert_int(kw_counter, kw, counter);
    }

    {
      ecl_kw_type * ecl_kw = ecl_block_iget_kw(ecl_block, kw, hash_get_int(kw_counter, kw));
      ecl_kw_fwrite(ecl_kw , fmt_file , fortio);
      kw = restart_kw_list_get_next(ecl_block->kw_list);
    }
  }

  hash_free(kw_counter);
}


/*
void * ecl_block_get_data_ref(const ecl_block_type *ecl_block, const char *kw) {
  if (ecl_block != NULL) {
    if (ecl_block_has_kw(ecl_block , kw)) {
      ecl_kw_type *ecl_kw = ecl_block_get_kw(ecl_block , kw);
      return ecl_kw_get_data_ref(ecl_kw);
    } else
      return NULL; 
  } else
    return NULL;
}
*/



void ecl_block_free(ecl_block_type *ecl_block) {
  hash_free(ecl_block->kw_hash);
  restart_kw_list_free(ecl_block->kw_list);
  if (ecl_block->src_file != NULL) free(ecl_block->src_file);
  free(ecl_block);
}

#undef ECL_BLOCK_ID
