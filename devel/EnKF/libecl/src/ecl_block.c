#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <fortio.h>
#include <ecl_kw.h>
#include <ecl_block.h>
#include <errno.h>
#include <hash.h>


struct ecl_block_struct {
  bool 	        fmt_file;
  bool 	        endian_convert;
  int           time_step;
  int           size;
  int  	        kw_list_size;  
  hash_type    *kw_hash;
  ecl_kw_type **kw_list;
};


/*ecl_block_type * ecl_block_alloc_shallow_clone(const ecl_block_type *ecl_src) {
  ecl_block_type *new_block = ecl_block_alloc(ecl_src->time_step , ecl_src->size, ecl_src->fmt_file , ecl_src->endian_convert);
  ecl_block_shallow_copy(new_block , ecl_src);
  return new_block;
}
*/


void kw_set_strip_copy(char * copy , const char *src) {
  const char null_char  = '\0';
  const char space_char = ' ';
  int i = 0;
  while (src[i] != null_char && src[i] != space_char) {
    copy[i] = src[i];
    i++;
  }
  copy[i] = null_char;
}

  

ecl_block_type * ecl_block_alloc_copy(const ecl_block_type *src) {
  ecl_block_type * copy;
  copy = ecl_block_alloc(src->time_step , src->size , src->fmt_file , src->endian_convert);
  {
    int i;
    for (i=0; i  < src->size; i++)
      ecl_block_add_kw_copy(copy , src->kw_list[i]);
  }
  return copy;
}


ecl_block_type * ecl_block_alloc(int time_step , int Nkw , bool fmt_file , bool endian_convert) {
  ecl_block_type *ecl_block;
  
  
  ecl_block = malloc(sizeof *ecl_block);
  ecl_block->time_step      = time_step;
  ecl_block->fmt_file       = fmt_file;
  ecl_block->endian_convert = endian_convert;
  ecl_block->size           = 0;
  ecl_block->kw_list_size   = Nkw;
  ecl_block->kw_list        = calloc(Nkw , sizeof(ecl_kw_type *));
  {
    int i;
    for (i=0; i < ecl_block->kw_list_size; i++)
      ecl_block->kw_list[i] = NULL;
  }
  ecl_block->kw_hash = hash_alloc(2*Nkw);
  return ecl_block;
}



bool ecl_block_add_kw(ecl_block_type *ecl_block , const ecl_kw_type *ecl_kw) {
  char kw[9];
  if (ecl_block_get_kw(ecl_block , ecl_kw_get_header_ref(ecl_kw)))
    return false;
  else {
    if (ecl_block->size == ecl_block->kw_list_size) {
      ecl_block->kw_list_size *= 2;
      ecl_block->kw_list = realloc(ecl_block->kw_list , ecl_block->kw_list_size * sizeof(ecl_kw_type *));
    }
    kw_set_strip_copy(kw , ecl_kw_get_header_ref(ecl_kw));
    hash_insert_int(ecl_block->kw_hash , kw , ecl_block->size);
    ecl_block->kw_list[ecl_block->size] = (ecl_kw_type *) ecl_kw;
    ecl_block->size++;
    return true;
  }
}


bool ecl_block_add_kw_copy(ecl_block_type *ecl_block , const ecl_kw_type *src_kw) {
  if (ecl_block_get_kw(ecl_block , ecl_kw_get_header_ref(src_kw)))
    return false;
  else {
    ecl_kw_type *new_kw = ecl_kw_alloc_copy(src_kw);
    ecl_block_add_kw(ecl_block , new_kw);
    return true;
  }
}




void ecl_block_fread(ecl_block_type *ecl_block, fortio_type *fortio , bool *at_eof) {
  ecl_kw_type *ecl_kw = ecl_kw_alloc_empty(ecl_block->fmt_file , ecl_block->endian_convert);
  bool cont;
  cont = true;
  
  while (cont) {
    if (ecl_kw_fread_realloc(ecl_kw , fortio)) {
      if (!ecl_block_add_kw_copy(ecl_block , ecl_kw)) {
	*at_eof = false;
	cont    = false;
	ecl_kw_rewind(ecl_kw , fortio);
      }
    } else {
      cont    = false;
      *at_eof = true;
    }
  }
  
  ecl_kw_free(ecl_kw);
}


