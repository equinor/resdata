#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <fortio.h>
#include <ecl_kw.h>
#include <ecl_block.h>
#include <ecl_fstate.h>
#include <dirent.h>
#include <util.h>


struct ecl_fstate_struct {
  char 	      	 **filelist;
  bool 	      	   fmt_file;
  int              fmt_mode;
  bool 	      	   endian_convert;
  bool        	   unified;
  int         	   files;
  int              N_blocks;
  int              block_size;
  ecl_block_type **block_list;
};


static void fmt_match(const char *filename , const char *substring , bool *fmt) {
  if (strstr(filename , substring) != NULL)
    *fmt = true;
}


static bool ecl_fstate_fmt_name(const char *filename) {
  bool fmt = false;

  fmt_match(filename , ".FSMSPEC" , &fmt);
  fmt_match(filename , ".FUNSMRY" , &fmt);
  fmt_match(filename , ".FUNRST"  , &fmt);
  fmt_match(filename , ".FEGRID"  , &fmt);
  fmt_match(filename , ".FGRID"   , &fmt);
  fmt_match(filename , ".A"       , &fmt);
  fmt_match(filename , ".F"       , &fmt);
  
  return fmt;
}


static bool ecl_fstate_fmt_bit8(const char *filename , int buffer_size) {
  const int min_read = 1024;
  FILE *stream;
  const double bit8set_limit = 0.00001;
  double bit8set_fraction;
  int N_bit8set = 0;
  char *buffer;
  int elm_read,i;
  
  buffer = malloc(buffer_size);
  stream = fopen(filename , "r");
  elm_read = fread(buffer , 1 , buffer_size , stream);
  if (elm_read < min_read) {
    fprintf(stderr,"Error in %s: file:%s is too small to automatically determine formatted/unformatted status \n",__func__ , filename);
    abort();
  }
  for (i=0; i < elm_read; i++)
    N_bit8set += (buffer[i] & (1 << 7)) >> 7;

  fclose(stream);
  free(buffer);

  bit8set_fraction = 1.0 * N_bit8set / elm_read;
  if (bit8set_fraction < bit8set_limit) 
    return true;
  else 
    return false;
}





static int ecl_fstate_fname2time(const char *filename) {
  const char char_X = 'X';
  const char char_F = 'F';
  const char char_S = 'S';
  const char char_A = 'A';
  
  int block;
  char *ext = strrchr(filename , '.');
  if (ext == NULL) {
    fprintf(stderr,"%s: can not determine timestep from filename:%s - aborting \n",__func__ , filename);
    abort();
  }
  
  if (ext[1] == char_X || ext[1] == char_F || ext[1] == char_S || ext[1] == char_A) 
    block = atoi(&ext[2]);
  else {
    fprintf(stderr,"%s: Filename:%s not recognized - valid extensions: Annnn / Xnnnn / Fnnnn / Snnnn - aborting \n",__func__ , filename);
    abort();
  } 
  
  return block;
}


ecl_fstate_type * ecl_fstate_alloc_empty(int fmt_mode , bool endian_convert , bool unified) {
  ecl_fstate_type *ecl_fstate = malloc(sizeof *ecl_fstate);
  ecl_fstate->unified  	      = unified;
  ecl_fstate->fmt_mode 	      = fmt_mode;
  ecl_fstate->endian_convert  = endian_convert;
  ecl_fstate->N_blocks        = 0;
  ecl_fstate->filelist        = NULL;
  ecl_fstate->block_list      = NULL;
  return ecl_fstate;
}


static void __ecl_fstate_set_fmt(ecl_fstate_type *ecl_fstate) {
  const bool existing_fmt = ecl_fstate->fmt_file;
  switch(ecl_fstate->fmt_mode) {
  case ECL_FORMATTED:
    ecl_fstate->fmt_file = true;
    break;
  case ECL_BINARY:
    ecl_fstate->fmt_file = false;
    break;
  case ECL_FMT_AUTO:
    if (util_file_exists(ecl_fstate->filelist[0])) 
      ecl_fstate->fmt_file = ecl_fstate_fmt_bit8(ecl_fstate->filelist[0] , 65536);
    else
      ecl_fstate->fmt_file = ecl_fstate_fmt_name(ecl_fstate->filelist[0]);
    break;
  }
  if (ecl_fstate->fmt_file != existing_fmt) {
    int i;
    for (i=0; i < ecl_fstate->N_blocks; i++)
      ecl_block_set_fmt_file(ecl_fstate->block_list[i] , ecl_fstate->fmt_file);
  }
}

