#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <inttypes.h>
#include <ecl_kw.h>
#include <fortio.h>

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

#define FLIP16(var) (((var >> 8) & 0x00ff) | ((var << 8) & 0xff00))

#define FLIP32(var) (( (var >> 24) & 0x000000ff) | \
		      ((var >>  8) & 0x0000ff00) | \
		      ((var <<  8) & 0x00ff0000) | \
		      ((var << 24) & 0xff000000))

#define FLIP64(var) (((var >> 56)  & 0x00000000000000ff) | \
		      ((var >> 40) & 0x000000000000ff00) | \
		      ((var >> 24) & 0x0000000000ff0000) | \
		      ((var >>  8) & 0x00000000ff000000) | \
		      ((var <<  8) & 0x000000ff00000000) | \
		      ((var << 24) & 0x0000ff0000000000) | \
		      ((var << 40) & 0x00ff000000000000) | \
		      ((var << 56) & 0xff00000000000000))

#define DEBUG 1




struct ecl_kw_struct {
  bool  fmt_file;
  int   size;
  int 	data_size;
  int 	sizeof_ctype;
  int   fmt_linesize;
  int   blocksize;
  bool  endian_convert;
  ecl_type_enum ecl_type;
  char  *header;
  char  *read_fmt, *write_fmt;
  char  *data;
  long  int _start_pos;
};


#define ecl_num_blocksize  1000
#define ecl_char_blocksize  105
#define ecl_type_len  4
#define ecl_Ntype     6

static const char ecl_type_map[ecl_Ntype][ecl_type_len + 1] = {{"CHAR\0"},
							       {"REAL\0"},
							       {"DOUB\0"},
							       {"INTE\0"},
							       {"LOGI\0"},
							       {"MESS\0"}};

/*static const char *ecl_type_map[ecl_Ntype] = {"CHAR\0"},
							       {"REAL\0"},
							       {"DOUB\0"},
							       {"INTE\0"},
							       {"LOGI\0"},
							       {"MESS\0"}};
*/
static const char *ecl_kw_header_write_fmt = " '%-8s' %11d '%-4s'\n";


/******************************************************************/

static void ecl_kw_assert_index(const ecl_kw_type *ecl_kw , int index, const char *caller) {
  if (index < 0 || index >= ecl_kw->size) {
    fprintf(stderr,"Invalid index lookup in:%s aborting \n",caller);
    abort();
  }
}


static const char * __get_ecl_str_type(ecl_type_enum ecl_type) {
  return ecl_type_map[ecl_type];
}


static ecl_type_enum __get_ecl_type(const char *ecl_type_str) {
  if (strlen(ecl_type_str) != ecl_type_len) {
    fprintf(stderr,"Type string: <%s> \n",ecl_type_str);
    fprintf(stderr,"Fatal error in %s eclipse_type:%s not %d characters long - aborting \n",__func__,ecl_type_str, ecl_type_len);
    abort();
  }
  {
    int i, ecl_type;
    ecl_type = -1;
    for (i = 0; i < ecl_Ntype; i++) {
      if (strncmp(ecl_type_str , ecl_type_map[i] , ecl_type_len) == 0)
	 ecl_type = i;
    }
    if (ecl_type == -1) {
      fprintf(stderr," Fatal error: eclipse_type :%s not recognized - aborting \n",ecl_type_str);
      abort();
    }
    return ecl_type;
  }
}


static void ecl_kw_endian_convert_data(ecl_kw_type *ecl_kw) {
  if (ecl_kw->ecl_type != ecl_char_type && ecl_kw->ecl_type != ecl_mess_type) {
    int i;
    switch (ecl_kw->sizeof_ctype) {
    case(1):
      break;
    case(4):
      {
	uint32_t *tmp_int = (uint32_t *) ecl_kw->data;
	for (i=0; i < ecl_kw->size; i++)
	  tmp_int[i] = FLIP32(tmp_int[i]);
	break;
      }
    case(8):
      {
	uint64_t *tmp_int = (uint64_t *) ecl_kw->data;
	for (i=0; i < ecl_kw->size; i++)
	  tmp_int[i] = FLIP64(tmp_int[i]);
	break;
      }
    default:
      fprintf(stderr," sizeof_ctype: %d is not handled in %s - aborting \n",ecl_kw->sizeof_ctype , __func__);
      abort();
    }
  }
}


void ecl_kw_set_fmt_file(ecl_kw_type *ecl_kw , bool fmt_file) {
  ecl_kw->fmt_file = fmt_file;
}

void ecl_kw_select_formatted(ecl_kw_type *ecl_kw) { ecl_kw_set_fmt_file(ecl_kw , true ); }
void ecl_kw_select_binary(ecl_kw_type *ecl_kw) { ecl_kw_set_fmt_file(ecl_kw , false); }

const char * ecl_kw_get_header_ref(const ecl_kw_type *ecl_kw) { return ecl_kw->header; }

void ecl_kw_get_memcpy_data(const ecl_kw_type *ecl_kw , void *target) {
  memcpy(target , ecl_kw->data , ecl_kw->size * ecl_kw->sizeof_ctype);
}

void ecl_kw_set_memcpy_data(ecl_kw_type *ecl_kw , const void *src) {
  memcpy(ecl_kw->data , src , ecl_kw->size * ecl_kw->sizeof_ctype);
}