static bool ecl_block_include_kw(const ecl_block_type *ecl_block , const ecl_kw_type *ecl_kw , int N_kw, const char **kwlist) {
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


void ecl_block_set_fmt_file(ecl_block_type *ecl_block , bool fmt_file) {
  ecl_block->fmt_file = fmt_file;
  {
    /*
      This could be done with a for loop - it is a good stress test of
      the hash algorithm though.
    */
    hash_node_type *kw_node = hash_iter_init(ecl_block->kw_hash);
    while (kw_node != NULL) {
      ecl_kw_set_fmt_file(ecl_block->kw_list[hash_node_as_int(kw_node)] , ecl_block->fmt_file);
      kw_node = hash_iter_next(ecl_block->kw_hash , kw_node);
    }
  }
}


void ecl_block_select_formatted(ecl_block_type *ecl_block) { ecl_block_set_fmt_file(ecl_block , true ); }
void ecl_block_select_binary(ecl_block_type *ecl_block) { ecl_block_set_fmt_file(ecl_block , false); }


void ecl_block_fread_kwlist(ecl_block_type *ecl_block , fortio_type *fortio , int N_kw, const char **kwlist) {
  ecl_kw_type *ecl_kw  = ecl_kw_alloc_empty(ecl_block->fmt_file , ecl_block->endian_convert);
  hash_type   *kw_hash = hash_alloc(N_kw * 2);
  
  while (ecl_kw_fread_header(ecl_kw , fortio)) {
    if (ecl_block_include_kw(ecl_block, ecl_kw , N_kw , kwlist)) {
      ecl_kw_alloc_data(ecl_kw);
      ecl_kw_fread_data(ecl_kw , fortio);
      ecl_block_add_kw_copy(ecl_block , ecl_kw);
    } else 
      ecl_kw_fskip_data(ecl_kw , fortio);
  }
  ecl_kw_free(ecl_kw);
  hash_free(kw_hash);
}


void ecl_block_fwrite(ecl_block_type *ecl_block , fortio_type *fortio) {
  int ikw;
  for (ikw = 0; ikw < ecl_block->size; ikw++) {
    ecl_kw_set_fmt_file(ecl_block->kw_list[ikw] , ecl_block->fmt_file);
    ecl_kw_fwrite(ecl_block->kw_list[ikw] , fortio);
  }
}


int ecl_block_get_block(const ecl_block_type *ecl_block) {
  return ecl_block->time_step;
}

/*
This is not hash-safe (I think ...)

void ecl_block_shallow_copy(ecl_block_type *target , const ecl_block_type *src) {
  target->fmt_file       = src->fmt_file;
  target->endian_convert = src->endian_convert;
  target->time_step      = src->time_step;
  target->size           = src->size;
  target->kw_list_size   = src->kw_list_size;
  target->kw_list        = realloc(target->kw_list , target->kw_list_size * sizeof (ecl_kw_type *));

    The keywords are *not* memcpy'ed

  {
    int iw;
    for (iw=0; iw < target->kw_list_size; iw++)
      target->kw_list[iw] = src->kw_list[iw];
  }
}
*/


ecl_kw_type * ecl_block_get_kw(const ecl_block_type *ecl_block , const char *kw) {
  ecl_kw_type *ecl_kw = NULL;
  char kw_s[9];
  kw_set_strip_copy(kw_s , kw);  

  if (hash_has_key(ecl_block->kw_hash , kw_s)) {
    const int index = hash_get_int(ecl_block->kw_hash , kw_s);
    ecl_kw = ecl_block->kw_list[index];
  }

  /*int i;
  for (i=0; i < ecl_block->size; i++) {
  if (ecl_kw_header_eq(ecl_block->kw_list[i] , kw)) {
  ecl_kw = ecl_block->kw_list[i];
  break;
  }
  }*/
  
  return ecl_kw;
}


void ecl_block_printf_kwlist(const ecl_block_type *ecl_block) {
  int i;
  printf("block nr: %d \n",ecl_block->time_step);
  for (i=0; i < ecl_block->size; i++) 
    printf("kw[%d] : %s \n",i,ecl_kw_get_header_ref(ecl_block->kw_list[i]));
}


void * ecl_block_get_data_ref(const ecl_block_type *ecl_block, const char *kw) {
  if (ecl_block != NULL) {
    ecl_kw_type *ecl_kw = ecl_block_get_kw(ecl_block , kw);
    if (ecl_kw != NULL)
      return ecl_kw_get_data_ref(ecl_kw);
    else
      return NULL; 
  } else
    return NULL;
}



void ecl_block_free(ecl_block_type *ecl_block) {
  int i;
  hash_free(ecl_block->kw_hash);
  for (i=0; i <ecl_block->size; i++)
    ecl_kw_free(ecl_block->kw_list[i]);
  free(ecl_block->kw_list);
  free(ecl_block);
}