bool ecl_fstate_set_fmt_mode(ecl_fstate_type *ecl_fstate , int fmt_mode) {
  ecl_fstate->fmt_mode = fmt_mode;
  __ecl_fstate_set_fmt(ecl_fstate);
  return ecl_fstate->fmt_mode;
}


void ecl_fstate_add_block(ecl_fstate_type *ecl_fstate , const ecl_block_type *new_block) {
  if (ecl_fstate->N_blocks == ecl_fstate->block_size) {
    ecl_fstate->block_size *= 2;
    ecl_fstate->block_list  = realloc(ecl_fstate->block_list , ecl_fstate->block_size * sizeof *ecl_fstate->block_list);
  }
  ecl_fstate->block_list[ecl_fstate->N_blocks] = (ecl_block_type *) new_block;
  ecl_fstate->N_blocks++;  
}

static void ecl_fstate_init_files(ecl_fstate_type *ecl_fstate , const char *filename1 , int files , const char ** filelist) {
  if (ecl_fstate->unified) {
    ecl_fstate->files = 1;
    ecl_fstate->filelist = calloc(1 , sizeof ecl_fstate->filelist);
    ecl_fstate->filelist[0] = calloc(strlen(filename1) + 1 , 1);
    strcpy(ecl_fstate->filelist[0] , filename1);
  } else {
    int file;
    for (file=0; file < files; file++) {
      ecl_fstate->filelist[file] = calloc(strlen(filelist[file]) + 1 , 1);
      strcpy(ecl_fstate->filelist[file] , filelist[file]);
    }
  }
  __ecl_fstate_set_fmt(ecl_fstate);
}


void ecl_fstate_set_unified_file(ecl_fstate_type *ecl_fstate, const char *filename1) {
  ecl_fstate_init_files(ecl_fstate , filename1 , 0 , NULL);
}

void ecl_fstate_set_unified(ecl_fstate_type *ecl_fstate , bool unified) {
  ecl_fstate->unified = unified;
}

void ecl_fstate_set_multiple_files(ecl_fstate_type *ecl_fstate, const char * basename , const char *ext) {
  char **filelist;
  int i;
  filelist = calloc(ecl_fstate->N_blocks , sizeof(char *));

  for (i=0; i < ecl_fstate->N_blocks; i++) {
    filelist[i] = calloc(strlen(basename) + strlen(ext) + 1 + 1 + 4 , sizeof(char));
    sprintf(filelist[i] , "%s.%s%04d" , basename , ext , ecl_block_get_block(ecl_fstate->block_list[i]));
  }
  ecl_fstate_init_files(ecl_fstate , NULL  , ecl_fstate->N_blocks , (const char **) filelist);
  
  for (i=0; i < ecl_fstate->N_blocks; i++) 
    free(filelist[i]);
  free(filelist);
}

static ecl_fstate_type * ecl_fstate_load_static(const char *filename1 , int files , const char ** filelist , int fmt_mode , bool endian_convert , bool unified) {
  ecl_fstate_type *ecl_fstate = ecl_fstate_alloc_empty(fmt_mode , endian_convert , unified);
  ecl_fstate->block_size  = 10;
  ecl_fstate->block_list  = calloc(ecl_fstate->block_size , sizeof *ecl_fstate->block_list);
  ecl_fstate_init_files(ecl_fstate , filename1 , files , filelist);
  if (unified) {
    fortio_type *fortio = fortio_open(ecl_fstate->filelist[0] , "r" , ecl_fstate->endian_convert);
    bool at_eof = false;
    int block_nr    = 0;
    while (!at_eof) {
      ecl_block_type *ecl_block = ecl_block_alloc(block_nr , 10 , ecl_fstate->fmt_file , ecl_fstate->endian_convert);
      ecl_block_fread(ecl_block , fortio , &at_eof);
      ecl_fstate_add_block(ecl_fstate , ecl_block);
      block_nr++;
    }
    fortio_close(fortio);
  } else {
    ecl_fstate->files = files;
    ecl_fstate->filelist = calloc(files , sizeof ecl_fstate->filelist);
    {
      int file;
      for (file=0; file < files; file++) {
	bool at_eof;
	ecl_block_type *ecl_block = ecl_block_alloc(file , 10 , ecl_fstate->fmt_file , ecl_fstate->endian_convert);
	fortio_type *fortio       = fortio_open(ecl_fstate->filelist[file] , "r" , ecl_fstate->endian_convert);
	ecl_block_fread(ecl_block , fortio , &at_eof);
	ecl_fstate_add_block(ecl_fstate , ecl_block);
	fortio_close(fortio);
      }
    }
  }
  return ecl_fstate;
}