static bool ecl_kw_string_eq(const char *s1 , const char *s2) {
  const char space_char = ' ';
  const char *long_kw   = (strlen(s1) >= strlen(s2)) ? s1 : s2;
  const char *short_kw  = (strlen(s1) <  strlen(s2)) ? s1 : s2;
  const int  len1       = strlen(long_kw);
  const int  len2       = strlen(short_kw);
  int index;
  bool eq = true;
  if (len1 > ecl_str_len) {
    fprintf(stderr,"%s : eclipse keyword:%s is too long - aborting \n",__func__ , long_kw);
    abort();
  }
  for (index = 0; index < len2; index++)
    eq = eq & (long_kw[index] == short_kw[index]);
  if (eq) {
    for (index = len2; index < len1; index++)
      eq = eq & (long_kw[index] == space_char);
  }
  return eq;
}



bool ecl_kw_ichar_eq(const ecl_kw_type *ecl_kw , int i , const char *value) {
  char s1[ecl_str_len + 1];
  ecl_kw_iget(ecl_kw , i , s1);
  return ecl_kw_string_eq(s1 , value);
}


bool ecl_kw_header_eq(const ecl_kw_type *ecl_kw , const char *kw) {
  return ecl_kw_string_eq(ecl_kw_get_header_ref(ecl_kw) , kw);
}


int ecl_kw_cmp(const ecl_kw_type *ecl_kw1, const ecl_kw_type *ecl_kw2 , int *index) {
  const int header_diff = 1;
  const int data_diff   = 2;
  const int header_header_diff_index = 0;
  const int header_size_diff_index   = 1;
  const int header_type_diff_index   = 2;
  int diff_site;

  diff_site = 0;
  *index    = 0;
  if (strcmp(ecl_kw1->header , ecl_kw2->header) != 0) {
    diff_site = header_diff;
    *index = header_header_diff_index;
  } else if (ecl_kw1->size != ecl_kw2->size) {
    diff_site = header_diff;
    *index = header_size_diff_index;
  } else if (ecl_kw1->ecl_type != ecl_kw2->ecl_type) {
    diff_site = header_diff;
    *index = header_type_diff_index;
  } else {
    int i;
    char *d1, *d2;
    d1 = ecl_kw1->data;
    d2 = ecl_kw2->data;
    
    for (i=0; i < ecl_kw1->size * ecl_kw1->sizeof_ctype; i++) {
      if (d1[i] != d2[i]) {
	printf("byte:%d differs: %03d  %03d  \n",i,d1[i],d2[i]);
	*index = i / ecl_kw1->sizeof_ctype;
	diff_site = data_diff;
	break;
      }
    }
  }
  return diff_site;
}


ecl_kw_type * ecl_kw_alloc_complete(bool fmt_file , bool endian_convert , const char * header ,  int size, const char * ecl_type_str , const void * data) {
  ecl_kw_type *ecl_kw;
  ecl_kw = ecl_kw_alloc_empty(fmt_file , endian_convert);
  ecl_kw_set_header(ecl_kw , header , size , ecl_type_str);
  ecl_kw_alloc_data(ecl_kw);
  ecl_kw_set_memcpy_data(ecl_kw , data);
  return ecl_kw;
}


ecl_kw_type * ecl_kw_alloc_empty(bool fmt_file , bool endian_convert) {
  ecl_kw_type *ecl_kw;

  ecl_kw = malloc(sizeof *ecl_kw);
  ecl_kw->endian_convert = endian_convert;
  ecl_kw->header    = NULL;
  ecl_kw->read_fmt  = NULL;
  ecl_kw->write_fmt = NULL;
  ecl_kw->data 	    = NULL;
  ecl_kw->size         = 0;
  ecl_kw->data_size    = 0;
  ecl_kw->sizeof_ctype = 0;
  ecl_kw_set_fmt_file(ecl_kw , fmt_file);
  return ecl_kw;
}

void ecl_kw_free(ecl_kw_type *ecl_kw) {
  free(ecl_kw->read_fmt);
  free(ecl_kw->write_fmt);
  free(ecl_kw->header);
  free(ecl_kw->data);
  free(ecl_kw);
}


void ecl_kw_memcpy(ecl_kw_type *target, const ecl_kw_type *src) {
  target->fmt_file     	      = src->fmt_file;
  target->size         	      = src->size;
  target->data_size           = src->data_size;
  target->sizeof_ctype 	      = src->sizeof_ctype;
  target->fmt_linesize 	      = src->fmt_linesize;
  target->blocksize           = src->blocksize;
  target->endian_convert      = src->endian_convert;
  target->ecl_type            = src->ecl_type;

  target->read_fmt = realloc(target->read_fmt , strlen(src->read_fmt) + 1);
  strcpy(target->read_fmt , src->read_fmt);

  target->write_fmt = realloc(target->write_fmt , strlen(src->write_fmt) + 1);
  strcpy(target->write_fmt , src->write_fmt);

  target->header = realloc(target->header , strlen(src->header) + 1);
  strcpy(target->header , src->header);

  ecl_kw_alloc_data(target);
  memcpy(target->data , src->data , target->size * target->sizeof_ctype);
}