int ecl_fstate_scandir(const char *path , const char *glob , char ***_filelist) {
  const char slash   = '/';
  struct dirent **namelist;
  bool add_slash;
  char *slash_ptr;
  char **filelist;
  int files,i;

  slash_ptr = strrchr(path , slash);
  if (slash_ptr == NULL) 
    add_slash = true;
  else {
    if (strlen(slash_ptr) != 1)
      add_slash  = true;
    else
      add_slash = false;
  }

  files = scandir(path , &namelist , NULL , NULL);
  if (files > 0) {
    filelist = calloc(files , sizeof(char **));
    for (i=0; i < files; i++) {
      
      if (add_slash) {
	filelist[i] = calloc(strlen(namelist[i]->d_name) + strlen(path) + 2 , sizeof(char));
	sprintf(filelist[i] , "%s/%s" , path , namelist[i]->d_name);
      } else {
	filelist[i] = calloc(strlen(namelist[i]->d_name) + strlen(path) + 1 , sizeof(char));
	sprintf(filelist[i] , "%s%s" , path , namelist[i]->d_name);
      }
      free(namelist[i]);
   }
    free(namelist);
    *_filelist = filelist;
  } 
  return files;
}



void ecl_fstate_scandir_free(int files , char **filelist) {
  int i;
  for (i=0; i < files; i++)
    free(filelist[i]);
  free(filelist);
}



ecl_fstate_type * ecl_fstate_load_unified(const char *filename , int fmt_mode , bool endian_convert) {
  ecl_fstate_type *ecl_fstate;
  ecl_fstate = ecl_fstate_load_static(filename , 1 , NULL , fmt_mode , endian_convert , true);
  return ecl_fstate;
}


ecl_fstate_type * ecl_fstate_load_multiple(int files , const char **filelist , int fmt_mode , bool endian_convert) {
  ecl_fstate_type *ecl_fstate;
  ecl_fstate = ecl_fstate_load_static(NULL , files , filelist , fmt_mode , endian_convert , false);
  return ecl_fstate;
}




ecl_block_type * ecl_fstate_get_block(const ecl_fstate_type * ecl_fstate , int istep) {
  ecl_block_type *block = NULL;
  int i;

  
  if (istep < ecl_fstate->N_blocks) {
    block = ecl_fstate->block_list[istep];
    if (ecl_block_get_block(block) == istep) 
      return block;
  }
  
  /* else iterate through the list ... */

  for (i = 0; i < ecl_fstate->N_blocks; i++) {
    ecl_block_type *tmp_block = ecl_fstate->block_list[i];
    if (ecl_block_get_block(block) == istep) {
      block = tmp_block;
    }
  }
  return block;
}

ecl_kw_type  * ecl_fstate_get_kw(const ecl_fstate_type * ecl_fstate , int istep , const char *kw) {
  ecl_block_type * ecl_block = ecl_fstate_get_block(ecl_fstate , istep);
  if (ecl_block == NULL) 
    return NULL;
  else 
    return ecl_block_get_kw(ecl_block , kw);
}


int ecl_fstate_kw_get_size(const ecl_fstate_type * ecl_fstate , int istep , const char *kw) {
  ecl_kw_type *ecl_kw = ecl_fstate_get_kw(ecl_fstate , istep , kw);
  if (ecl_kw == NULL) 
    return 0;
  else 
    return ecl_kw_get_size(ecl_kw);
} 

bool ecl_fstate_kw_get_memcpy_data(const ecl_fstate_type * ecl_fstate , int istep , const char *kw , void *data) {
  ecl_kw_type *ecl_kw = ecl_fstate_get_kw(ecl_fstate , istep , kw);
  if (ecl_kw == NULL) 
    return false;
  else {
    ecl_kw_get_memcpy_data(ecl_kw , data);
    return true;
  }
}


bool ecl_fstate_kw_iget(const ecl_fstate_type * ecl_fstate , int istep , const char *kw , int iw , void *value) {
  ecl_kw_type *ecl_kw = ecl_fstate_get_kw(ecl_fstate , istep , kw);
  if (ecl_kw == NULL) 
    return false;
  else {
    ecl_kw_iget(ecl_kw , iw , value);
    return true;
  }
}


bool ecl_fstate_kw_exists(const ecl_fstate_type *ecl_fstate , int istep , const char *kw) {
  ecl_kw_type *ecl_kw = ecl_fstate_get_kw(ecl_fstate , istep , kw);
  if (ecl_kw == NULL) 
    return false;
  else 
    return true;
}