ecl_kw_type *ecl_kw_alloc_copy(const ecl_kw_type *src) {
  ecl_kw_type *new;
  new = ecl_kw_alloc_empty(true , true);
  ecl_kw_memcpy(new , src);
  return new;
}

static void * ecl_kw_iget_ptr_static(const ecl_kw_type *ecl_kw , int i) {
#ifdef DEBUG
  ecl_kw_assert_index(ecl_kw , i , __func__);
#endif
  return &ecl_kw->data[i * ecl_kw->sizeof_ctype];
}


static void ecl_kw_iget_static(const ecl_kw_type *ecl_kw , int i , void *iptr) {
  memcpy(iptr , ecl_kw_iget_ptr_static(ecl_kw , i) , ecl_kw->sizeof_ctype);  
}

static void ecl_kw_iset_static(ecl_kw_type *ecl_kw , int i , const void *iptr) {
#ifdef DEBUG
  ecl_kw_assert_index(ecl_kw , i , __func__);
#endif
  memcpy(&ecl_kw->data[i * ecl_kw->sizeof_ctype] , iptr, ecl_kw->sizeof_ctype);
}

void ecl_kw_iget(const ecl_kw_type *ecl_kw , int i , void *iptr) { 
  ecl_kw_iget_static(ecl_kw , i , iptr);
}

void * ecl_kw_iget_ptr(const ecl_kw_type *ecl_kw , int i) { 
  return ecl_kw_iget_ptr_static(ecl_kw , i);
}




void ecl_kw_iset(ecl_kw_type *ecl_kw , int i , const void *iptr) { 
  ecl_kw_iset_static(ecl_kw , i , iptr);
}

static void ecl_kw_set_types(ecl_kw_type *ecl_kw, const char *ecl_str_type) {
  ecl_kw->ecl_type = __get_ecl_type(ecl_str_type);
  switch(ecl_kw->ecl_type) {
  case (ecl_char_type):
    /*
      The char type is a somewhat special case because of the
      trailing '\0'; the storage needs to accomodate this, but
      for (unformatted) reading and writing the trailing '\0' 
      is *not* included.
    */
    ecl_kw->sizeof_ctype = sizeof(char) * (ecl_str_len + 1);
    
    ecl_kw->read_fmt = realloc(ecl_kw->read_fmt , 4);
    sprintf(ecl_kw->read_fmt , "%s%s" , "%" , "8c");
    ecl_kw->write_fmt = realloc(ecl_kw->write_fmt , 8);
    sprintf(ecl_kw->write_fmt , " '%s%s'" , "%" , "-8s");
    
    ecl_kw->fmt_linesize = 7;
    ecl_kw->blocksize    = ecl_char_blocksize;
    break;
  case (ecl_float_type):
    ecl_kw->sizeof_ctype = sizeof(float);
    
    ecl_kw->read_fmt = realloc(ecl_kw->read_fmt , 4);
    sprintf(ecl_kw->read_fmt , "%sgE" , "%");
    ecl_kw->write_fmt = realloc(ecl_kw->write_fmt , 19);
    sprintf(ecl_kw->write_fmt , "  %s11.8fE%s+03d" , "%" , "%");
    
    ecl_kw->fmt_linesize = 4;
    ecl_kw->blocksize    = ecl_num_blocksize;
    break;
  case (ecl_double_type):
    ecl_kw->sizeof_ctype = sizeof(double);

    ecl_kw->read_fmt = realloc(ecl_kw->read_fmt , 7);
    sprintf(ecl_kw->read_fmt , "%slgD%sd" , "%" , "%");
    ecl_kw->write_fmt = realloc(ecl_kw->write_fmt , 20);
    sprintf(ecl_kw->write_fmt , "  %s17.14fD%s+03d" , "%" , "%");

    ecl_kw->fmt_linesize = 3;
    ecl_kw->blocksize    = ecl_num_blocksize;
    break;
  case (ecl_int_type):
    ecl_kw->sizeof_ctype = sizeof(int);
    
    ecl_kw->read_fmt = realloc(ecl_kw->read_fmt , 3);
    sprintf(ecl_kw->read_fmt , "%sd" , "%");
    ecl_kw->write_fmt = realloc(ecl_kw->write_fmt , 7);
    sprintf(ecl_kw->write_fmt , " %s11d" , "%");
    
    ecl_kw->fmt_linesize = 6;
    ecl_kw->blocksize    = ecl_num_blocksize;
    break;
  case (ecl_mess_type):
    ecl_kw->sizeof_ctype = sizeof(char);

    ecl_kw->read_fmt = realloc(ecl_kw->read_fmt , 3);
    sprintf(ecl_kw->read_fmt , "%s%s" , "%" , "8c");
    
    ecl_kw->write_fmt = realloc(ecl_kw->write_fmt , 3);
    sprintf(ecl_kw->write_fmt , "%ss" , "%");

    ecl_kw->fmt_linesize = 1;
    ecl_kw->blocksize    = ecl_char_blocksize;
    break;
  case (ecl_bool_type): /* Uncertain of this one ... */
    ecl_kw->sizeof_ctype = sizeof(int);
    
    ecl_kw->read_fmt = realloc(ecl_kw->read_fmt , 3);
    sprintf(ecl_kw->read_fmt , "%sd" , "%");
    ecl_kw->write_fmt = realloc(ecl_kw->write_fmt , 3);
    sprintf(ecl_kw->write_fmt , "%sd" , "%");

    ecl_kw->fmt_linesize = 25;
    ecl_kw->blocksize    = ecl_num_blocksize;
    break;
  default:
    fprintf(stderr,"Internal error in %s - internal eclipse_type: %d not recognized - aborting \n",__func__ , ecl_kw->ecl_type);
    abort();
  }
}



static bool ecl_kw_qskip(FILE *stream) {
  const char sep       = '\'';
  const char space     = ' ';
  const char newline   = '\n';
  const char tab       = '\t';
  bool OK = true;
  char c;
  bool cont = true;
  while (cont) {
    c = fgetc(stream);
    if (c == EOF) {
      cont = false;
      OK   = false;
    } else {
      if (c == space || c == newline || c == tab) 
	cont = true;
      else if (c == sep)
	cont = false;
    }
  }
  return OK;
}  


static bool ecl_kw_fscanf_qstring(char *s , const char *fmt , int len, FILE *stream) {
  const char null_char = '\0';
  char last_sep;
  bool OK;
  OK = ecl_kw_qskip(stream);
  if (OK) {
    fscanf(stream , fmt , s);
    s[len] = null_char;
    fscanf(stream , "%c" , &last_sep);
  }
  return OK;
}

/*
  true  => -1             -1 : 1  11111111  11111111  11111111  1111111
  false =>  0              0 : 0  00000000  00000000  00000000  0000000
*/

void ecl_kw_fread_data(ecl_kw_type *ecl_kw, fortio_type *fortio) {
  const char null_char         = '\0';
  const char char_true         = 84;
  const char char_false        = 70;
  const int  fortran_int_true  = -1;
  const int  fortran_int_false = 0;
  if (ecl_kw->size > 0) {
    if (ecl_kw->fmt_file) {
      const int blocks = ecl_kw->size / ecl_kw->blocksize + (ecl_kw->size % ecl_kw->blocksize == 0 ? 0 : 1);
      FILE *stream = fortio_get_FILE(fortio);
      int offset = 0;
      int index = 0;
      int ib,ir;
      for (ib = 0; ib < blocks; ib++) {
	int read_elm = MIN((ib + 1) * ecl_kw->blocksize , ecl_kw->size) - ib * ecl_kw->blocksize;
	for (ir = 0; ir < read_elm; ir++) {
	  switch(ecl_kw->ecl_type) {
	  case(ecl_char_type):
	    ecl_kw_fscanf_qstring(&ecl_kw->data[offset] , ecl_kw->read_fmt , 8, stream);
	    break;
	  case(ecl_int_type):
	    fscanf(stream , ecl_kw->read_fmt , (int *) &ecl_kw->data[offset]);
	    break;
	  case(ecl_float_type):
	    fscanf(stream , ecl_kw->read_fmt , (float *) &ecl_kw->data[offset]);
	    /*{
	      float arg , value;
	      int    power;
	      fscanf(stream,ecl_kw->read_fmt,&arg , &power);
	      value = arg * expf(logf(10.0) * power);
	      ecl_kw_iset(ecl_kw , index , &value);
	    }
	    */
	    break;
	  case(ecl_double_type):
	    {
	      double arg , value;
	      int    power;
	      fscanf(stream,ecl_kw->read_fmt,&arg , &power);
	      value = arg * exp(log(10.0) * power);
	      ecl_kw_iset(ecl_kw , index , &value);
	    }
	    break;
	  case(ecl_bool_type): 
	    {
	      char bool_char;
	      fscanf(stream , "  %c" , &bool_char);
	      if (bool_char == char_true) 
		ecl_kw_iset(ecl_kw , index , &fortran_int_true);
	      else if (bool_char == char_false)
		ecl_kw_iset(ecl_kw , index , &fortran_int_false);
	      else {
		printf("Logical value: [%c] not recogniced - aborting \n", bool_char);
		exit(1);
	      }
	    }
	    break;
	  case(ecl_mess_type):
	    ecl_kw_fscanf_qstring(&ecl_kw->data[offset] , ecl_kw->read_fmt , 8 , stream);
	    break;
	  }
	  offset += ecl_kw->sizeof_ctype;
	  index++;
	}
      }
    } else {
      const int blocks = ecl_kw->size / ecl_kw->blocksize + (ecl_kw->size % ecl_kw->blocksize == 0 ? 0 : 1);
      int ib;
      for (ib = 0; ib < blocks; ib++) {
	if (ecl_kw->ecl_type == ecl_char_type || ecl_kw->ecl_type == ecl_mess_type) {
	  /* 
	     Due to the necessaary terminating \0 characters there is
	     not a continous file/memory mapping.
	  */
	  int read_elm = MIN((ib + 1) * ecl_kw->blocksize , ecl_kw->size) - ib * ecl_kw->blocksize;
	  FILE *stream = fortio_get_FILE(fortio);
	  int ir;
	  fortio_init_read(fortio);
	  for (ir = 0; ir < read_elm; ir++) {
	    fread(&ecl_kw->data[(ib * ecl_kw->blocksize + ir) * ecl_kw->sizeof_ctype] , 1 , ecl_str_len , stream);
	    ecl_kw->data[(ib * ecl_kw->blocksize + ir) * ecl_kw->sizeof_ctype + ecl_str_len] = null_char;
	  }
	  fortio_complete_read(fortio);
	} else
	  fortio_read_record(fortio , &ecl_kw->data[ib * ecl_kw->blocksize * ecl_kw->sizeof_ctype]);
      }

      if (ecl_kw->endian_convert)
	 ecl_kw_endian_convert_data(ecl_kw);
    }
  }
}