int ecl_fstate_get_blocksize(const ecl_fstate_type *ecl_fstate) {
  return ecl_fstate->N_blocks;
}



void ecl_fstate_free(ecl_fstate_type *ecl_fstate) {
  int i;
  for (i=0; i <ecl_fstate->files; i++)
    free(ecl_fstate->filelist[i]);
  free(ecl_fstate->filelist);

  for (i=0; i <ecl_fstate->N_blocks; i++)
    ecl_block_free(ecl_fstate->block_list[i]);
  free(ecl_fstate->block_list);

  free(ecl_fstate);
}


static void ecl_fstate_save_multiple(const ecl_fstate_type *ecl_fstate) {
  int block;
  for (block = 0; block < ecl_fstate->N_blocks; block++) {
    fortio_type *fortio = fortio_open(ecl_fstate->filelist[block] , "w" , ecl_fstate->endian_convert);
    ecl_block_fwrite(ecl_fstate->block_list[block] , fortio);
    fortio_close(fortio);
  }
}

static void ecl_fstate_save_unified(const ecl_fstate_type *ecl_fstate) {
  int block;
  fortio_type *fortio = fortio_open(ecl_fstate->filelist[0] , "w" , ecl_fstate->endian_convert);
  for (block = 0; block < ecl_fstate->N_blocks; block++) 
    ecl_block_fwrite(ecl_fstate->block_list[block] , fortio);
  fortio_close(fortio);
}

void ecl_fstate_save(const ecl_fstate_type *ecl_fstate) {
  if (ecl_fstate->unified)
    ecl_fstate_save_unified(ecl_fstate);
  else
    ecl_fstate_save_multiple(ecl_fstate);
}



/* void ecl_fstate_set_filename(ecl_fstate_type *ecl_fstate , const char *filename) { */
/*   ecl_fstate->filename = malloc(strlen(filename)+1); */
/*   strcpy(ecl_fstate->filename , filename);   */
/* } */


/* ecl_fstate_type * ecl_fstate_alloc(const char *filename , int Nkw , int fmt_mode , bool endian_convert) { */
/*   ecl_fstate_type *ecl_fstate; */
/*   bool fmt_file; */
  
/*   if (fmt_mode == 0) { */
/*     if (file_exists(filename)) */
/*       fmt_file = ecl_fstate_fmt(filename , 16384); */
/*     else { */
/*       fprintf(stderr,"Error in %s - can *not* be called with fmt_mode == 0 for nonexisting file \n",__func__); */
/*       abort(); */
/*     } */
/*   } else if (fmt_mode > 0) */
/*     fmt_file = true; */
/*   else */
/*     fmt_file = false; */
  
/*   ecl_fstate = malloc(sizeof *ecl_fstate); */
/*   ecl_fstate_set_filename(ecl_fstate , filename); */
/*   ecl_fstate->fmt_file       = fmt_file; */
/*   ecl_fstate->endian_convert = endian_convert; */
/*   ecl_fstate->size           = 0; */
/*   ecl_fstate->kw_list_size   = Nkw; */
/*   ecl_fstate->kw_list        = calloc(Nkw , sizeof(ecl_kw_type *)); */
/*   ecl_fstate->unified        = false; */
/*   { */
/*     int i; */
/*     for (i=0; i < ecl_fstate->kw_list_size; i++) */
/*       ecl_fstate->kw_list[i] = NULL; */
/*   } */
  
/*   return ecl_fstate; */
/* } */


/* void ecl_fstate_add_kw(ecl_fstate_type *ecl_fstate , const ecl_kw_type *ecl_kw) { */
/*   if (ecl_fstate->size == ecl_fstate->kw_list_size) { */
/*     ecl_fstate->kw_list_size *= 2; */
/*     ecl_fstate->kw_list = realloc(ecl_fstate->kw_list , ecl_fstate->kw_list_size * sizeof(ecl_kw_type *)); */
/*   } */
/*   ecl_fstate->kw_list[ecl_fstate->size] = (ecl_kw_type *) ecl_kw; */
/*   ecl_fstate->size++; */
/* } */


/* void ecl_fstate_add_kw_copy(ecl_fstate_type *ecl_fstate , const ecl_kw_type *src_kw) { */
/*   ecl_kw_type *new_kw; */
/*   new_kw = ecl_kw_alloc_clone(src_kw); */
/*   ecl_fstate_add_kw(ecl_fstate , new_kw); */
/* } */