void ecl_kw_rewind(const ecl_kw_type *ecl_kw , fortio_type *fortio) {
  fseek(fortio_get_FILE(fortio) , ecl_kw->_start_pos , SEEK_SET);
}


bool ecl_kw_fread_header(ecl_kw_type *ecl_kw , fortio_type *fortio) {
  const char null_char = '\0';
  FILE *stream = fortio_get_FILE(fortio);
  char header[ecl_str_len + 1];
  char ecl_type_str[ecl_type_len + 1];
  int record_size;
  int size;
  bool OK;

  ecl_kw->_start_pos = ftell(stream);
  if (ecl_kw->fmt_file) {
    OK = ecl_kw_fscanf_qstring(header , "%8c" , 8 , stream); 
    if (OK) {
      fscanf(stream , "%d" , &size);
      ecl_kw_fscanf_qstring(ecl_type_str , "%4c" , 4 , stream);
      fgetc(stream);  /* Reading the trailing newline ... */
    }
  } else {
    header[ecl_str_len]        = null_char;
    ecl_type_str[ecl_type_len] = null_char;
    record_size = fortio_init_read(fortio);
    if (record_size > 0) {
      fread(header       , sizeof(char)   , ecl_str_len  , stream); 
      fread(&size        , sizeof( size ) , 1            , stream); 
      fread(ecl_type_str , sizeof(char)   , ecl_type_len , stream);
      fortio_complete_read(fortio);
      OK = true;
      if (ecl_kw->endian_convert) 
	 size = FLIP32(size);
    } else 
      OK = false;
  }
  if (OK) {
    ecl_kw_set_header(ecl_kw , header , size , ecl_type_str);
  } 
  return OK;
}


void ecl_kw_alloc_data(ecl_kw_type *ecl_kw) {
  char *tmp;
  tmp = realloc(ecl_kw->data , ecl_kw->size * ecl_kw->sizeof_ctype);
  if (tmp == NULL) {
    if (ecl_kw->size * ecl_kw->sizeof_ctype != 0) {
      fprintf(stderr,"Allocation of %d bytes in %s failed - aborting \n",ecl_kw->size * ecl_kw->sizeof_ctype , __func__);
      abort();
    }
  }
  if (ecl_kw->data != tmp) {
    ecl_kw->data  = tmp;
    ecl_kw->data_size = ecl_kw->size;
  }
}





void ecl_kw_free_data(ecl_kw_type *ecl_kw) {
  free(ecl_kw->data);
  ecl_kw->data = NULL;
}



void ecl_kw_set_header(ecl_kw_type *ecl_kw , const char *header ,  int size , const char *ecl_str_type ) {
  ecl_kw->ecl_type = __get_ecl_type(ecl_str_type);
  ecl_kw_set_types(ecl_kw , ecl_str_type);
  if (strlen(header) > ecl_str_len) {
    fprintf(stderr," Fatal error in %s ecl_header_name:%s is longer than eight characters - aborting \n",__func__,header);
    abort();
  }
  ecl_kw->header = realloc(ecl_kw->header , ecl_str_len + 1);
  sprintf(ecl_kw->header , "%-8s" , header);
  ecl_kw->size = size;
}


void ecl_kw_set_header_alloc(ecl_kw_type *ecl_kw , const char *header ,  int size , const char *ecl_str_type ) {
  ecl_kw_set_header(ecl_kw , header , size , ecl_str_type);
  ecl_kw_alloc_data(ecl_kw);
}

bool ecl_kw_fread_realloc(ecl_kw_type *ecl_kw , fortio_type *fortio) {
  bool OK;
  OK = ecl_kw_fread_header(ecl_kw , fortio);
  if (OK) {
    ecl_kw_alloc_data(ecl_kw);
    ecl_kw_fread_data(ecl_kw , fortio);
  } 
  return OK;
}

ecl_kw_type *ecl_kw_fread_alloc(fortio_type *fortio , bool fmt_file , bool endian_convert) {
  bool OK;
  ecl_kw_type *ecl_kw = ecl_kw_alloc_empty(fmt_file , endian_convert);
  OK = ecl_kw_fread_realloc(ecl_kw , fortio);

  if (!OK) {
    free(ecl_kw);
    ecl_kw = NULL;
  }
  printf("Har loadet: %s \n",ecl_kw_get_header_ref(ecl_kw));
  return ecl_kw;
}


void ecl_kw_fskip_data(ecl_kw_type *ecl_kw, fortio_type *fortio) {
 if (ecl_kw->size > 0) {
    if (ecl_kw->fmt_file) {
      if (ecl_kw->data != NULL) 
	ecl_kw_fread_data(ecl_kw , fortio);
      else {
	ecl_kw_alloc_data(ecl_kw);
	ecl_kw_fread_data(ecl_kw , fortio);
	ecl_kw_free_data(ecl_kw);
      }
    } else {
      const int blocks = ecl_kw->size / ecl_kw->blocksize + (ecl_kw->size % ecl_kw->blocksize == 0 ? 0 : 1);
      int ib;
      for (ib = 0; ib < blocks; ib++) 
	fortio_skip_record(fortio);
    }
  }
} 


void ecl_kw_fskip(fortio_type *fortio , bool fmt_file , bool endian_flip) {
  ecl_kw_type *tmp_kw;
  tmp_kw = ecl_kw_fread_alloc(fortio , fmt_file , endian_flip);
  ecl_kw_free(tmp_kw);
}




#define FPRINTF_BLOCK(ecl_kw , elements , tmp_var , stream)                                                        \
 {                                                                                                                 \
    int ib2;                                                                                                       \
    int small_blocks = (elements) / (ecl_kw)->fmt_linesize + (elements % (ecl_kw)->fmt_linesize == 0 ? 0 : 1);     \
    for (ib2 = 0; ib2 < small_blocks; ib2++) {                                                                     \
	 int elements2 = MIN((ib2 + 1)*(ecl_kw)->fmt_linesize , (elements)) - ib2 * (ecl_kw)->fmt_linesize;        \
	 int ie;                                                                                                   \
	 for (ie=0; ie < elements2; ie++) {                                                                        \
	   int index = ib * ecl_kw->blocksize + ib2 * (ecl_kw)->fmt_linesize + ie;                                 \
	    ecl_kw_iget_static((ecl_kw) , (index) , &(tmp_var));                                                   \
	    fprintf((stream) , (ecl_kw)->write_fmt , (tmp_var));                                                   \
	 }                                                                                                         \
	 fprintf(stream , "\n");                                                                                   \
    }                                                                                                              \
 }                                                                                                                


#define FPRINTF_BLOCK_BOOL(ecl_kw , elements , tmp_var , stream)                                                   \
 {                                                                                                                 \
    int ib2;                                                                                                       \
    int small_blocks = (elements) / (ecl_kw)->fmt_linesize + (elements % (ecl_kw)->fmt_linesize == 0 ? 0 : 1);     \
    for (ib2 = 0; ib2 < small_blocks; ib2++) {                                                                     \
	 int elements2 = MIN((ib2 + 1)*(ecl_kw)->fmt_linesize , (elements)) - ib2 * (ecl_kw)->fmt_linesize;        \
	 int ie;                                                                                                   \
	 for (ie=0; ie < elements2; ie++) {                                                                        \
	   int index = ib * ecl_kw->blocksize + ib2 * (ecl_kw)->fmt_linesize + ie;                                 \
	    ecl_kw_iget_static((ecl_kw) , (index) , &(tmp_var));                                                   \
	    if ((tmp_var))                                                                                         \
	       fprintf(stream , "  T");                                                                            \
	    else                                                                                                   \
	       fprintf(stream , "  F");                                                                            \
	 }                                                                                                         \
	 fprintf(stream , "\n");                                                                                   \
    }                                                                                                              \
 }                                                                                                                

static void __set_double_arg(double x , double *_arg_x , int *_pow_x ) {
  if (x != 0.0) {
    double pow_x = ceil(log10(fabs(x)));
    double arg_x   = x / pow(10.0 , pow_x);
    
    if (arg_x == 1.0) {
      arg_x *= 0.10;
      pow_x += 1;
    }
    *_arg_x = arg_x;
    *_pow_x = (int) pow_x;
  } else {
    *_arg_x = 0.0;
    *_pow_x = 0.0;
  }
}

static void __set_float_arg(float x , double *_arg_x , int *_pow_x ) {
  if (x != 0.0) {
    double pow_x = ceil(log10(fabs(x)));
    double arg_x   = x / pow(10.0 , pow_x);
    
    if (arg_x == 1.0) {
      arg_x *= 0.10;
      pow_x += 1;
    }
    *_arg_x = arg_x;
    *_pow_x = (int) pow_x;
  } else {
    *_arg_x = 0.0;
    *_pow_x = 0.0;
  }
}


#define FPRINTF_BLOCK_DOUBLE(ecl_kw , elements , tmp_var , stream)                                                 \
 {                                                                                                                 \
    int ib2;                                                                                                       \
    int small_blocks = (elements) / (ecl_kw)->fmt_linesize + (elements % (ecl_kw)->fmt_linesize == 0 ? 0 : 1);     \
    for (ib2 = 0; ib2 < small_blocks; ib2++) {                                                                     \
	 int elements2 = MIN((ib2 + 1)*(ecl_kw)->fmt_linesize , (elements)) - ib2 * (ecl_kw)->fmt_linesize;        \
	 int ie;                                                                                                   \
	 for (ie=0; ie < elements2; ie++) {                                                                        \
	   int index = ib * ecl_kw->blocksize + ib2 * (ecl_kw)->fmt_linesize + ie;                                 \
           double arg_x;                                                                                           \
	   int    pow_x;                                                                                           \
	    ecl_kw_iget_static((ecl_kw) , (index) , &(tmp_var));                                                   \
	    __set_double_arg((tmp_var) , &arg_x , &pow_x);                                                         \
	    fprintf(stream , ecl_kw->write_fmt , arg_x , pow_x);                                                   \
         }                                                                                                         \
	 fprintf(stream , "\n");                                                                                   \
    }                                                                                                              \
 }                                                                                                                