/* void ecl_fstate_load(ecl_fstate_type *ecl_fstate, int verbosity) { */
/*   ecl_kw_type *ecl_kw = ecl_kw_alloc_empty(ecl_fstate->fmt_file , ecl_fstate->endian_convert); */
/*   fortio_type *fortio = fortio_open(ecl_fstate->filename , "r" , ecl_fstate->endian_convert); */
/*   if (verbosity >= 1) */
/*     printf("Loading:%s \n",ecl_fstate->filename); */

/*   while (ecl_kw_fread_realloc(ecl_kw , fortio)) { */
/*     ecl_fstate_add_kw_copy(ecl_fstate , ecl_kw); */
/*     if (verbosity >= 2)  */
/*       printf("Loading: %s/%s \n",ecl_fstate->filename , ecl_kw_get_header_ref(ecl_kw)); */
    
/*   } */
  
/*   ecl_kw_free(ecl_kw); */
/*   fortio_close(fortio); */
/* } */


/* static bool ecl_fstate_include_kw(const ecl_fstate_type *ecl_fstate , const ecl_kw_type *ecl_kw , int N_kw, const char **kwlist) { */
/*   const char *kw = ecl_kw_get_header_ref(ecl_kw); */
/*   bool inc = false; */
/*   int i; */
  
/*   for (i=0; i < N_kw; i++) { */
/*     if (strcmp(kwlist[i] , kw) == 0) { */
/*       inc = true; */
/*       break; */
/*     } */
/*   } */
/*   return inc; */
/* } */


/* void ecl_fstate_set_fmt_file(ecl_fstate_type *ecl_fstate , bool fmt_file) { */
/*   ecl_fstate->fmt_file = fmt_file; */
/* } */

/* void ecl_fstate_select_formatted(ecl_fstate_type *ecl_fstate) { ecl_fstate_set_fmt_file(ecl_fstate , true ); } */
/* void ecl_fstate_select_binary(ecl_fstate_type *ecl_fstate) { ecl_fstate_set_fmt_file(ecl_fstate , false); } */


/* void ecl_fstate_load_kwlist(ecl_fstate_type *ecl_fstate , int N_kw, const char **kwlist) { */
/*   ecl_kw_type *ecl_kw = ecl_kw_alloc_empty(ecl_fstate->fmt_file , ecl_fstate->endian_convert); */
/*   fortio_type *fortio = fortio_open(ecl_fstate->filename , "r"  , ecl_fstate->endian_convert); */

/*   while (ecl_kw_fread_header(ecl_kw , fortio)) { */
/*     if (ecl_fstate_include_kw(ecl_fstate, ecl_kw , N_kw , kwlist)) { */
/*       ecl_kw_alloc_data(ecl_kw); */
/*       ecl_kw_fread_data(ecl_kw , fortio); */
/*       ecl_fstate_add_kw_copy(ecl_fstate , ecl_kw); */
/*     } else  */
/*       ecl_kw_fskip_data(ecl_kw , fortio); */
/*   } */
/*   ecl_kw_free(ecl_kw); */
/*   fortio_close(fortio); */
/* } */


/* void ecl_fstate_fwrite(ecl_fstate_type *ecl_fstate) { */
/*   fortio_type *fortio = fortio_open(ecl_fstate->filename , "w" , ecl_fstate->endian_convert); */
/*   int ikw; */
/*   for (ikw = 0; ikw < ecl_fstate->size; ikw++) { */
/*     ecl_kw_set_fmt_file(ecl_fstate->kw_list[ikw] , ecl_fstate->fmt_file); */
/*     ecl_kw_fwrite(ecl_fstate->kw_list[ikw] , fortio); */
/*   } */
/*   fortio_close(fortio); */
/* } */



/* ecl_kw_type * ecl_fstate_get_kw(const ecl_fstate_type *ecl_fstate , const char *kw) { */
/*   int i; */
/*   ecl_kw_type *ecl_kw = NULL; */
/*   for (i=0; i < ecl_fstate->size; i++) { */
/*     if (strcmp(kw , ecl_kw_get_header_ref(ecl_fstate->kw_list[i])) == 0) { */
/*       ecl_kw = ecl_fstate->kw_list[i]; */
/*       break; */
/*     } */
/*   } */
/*   return ecl_kw; */
/* } */


/* void * ecl_fstate_get_data_ref(const ecl_fstate_type *ecl_fstate, const char *kw) { */
/*   if (ecl_fstate != NULL) { */
/*     ecl_kw_type *ecl_kw = ecl_fstate_get_kw(ecl_fstate , kw); */
/*     if (ecl_kw != NULL) */
/*       return ecl_kw_get_data_ref(ecl_kw); */
/*     else */
/*       return NULL;  */
/*   } else */
/*     return NULL; */
/* } */