#define FPRINTF_BLOCK_FLOAT(ecl_kw , elements , tmp_var , stream)                                                  \
 {                                                                                                                 \
    int ib2;                                                                                                       \
    int small_blocks = (elements) / (ecl_kw)->fmt_linesize + (elements % (ecl_kw)->fmt_linesize == 0 ? 0 : 1);     \
    for (ib2 = 0; ib2 < small_blocks; ib2++) {                                                                     \
	 int elements2 = MIN((ib2 + 1)*(ecl_kw)->fmt_linesize , (elements)) - ib2 * (ecl_kw)->fmt_linesize;        \
	 int ie;                                                                                                   \
	 for (ie=0; ie < elements2; ie++) {                                                                        \
	   int index = ib * ecl_kw->blocksize + ib2 * (ecl_kw)->fmt_linesize + ie;                                 \
           double arg_x;                                                                                           \
	   int pow_x;                                                                                              \
	    ecl_kw_iget_static((ecl_kw) , (index) , &(tmp_var));                                                   \
	    __set_float_arg((tmp_var) , &arg_x , &pow_x);                                                          \
	    fprintf(stream , ecl_kw->write_fmt , arg_x , pow_x);                                                   \
         }                                                                                                         \
	 fprintf(stream , "\n");                                                                                   \
    }                                                                                                              \
 }                                                                                                                


static void ecl_kw_fwrite_data(ecl_kw_type *ecl_kw, fortio_type *fortio) {
  const int blocks    = ecl_kw->size / ecl_kw->blocksize + (ecl_kw->size % ecl_kw->blocksize == 0 ? 0 : 1);
  FILE *stream        = fortio_get_FILE(fortio);
  int ib;
  bool local_endian_flip = false;

  if (!ecl_kw->fmt_file) { 
    if (ecl_kw->endian_convert) {
      ecl_kw_endian_convert_data(ecl_kw);
      local_endian_flip = true;
    }
  }
  
  for (ib = 0; ib < blocks; ib++) {
    int elements = MIN((ib + 1)*ecl_kw->blocksize , ecl_kw->size) - ib*ecl_kw->blocksize;
    if (ecl_kw->fmt_file) {
      double tmp_double;
      float  tmp_float;
      int    tmp_int;
      char   tmp_char[ecl_str_len + 1];
      bool   tmp_bool;
      switch (ecl_kw->ecl_type) {
      case(ecl_char_type):
	FPRINTF_BLOCK(ecl_kw , elements , tmp_char , stream);
	break;
      case(ecl_double_type):
	FPRINTF_BLOCK_DOUBLE(ecl_kw , elements , tmp_double , stream);
	break;
      case(ecl_float_type):
	FPRINTF_BLOCK_FLOAT(ecl_kw , elements , tmp_float , stream);
	break;
      case(ecl_int_type):
	FPRINTF_BLOCK(ecl_kw , elements , tmp_int , stream);
	break;
      case (ecl_bool_type):
	FPRINTF_BLOCK_BOOL(ecl_kw , elements , tmp_bool , stream);
	break;
      default:
	 fprintf(stderr,"Internal error in %s - internal eclipse_type: %d not recognized - aborting \n",__func__ , ecl_kw->ecl_type);
	 abort();
      }
    } else {
      int sizeof_ctype = (ecl_kw->ecl_type == ecl_char_type) ? ecl_str_len * sizeof(char) : ecl_kw->sizeof_ctype;
      if (ecl_kw->ecl_type == ecl_char_type || ecl_kw->ecl_type == ecl_mess_type) {
	/* 
	   Due to the necessaary terminating \0 characters there is
	   not a continous file/memory mapping.
	*/
	FILE *stream = fortio_get_FILE(fortio);
	int ir;
	fortio_init_write(fortio , elements * sizeof_ctype);
	for (ir = 0; ir < elements; ir++) 
	  fwrite(&ecl_kw->data[(ib * ecl_kw->blocksize + ir) * ecl_kw->sizeof_ctype] , 1 , ecl_str_len , stream);
	fortio_complete_write(fortio);
      } else
	fortio_write_record(fortio , &ecl_kw->data[ib * ecl_kw->blocksize * ecl_kw->sizeof_ctype] , sizeof_ctype * elements);
    }
  }
  /* Convert back - the in-memory representation should always be "correct". */
  if (local_endian_flip) 
    ecl_kw_endian_convert_data(ecl_kw);
  
}




void ecl_kw_fwrite_header(const ecl_kw_type *ecl_kw , fortio_type *fortio) {
  FILE *stream = fortio_get_FILE(fortio);
  if (ecl_kw->fmt_file) 
    fprintf(stream , ecl_kw_header_write_fmt ,ecl_kw->header , ecl_kw->size, __get_ecl_str_type(ecl_kw->ecl_type));
  else {
    int size = ecl_kw->size;
    if (ecl_kw->endian_convert)
      size = FLIP32(size);

    fortio_init_write(fortio , ecl_str_len + sizeof(int) + ecl_type_len);
    fwrite(ecl_kw->header 			, sizeof(char)    , ecl_str_len  , stream);
    fwrite(&size    			        , sizeof(int)     , 1            , stream);
    fwrite(__get_ecl_str_type(ecl_kw->ecl_type) , sizeof(char)    , ecl_type_len , stream);
    fortio_complete_write(fortio);

  }
}


void ecl_kw_fwrite(ecl_kw_type *ecl_kw , fortio_type *fortio) {
  ecl_kw_fwrite_header(ecl_kw , fortio);
  ecl_kw_fwrite_data(ecl_kw , fortio);
}


void * ecl_kw_get_data_ref(const ecl_kw_type *ecl_kw) {
  return ecl_kw->data;
}

int ecl_kw_get_size(const ecl_kw_type * ecl_kw) {
  return ecl_kw->size;
}

const char * ecl_kw_get_str_type_ref(const ecl_kw_type *ecl_kw) {
  return __get_ecl_str_type(ecl_kw->ecl_type);
}

ecl_type_enum ecl_kw_get_type(const ecl_kw_type * ecl_kw) { return ecl_kw->ecl_type; }


/******************************************************************/

void ecl_kw_cfwrite(const ecl_kw_type * ecl_kw , FILE *stream) {
  fwrite(&ecl_kw->fmt_file     	 , sizeof ecl_kw->fmt_file , 1 , stream);
  fwrite(&ecl_kw->sizeof_ctype 	 , sizeof ecl_kw->sizeof_ctype , 1 , stream);
  fwrite(&ecl_kw->size         	 , sizeof ecl_kw->size , 1 , stream);
  fwrite(&ecl_kw->fmt_linesize 	 , sizeof ecl_kw->fmt_linesize , 1 , stream);
  fwrite(&ecl_kw->blocksize    	 , sizeof ecl_kw->blocksize , 1 , stream);
  fwrite(&ecl_kw->endian_convert , sizeof ecl_kw->endian_convert , 1 , stream);
  fwrite(&ecl_kw->ecl_type       , sizeof ecl_kw->ecl_type   , 1 , stream);
  {
    int tmp_len;
    tmp_len = strlen(ecl_kw->header);
    fwrite(&tmp_len , sizeof tmp_len , 1 , stream); 
    fwrite(ecl_kw->header   , sizeof ecl_kw->header  , tmp_len + 1 , stream);          /* The terminating \0 is included */
    
    fwrite(&tmp_len , sizeof tmp_len , 1 , stream);
    fwrite(ecl_kw->write_fmt   , sizeof ecl_kw->write_fmt  , tmp_len + 1 , stream);    /* The terminating \0 is included */

    fwrite(&tmp_len , sizeof tmp_len , 1 , stream);
    fwrite(ecl_kw->read_fmt   , sizeof ecl_kw->read_fmt  , tmp_len + 1 , stream);      /* The terminating \0 is included */
  }
  {
    int items_written = fwrite(ecl_kw->data , ecl_kw->sizeof_ctype , ecl_kw->size , stream);
    if (items_written != ecl_kw->size) {
      fprintf(stderr,"%s: failed to write all data to disk - aborting. \n",__func__);
      abort();
    }
  }
}



void ecl_kw_cfread(ecl_kw_type * ecl_kw , FILE *stream) {
  fread(&ecl_kw->fmt_file     	 , sizeof ecl_kw->fmt_file , 1 , stream);
  fread(&ecl_kw->sizeof_ctype 	 , sizeof ecl_kw->sizeof_ctype , 1 , stream);
  {
    int file_size;
    fread(&file_size , sizeof file_size , 1 , stream);
    if (file_size != ecl_kw->size) 
      ecl_kw_alloc_data(ecl_kw);
  }
      
  fread(&ecl_kw->fmt_linesize 	 , sizeof ecl_kw->fmt_linesize , 1 , stream);
  fread(&ecl_kw->blocksize    	 , sizeof ecl_kw->blocksize , 1 , stream);
  fread(&ecl_kw->endian_convert  , sizeof ecl_kw->endian_convert , 1 , stream);
  fread(&ecl_kw->ecl_type        , sizeof ecl_kw->ecl_type   , 1 , stream);
  {
    int tmp_len;
    tmp_len = strlen(ecl_kw->header);
    fread(&tmp_len , sizeof tmp_len , 1 , stream);
    ecl_kw->header = realloc(ecl_kw->header , tmp_len + 1);
    fread(ecl_kw->header   , sizeof ecl_kw->header  , tmp_len + 1 , stream);        /* The terminating \0 is included */
    
    fread(&tmp_len , sizeof tmp_len , 1 , stream);
    ecl_kw->write_fmt = realloc(ecl_kw->write_fmt , tmp_len + 1);
    fread(ecl_kw->write_fmt   , sizeof ecl_kw->write_fmt  , tmp_len + 1 , stream);  /* The terminating \0 is included */
    
    fread(&tmp_len , sizeof tmp_len , 1 , stream);
    ecl_kw->read_fmt = realloc(ecl_kw->read_fmt , tmp_len + 1);
    fread(ecl_kw->read_fmt   , sizeof ecl_kw->read_fmt  , tmp_len + 1 , stream);    /* The terminating \0 is included */
  }
  {
    int items_written = fread(ecl_kw->data , sizeof *ecl_kw->data , ecl_kw->size , stream);
    if (items_written != ecl_kw->size) {
      fprintf(stderr,"%s: failed to write all data to disk - aborting. \n",__func__);
      abort();
    }
  }
}
