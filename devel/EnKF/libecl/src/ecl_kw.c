#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <inttypes.h>
#include <ecl_kw.h>
#include <ecl_util.h>
#include <fortio.h>
#include <util.h>


#define DEBUG 1

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))



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
  bool  shared_data;
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


/*
  static const char *ecl_type_map[] = {"CHAR","REAL","DOUB","INTE","LOGI","MESS"};
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

/*
  const char * ecl_kw_str_type(ecl_type_enum ecl_type) { return __get_ecl_str_type(ecl_type); }
*/


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
      fprintf(stderr,"%s: Fatal error: eclipse_type :%s not recognized - aborting \n",__func__ , ecl_type_str);
      abort();
    }
    return ecl_type;
  }
}



static void ecl_kw_endian_convert_data(ecl_kw_type *ecl_kw) {
  if (ecl_kw->ecl_type != ecl_char_type && ecl_kw->ecl_type != ecl_mess_type) 
    util_endian_flip_vector(ecl_kw->data , ecl_kw->sizeof_ctype , ecl_kw->size);
}


void ecl_kw_set_fmt_file(ecl_kw_type *ecl_kw , bool fmt_file) {
  ecl_kw->fmt_file = fmt_file;
}

bool ecl_kw_get_fmt_file(const ecl_kw_type *ecl_kw) {
  return ecl_kw->fmt_file;
}

void ecl_kw_select_formatted(ecl_kw_type *ecl_kw) { ecl_kw_set_fmt_file(ecl_kw , true ); }
void ecl_kw_select_binary(ecl_kw_type *ecl_kw) { ecl_kw_set_fmt_file(ecl_kw , false); }

const char * ecl_kw_get_header_ref(const ecl_kw_type *ecl_kw) { return ecl_kw->header; }

char * ecl_kw_alloc_strip_header(const ecl_kw_type *ecl_kw) {
  return util_alloc_strip_copy(ecl_kw->header);
}

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

void ecl_kw_set_shared_ref(ecl_kw_type * ecl_kw , void *data_ptr) {
  if (!ecl_kw->shared_data) {
    fprintf(stderr,"%s: trying to set shared data reference for ecl_kw object which has been allocated with private storage - aborting \n",__func__);
    abort();
  }
  ecl_kw->data = data_ptr;
}


static void ecl_kw_set_shared(ecl_kw_type * ecl_kw) {
  if (!ecl_kw->shared_data) {
    if (ecl_kw->data != NULL) {
      fprintf(stderr,"%s: can not change to shared for keyword with allocated storage - aborting \n",__func__);
      abort();
    }
  }
  ecl_kw->shared_data = true;
}




ecl_kw_type * ecl_kw_alloc_complete(bool fmt_file , bool endian_convert , const char * header ,  int size, ecl_type_enum ecl_type , const void * data) {
  ecl_kw_type *ecl_kw;
  ecl_kw = ecl_kw_alloc_empty(fmt_file , endian_convert);
  ecl_kw_set_header(ecl_kw , header , size , __get_ecl_str_type(ecl_type));
  ecl_kw_alloc_data(ecl_kw);
  ecl_kw_set_memcpy_data(ecl_kw , data);
  return ecl_kw;
}



ecl_kw_type * ecl_kw_alloc_complete_shared(bool fmt_file , bool endian_convert , const char * header ,  int size, ecl_type_enum ecl_type , void * data) {
  ecl_kw_type *ecl_kw;
  ecl_kw = ecl_kw_alloc_empty(fmt_file , endian_convert);
  ecl_kw_set_header(ecl_kw , header , size , __get_ecl_str_type(ecl_type));
  ecl_kw_set_shared(ecl_kw);
  ecl_kw_set_shared_ref(ecl_kw , data);
  return ecl_kw;
}



ecl_kw_type * ecl_kw_alloc_empty(bool fmt_file , bool endian_convert) {
  ecl_kw_type *ecl_kw;

  ecl_kw = malloc(sizeof *ecl_kw);
  ecl_kw->endian_convert = endian_convert;
  ecl_kw->header       = NULL;
  ecl_kw->read_fmt     = NULL;
  ecl_kw->write_fmt    = NULL;
  ecl_kw->data 	       = NULL;
  ecl_kw->shared_data  = false;
  ecl_kw->size         = 0;
  ecl_kw->data_size    = 0;
  ecl_kw->sizeof_ctype = 0;
  ecl_kw_set_fmt_file(ecl_kw , fmt_file);
  
  return ecl_kw;

}



void ecl_kw_free(ecl_kw_type *ecl_kw) {
  if (ecl_kw->read_fmt != NULL) free(ecl_kw->read_fmt);
  free(ecl_kw->write_fmt);
  free(ecl_kw->header);
  if (!ecl_kw->shared_data) free(ecl_kw->data);
  free(ecl_kw);
  
}

void ecl_kw_free__(void *void_ecl_kw) {
  ecl_kw_free((ecl_kw_type *) void_ecl_kw);
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

  if (src->read_fmt == NULL)
    target->read_fmt = NULL; 
  else {
    target->read_fmt = realloc(target->read_fmt , strlen(src->read_fmt) + 1);
    strcpy(target->read_fmt , src->read_fmt);
  }

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

const void * ecl_kw_copyc__(const void * void_kw) {
  return ecl_kw_alloc_copy((const ecl_kw_type *) void_kw); 
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


#define ECL_KW_IGET_TYPED(type)                                						    \
                                                                                                            \
type ecl_kw_iget_ ## type(const ecl_kw_type * ecl_kw, int i) { 						    \
  type value;                                                  						    \
  if (ecl_kw_get_type(ecl_kw) != ecl_ ## type ## _type) {            						    \
    fprintf(stderr,"%s: Keyword: %s is wrong type - aborting \n",__func__ , ecl_kw_get_header_ref(ecl_kw)); \
    abort();                                                                                                \
  }                                                                                                         \
  ecl_kw_iget_static(ecl_kw , i , &value);                                                                  \
 return value;                                                                                              \
}                                                                                                           \

ECL_KW_IGET_TYPED(double);
ECL_KW_IGET_TYPED(float);
ECL_KW_IGET_TYPED(int);

#undef ECL_KW_IGET_TYPED

void * ecl_kw_iget_ptr(const ecl_kw_type *ecl_kw , int i) { 
  return ecl_kw_iget_ptr_static(ecl_kw , i);
}




void ecl_kw_iset(ecl_kw_type *ecl_kw , int i , const void *iptr) { 
  ecl_kw_iset_static(ecl_kw , i , iptr);
}

static void ecl_kw_init_types(ecl_kw_type *ecl_kw, ecl_type_enum ecl_type) {
  ecl_kw->ecl_type = ecl_type;
  ecl_kw->sizeof_ctype = ecl_util_get_sizeof_ctype(ecl_kw->ecl_type);
  
  switch(ecl_kw->ecl_type) {
  case (ecl_char_type):
    /*
      The char type is a somewhat special case because of the
      trailing '\0'; the storage needs to accomodate this, but
      for (unformatted) reading and writing the trailing '\0' 
      is *not* included.
    */
    
    ecl_kw->read_fmt = realloc(ecl_kw->read_fmt , 4);
    sprintf(ecl_kw->read_fmt , "%s%s" , "%" , "8c");
    ecl_kw->write_fmt = realloc(ecl_kw->write_fmt , 8);
    sprintf(ecl_kw->write_fmt , " '%s%s'" , "%" , "-8s");
    
    ecl_kw->fmt_linesize = 7;
    ecl_kw->blocksize    = ecl_char_blocksize;
    break;
  case (ecl_float_type):
    ecl_kw->read_fmt = realloc(ecl_kw->read_fmt , 4);
    sprintf(ecl_kw->read_fmt , "%sgE" , "%");
    ecl_kw->write_fmt = realloc(ecl_kw->write_fmt , 19);
    sprintf(ecl_kw->write_fmt , "  %s11.8fE%s+03d" , "%" , "%");
    
    ecl_kw->fmt_linesize = 4;
    ecl_kw->blocksize    = ecl_num_blocksize;
    break;
  case (ecl_double_type):
    /*
      The read_fmt variable is not set,
      because the formatted double variables
      are in a two-step process using strtod().
    */
    ecl_kw->write_fmt = realloc(ecl_kw->write_fmt , 20);
    sprintf(ecl_kw->write_fmt , "  %s17.14fD%s+03d" , "%" , "%");

    ecl_kw->fmt_linesize = 3;
    ecl_kw->blocksize    = ecl_num_blocksize;
    break;
  case (ecl_int_type):
    ecl_kw->read_fmt = realloc(ecl_kw->read_fmt , 3);
    sprintf(ecl_kw->read_fmt , "%sd" , "%");
    ecl_kw->write_fmt = realloc(ecl_kw->write_fmt , 7);
    sprintf(ecl_kw->write_fmt , " %s11d" , "%");
    
    ecl_kw->fmt_linesize = 6;
    ecl_kw->blocksize    = ecl_num_blocksize;
    break;
  case (ecl_mess_type):
    ecl_kw->read_fmt = realloc(ecl_kw->read_fmt , 4);
    sprintf(ecl_kw->read_fmt , "%s%s" , "%" , "8c");
    
    ecl_kw->write_fmt = realloc(ecl_kw->write_fmt , 4);
    sprintf(ecl_kw->write_fmt , "%ss" , "%");

    ecl_kw->fmt_linesize = 1;
    ecl_kw->blocksize    = ecl_char_blocksize;
    break;
  case (ecl_bool_type): /* Uncertain of this one ... */
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
	int read_elm = util_int_min((ib + 1) * ecl_kw->blocksize , ecl_kw->size) - ib * ecl_kw->blocksize;
	for (ir = 0; ir < read_elm; ir++) {
	  switch(ecl_kw->ecl_type) {
	  case(ecl_char_type):
	    ecl_kw_fscanf_qstring(&ecl_kw->data[offset] , ecl_kw->read_fmt , 8, stream);
	    break;
	  case(ecl_int_type):
	    {
	      int iread = fscanf(stream , ecl_kw->read_fmt , (int *) &ecl_kw->data[offset]);
	      if (iread != 1) {
		fprintf(stderr,"%s: after reading %d values reading of keyword:%s failed - aborting \n",__func__ , offset / ecl_kw->sizeof_ctype , ecl_kw->header);
		abort();
	      }
	    }
	    break;
	  case(ecl_float_type): 
	    {
	      int iread = fscanf(stream , ecl_kw->read_fmt , (float *) &ecl_kw->data[offset]);
	      if (iread != 1) {
		fprintf(stderr,"%s: after reading %d values reading of keyword:%s failed - aborting \n",__func__ , offset / ecl_kw->sizeof_ctype , ecl_kw->header);
		abort();
	      }
	    }
	    break;
	  case(ecl_double_type):
	    {
	      /*
		This rather painful parsing is because formatted
		eclipse double is 0.000D+01 - difficult to parse
		the 'D';
	      */
	      char   *end_ptr1 = NULL;
	      char   *end_ptr2 = NULL;
	      char   token[16];
	      double value;
	      int iread = fscanf(stream , "%s" , token);
	      if (iread == 1) {
		value = strtod(token , &end_ptr1);
		if (end_ptr1[0] == 'D') {
		  int power = strtod(end_ptr1 + 1 , &end_ptr2);
		  value *= exp(log(10.0) * power);
		  end_ptr1 = end_ptr2;
		}
		if (end_ptr1[0] != '\0') {
		  fprintf(stderr,"%s: 2: after reading %d values reading of keyword:%s failed - aborting \n",__func__ , offset / ecl_kw->sizeof_ctype , ecl_kw->header);
		  abort();
		}
	      } else {
		fprintf(stderr,"%s: after reading %d values reading of keyword:%s failed - aborting \n",__func__ , offset / ecl_kw->sizeof_ctype , ecl_kw->header);
		abort();
	      }

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
	  default:
	    fprintf(stderr,"Internal error in %s - internal eclipse_type: %d not recognized - aborting \n",__func__ , ecl_kw->ecl_type);
	    abort();
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
	  int read_elm = util_int_min((ib + 1) * ecl_kw->blocksize , ecl_kw->size) - ib * ecl_kw->blocksize;
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



bool ecl_kw_fseek_kw(const char * kw , bool fmt_file , bool rewind , bool abort_on_error , fortio_type *fortio) {
  ecl_kw_type *tmp_kw = ecl_kw_alloc_empty(fmt_file , fortio_get_endian_flip(fortio));     
  FILE *stream      = fortio_get_FILE(fortio);
  long int init_pos = ftell(stream);
  bool cont, kw_found;

  cont     = true;
  kw_found = false;
  while (cont) {
    bool header_OK = ecl_kw_fread_header(tmp_kw , fortio);
    if (header_OK) {
      if (ecl_kw_string_eq(ecl_kw_get_header_ref(tmp_kw) , kw)) {
	ecl_kw_rewind(tmp_kw, fortio);
	kw_found = true;
	cont = false;
      } else
	ecl_kw_fskip_data(tmp_kw , fortio);
    } else {
      if (rewind) {
	fortio_rewind(fortio);
	rewind = false;
      } else 
	cont = false;
    }
  }
  if (!kw_found) {
    if (abort_on_error) {
      fprintf(stderr,"%s: failed to locate keyword:%s in file:%s - aborting \n",__func__ , kw , fortio_filename_ref(fortio));
      abort();
    }
    fseek(stream , init_pos , SEEK_SET);
  }
  
  return kw_found;
}



bool ecl_kw_fseek_last_kw(const char * kw , bool fmt_file , bool abort_on_error , fortio_type *fortio) {
  FILE *stream      = fortio_get_FILE(fortio);
  long int init_pos = ftell(stream);
  bool kw_found     = false;

  fseek(stream , 0L , SEEK_SET);
  kw_found = ecl_kw_fseek_kw(kw , fmt_file , false , false , fortio);
  if (kw_found) {
    bool cont = true;
    do {
      long int current_pos = ftell(stream);
      ecl_kw_fskip(fortio , fmt_file);
      cont = ecl_kw_fseek_kw(kw , fmt_file , false , false , fortio);
      if (!cont) fseek(stream , current_pos , SEEK_SET);
    } while (cont);
  } else {
    if (abort_on_error) {
      fprintf(stderr,"%s: could not locate keyword:%s - aborting \n",__func__ , kw);
      abort();
    } else
      fseek(stream , init_pos , SEEK_SET);
  }
  return kw_found;
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
	util_endian_flip_vector(&size , sizeof size , 1);
    } else 
      OK = false;
  }
  if (OK) {
    ecl_kw_set_header(ecl_kw , header , size , ecl_type_str);
  } 
  return OK;
}


void ecl_kw_alloc_data(ecl_kw_type *ecl_kw) {
  if (ecl_kw->shared_data) {
    fprintf(stderr,"%s: trying to allocate data for ecl_kw object which has been declared with shared storage - aborting \n",__func__);
    abort();
  }
  {
    char *tmp;
    tmp = realloc(ecl_kw->data , ecl_kw->size * ecl_kw->sizeof_ctype);
    if (tmp == NULL) {
      if (ecl_kw->size * ecl_kw->sizeof_ctype != 0) {
	fprintf(stderr,"%s: Allocation of %d bytes failed - aborting \n",__func__ , ecl_kw->size * ecl_kw->sizeof_ctype);
	abort();
      }
    }
    if (ecl_kw->data != tmp) {
      ecl_kw->data  = tmp;
      ecl_kw->data_size = ecl_kw->size;
    }
  }
}




void ecl_kw_free_data(ecl_kw_type *ecl_kw) {
  free(ecl_kw->data);
  ecl_kw->data = NULL;
}


void ecl_kw_set_header_name(ecl_kw_type * ecl_kw , const char * header) {
  ecl_kw->header = realloc(ecl_kw->header , ecl_str_len + 1);
  sprintf(ecl_kw->header , "%-8s" , header);
}



void ecl_kw_set_header(ecl_kw_type *ecl_kw , const char *header ,  int size , const char *ecl_str_type ) {
  ecl_kw->ecl_type = __get_ecl_type(ecl_str_type);
  ecl_kw_init_types(ecl_kw , __get_ecl_type(ecl_str_type));
  if (strlen(header) > ecl_str_len) {
    fprintf(stderr," Fatal error in %s ecl_header_name:%s is longer than eight characters - aborting \n",__func__,header);
    abort();
  }
  ecl_kw_set_header_name(ecl_kw , header);
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


ecl_kw_type *ecl_kw_fread_alloc(fortio_type *fortio , bool fmt_file) {
  bool OK;
  ecl_kw_type *ecl_kw = ecl_kw_alloc_empty(fmt_file , fortio_get_endian_flip(fortio));
  OK = ecl_kw_fread_realloc(ecl_kw , fortio);
  
  if (!OK) {
    free(ecl_kw);
    ecl_kw = NULL;
  }

  if (ecl_kw == NULL) printf("%s: returning NULL \n",__func__);
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


void ecl_kw_fskip(fortio_type *fortio , bool fmt_file) {
  ecl_kw_type *tmp_kw;
  tmp_kw = ecl_kw_fread_alloc(fortio , fmt_file);
  ecl_kw_free(tmp_kw);
}




#define FPRINTF_BLOCK(ecl_kw , elements , tmp_var , stream)                                                        \
 {                                                                                                                 \
    int ib2;                                                                                                       \
    int small_blocks = (elements) / (ecl_kw)->fmt_linesize + (elements % (ecl_kw)->fmt_linesize == 0 ? 0 : 1);     \
    for (ib2 = 0; ib2 < small_blocks; ib2++) {                                                                     \
	 int elements2 = util_int_min((ib2 + 1)*(ecl_kw)->fmt_linesize , (elements)) - ib2 * (ecl_kw)->fmt_linesize;        \
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
	 int elements2 = util_int_min((ib2 + 1)*(ecl_kw)->fmt_linesize , (elements)) - ib2 * (ecl_kw)->fmt_linesize;        \
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
	 int elements2 = util_int_min((ib2 + 1)*(ecl_kw)->fmt_linesize , (elements)) - ib2 * (ecl_kw)->fmt_linesize;        \
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
	 int elements2 = util_int_min((ib2 + 1)*(ecl_kw)->fmt_linesize , (elements)) - ib2 * (ecl_kw)->fmt_linesize;        \
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



/*
  The function guarantees not net change to the data, 
  but there is temporarry change - that is the reason
  for the ugly (const casting).
*/
static void ecl_kw_fwrite_data(const ecl_kw_type *_ecl_kw, fortio_type *fortio) {
  ecl_kw_type *ecl_kw = (ecl_kw_type *) _ecl_kw;
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
    int elements = util_int_min((ib + 1)*ecl_kw->blocksize , ecl_kw->size) - ib*ecl_kw->blocksize;
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
      util_endian_flip_vector(&size , sizeof size , 1);

    fortio_init_write(fortio , ecl_str_len + sizeof(int) + ecl_type_len);
    fwrite(ecl_kw->header 			, sizeof(char)    , ecl_str_len  , stream);
    fwrite(&size    			        , sizeof(int)     , 1            , stream);
    fwrite(__get_ecl_str_type(ecl_kw->ecl_type) , sizeof(char)    , ecl_type_len , stream);
    fortio_complete_write(fortio);

  }
}


void ecl_kw_fwrite(const ecl_kw_type *ecl_kw , fortio_type *fortio) {
  ecl_kw_fwrite_header(ecl_kw , fortio);
  ecl_kw_fwrite_data(ecl_kw , fortio);
}


void * ecl_kw_get_data_ref(const ecl_kw_type *ecl_kw) {
  return ecl_kw->data;
}

void * ecl_kw_get_safe_data_ref(const ecl_kw_type *ecl_kw, ecl_type_enum ecl_type) {
  if (ecl_type != ecl_kw->ecl_type) {
    fprintf(stderr,"%s asked for type:%s  - internal ecl_kw_type is:%s - aborting \n",__func__ , __get_ecl_str_type(ecl_type) , __get_ecl_str_type(ecl_kw->ecl_type));
    abort();
  }
  return ecl_kw->data;
}



int ecl_kw_get_size(const ecl_kw_type * ecl_kw) {
  return ecl_kw->size;
}

const char * ecl_kw_get_str_type_ref(const ecl_kw_type *ecl_kw) {
  return __get_ecl_str_type(ecl_kw->ecl_type);
}

ecl_type_enum ecl_kw_get_type(const ecl_kw_type * ecl_kw) { return ecl_kw->ecl_type; }

bool ecl_kw_get_endian_convert(const ecl_kw_type * ecl_kw) { return ecl_kw->endian_convert; }


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


void ecl_kw_fwrite_param_fortio(fortio_type * fortio, bool fmt_file , bool endian_convert , const char * header ,  ecl_type_enum ecl_type , int size, void * data) {
  ecl_kw_type   * ecl_kw = ecl_kw_alloc_complete_shared(fmt_file , endian_convert , header , size , ecl_type , data);
  ecl_kw_fwrite(ecl_kw , fortio);
  ecl_kw_free(ecl_kw);
}
    


void ecl_kw_fwrite_param(const char * filename , bool fmt_file , bool endian_convert , const char * header ,  ecl_type_enum ecl_type , int size, void * data) {
  fortio_type   * fortio = fortio_open(filename , "w" , endian_convert);
  ecl_kw_fwrite_param_fortio(fortio , fmt_file , endian_convert , header , ecl_type , size , data);
  fortio_close(fortio);
}



void ecl_kw_get_data_as_double(const ecl_kw_type * ecl_kw , double * double_data) {

  if (ecl_kw->ecl_type == ecl_double_type)
    ecl_kw_get_memcpy_data(ecl_kw , double_data);
  else {
    if (ecl_kw->ecl_type == ecl_float_type) {
      const float * float_data = (const float *) ecl_kw->data;
      util_float_to_double(double_data , float_data  , ecl_kw->size);
    }
    else {
      fprintf(stderr,"%s: type can not be converted to double - aborting \n",__func__);
      ecl_kw_summarize(ecl_kw);
      abort();
    }
  }
}


void ecl_kw_fread_double_param(const char * filename , bool fmt_file , bool endian_convert, double * double_data) {
  fortio_type   * fortio      = fortio_open(filename , "r" , endian_convert);
  ecl_kw_type   * ecl_kw      = ecl_kw_fread_alloc(fortio , fmt_file);
  fortio_close(fortio);
  
  if (ecl_kw == NULL) {
    fprintf(stderr,"%s: fatal error: loading parameter from: %s failed - aborting \n",__func__ , filename);
    abort();
  }
  ecl_kw_get_data_as_double(ecl_kw , double_data);
  ecl_kw_free(ecl_kw);
}
    

void ecl_kw_summarize(const ecl_kw_type * ecl_kw) {
  printf("%8s   %10d:%4s \n",ecl_kw_get_header_ref(ecl_kw),
	 ecl_kw_get_size(ecl_kw),
	 ecl_kw_get_str_type_ref(ecl_kw));
}

void ecl_kw_fprintf_grdecl(ecl_kw_type * ecl_kw , FILE * stream) {
  fortio_type * fortio = fortio_alloc_FILE_wrapper(NULL , false , stream);   /* Endian flip should *NOT* be used */
  bool org_fmt = ecl_kw_get_fmt_file(ecl_kw);
  ecl_kw_set_fmt_file(ecl_kw , true);
  fprintf(stream,"%s\n" , ecl_kw_get_header_ref(ecl_kw));
  ecl_kw_fwrite_data(ecl_kw , fortio);
  fprintf(stream,"\n/"); /* Unsure about the leading newline ?? */
  fortio_free_FILE_wrapper(fortio);
  ecl_kw_set_fmt_file(ecl_kw , org_fmt);
}



ecl_kw_type * ecl_kw_fscanf_alloc_grdecl_data(FILE * stream , int size , ecl_type_enum ecl_type , bool endian_flip) {
  char buffer[9];
  
  ecl_kw_type * ecl_kw = ecl_kw_alloc_empty(true , endian_flip);
  ecl_kw_init_types(ecl_kw , ecl_type);
  ecl_kw->size     = size;
  ecl_kw_alloc_data(ecl_kw);

  fscanf(stream , "%s" , buffer);
  ecl_kw_set_header_name(ecl_kw , buffer);
  {
    bool at_eof;
    fortio_type * fortio = fortio_alloc_FILE_wrapper(NULL , endian_flip , stream);
    ecl_kw_fread_data(ecl_kw , fortio);
    util_fskip_chars(stream , " \n\r" , &at_eof);
    fscanf(stream , "%s" , buffer);

    if (buffer[0] != '/') {
      fprintf(stderr,"%s: Did not find '/' at end of %s \n",__func__ , ecl_kw->header);
      abort();
    }
    fortio_free_FILE_wrapper(fortio);
  }

  return ecl_kw;
}


ecl_kw_type * ecl_kw_fscanf_alloc_parameter(FILE * stream , int size , bool endian_flip) {
  return ecl_kw_fscanf_alloc_grdecl_data(stream , size , ecl_float_type , endian_flip);
}



/*****************************************************************/


void ecl_kw_scalar_init(ecl_kw_type * ecl_kw , double init_value) {
  void * data = ecl_kw_get_data_ref(ecl_kw);
  int    size = ecl_kw_get_size(ecl_kw);
  int i;

  switch (ecl_kw->ecl_type) {
  case(ecl_double_type):
    {
      double *double_data = (double *) data;
      for (i=0; i < size; i++)
	double_data[i] = init_value;
      break;
    }
  case(ecl_float_type):
    {
      float *float_data = (float *) data;
      for (i=0; i < size; i++)
	float_data[i] = init_value;
      break;
    }
  default:
    fprintf(stderr,"%s: can only be called on ecl_float_type and ecl_double_type - aborting \n",__func__);
    abort();
  }
}


void ecl_kw_shift(ecl_kw_type * ecl_kw , double shift_value) {
  void * data = ecl_kw_get_data_ref(ecl_kw);
  int    size = ecl_kw_get_size(ecl_kw);
  int i;


  switch (ecl_kw->ecl_type) {
  case(ecl_double_type):
    {
      double *double_data = (double *) data;
      for (i=0; i < size; i++)
	double_data[i] += shift_value;
      break;
    }
  case(ecl_float_type):
    {
      float *float_data = (float *) data;
      for (i=0; i < size; i++)
	float_data[i] += shift_value;
      break;
    }
  default:
    fprintf(stderr,"%s: can only be called on ecl_float_type and ecl_double_type - aborting \n",__func__);
    abort();
  }
}


void ecl_kw_scale(ecl_kw_type * ecl_kw , double scale_factor) {
  void * data = ecl_kw_get_data_ref(ecl_kw);
  int    size = ecl_kw_get_size(ecl_kw);
  int i;

  switch (ecl_kw->ecl_type) {
  case(ecl_double_type):
    {
      double *double_data = (double *) data;
      for (i=0; i < size; i++)
	double_data[i] *= scale_factor;
      break;
    }
  case(ecl_float_type):
    {
      float *float_data = (float *) data;
      for (i=0; i < size; i++)
	float_data[i] *= scale_factor;
      break;
    }
  default:
    fprintf(stderr,"%s: can only be called on ecl_float_type and ecl_double_type - aborting \n",__func__);
    abort();
  }
}



void ecl_kw_inplace_sub(ecl_kw_type * my_kw , const ecl_kw_type * sub_kw) {

  int            size = ecl_kw_get_size(my_kw);
  ecl_type_enum type = ecl_kw_get_type(my_kw);
  if ((size != ecl_kw_get_size(sub_kw)) || (type != ecl_kw_get_type(sub_kw))) {
    fprintf(stderr,"%s: attempt to subtract to fields of different size - aborting \n",__func__);
    abort();
  }
  {
    int i;
    void * my_data        = ecl_kw_get_data_ref(my_kw);
    const void * sub_data = ecl_kw_get_data_ref(sub_kw);

    switch (type) {
    case(ecl_double_type):
      {
	double *my_double        = (double *) my_data;
	const double *sub_double = (const double *) sub_data;
	for (i=0; i < size; i++)
	  my_double[i] -= sub_double[i];
	break;
      }
    case(ecl_float_type):
      {
	float *my_float        = (float *)       my_data;
	const float *sub_float = (const float *) sub_data;
	for (i=0; i < size; i++)
	  my_float[i] -= sub_float[i];
	break;
      }
    default:
      fprintf(stderr,"%s: can only be called on ecl_float_type and ecl_double_type - aborting \n",__func__);
      abort();
    }

  }
}


void ecl_kw_inplace_mul(ecl_kw_type * my_kw , const ecl_kw_type * mul_kw) {

  int            size = ecl_kw_get_size(my_kw);
  ecl_type_enum type = ecl_kw_get_type(my_kw);
  if ((size != ecl_kw_get_size(mul_kw)) || (type != ecl_kw_get_type(mul_kw))) {
    fprintf(stderr,"%s: attempt to multract to fields of different size - aborting \n",__func__);
    abort();
  }
  {
    int i;
    void * my_data        = ecl_kw_get_data_ref(my_kw);
    const void * mul_data = ecl_kw_get_data_ref(mul_kw);

    switch (type) {
    case(ecl_double_type):
      {
	double *my_double        = (double *) my_data;
	const double *mul_double = (const double *) mul_data;
	for (i=0; i < size; i++)
	  my_double[i] *= mul_double[i];
	break;
      }
    case(ecl_float_type):
      {
	float *my_float        = (float *)       my_data;
	const float *mul_float = (const float *) mul_data;
	for (i=0; i < size; i++)
	  my_float[i] *= mul_float[i];
	break;
      }
    default:
      fprintf(stderr,"%s: can only be called on ecl_float_type and ecl_double_type - aborting \n",__func__);
      abort();
    }

  }
}


void ecl_kw_inplace_div(ecl_kw_type * my_kw , const ecl_kw_type * div_kw) {

  int            size = ecl_kw_get_size(my_kw);
  ecl_type_enum type = ecl_kw_get_type(my_kw);
  if ((size != ecl_kw_get_size(div_kw)) || (type != ecl_kw_get_type(div_kw))) {
    fprintf(stderr,"%s: attempt to divtract to fields of different size - aborting \n",__func__);
    abort();
  }
  {
    int i;
    void * my_data        = ecl_kw_get_data_ref(my_kw);
    const void * div_data = ecl_kw_get_data_ref(div_kw);

    switch (type) {
    case(ecl_double_type):
      {
	double *my_double        = (double *) my_data;
	const double *div_double = (const double *) div_data;
	for (i=0; i < size; i++)
	  my_double[i] /= div_double[i];
	break;
      }
    case(ecl_float_type):
      {
	float *my_float        = (float *)       my_data;
	const float *div_float = (const float *) div_data;
	for (i=0; i < size; i++)
	  my_float[i] /= div_float[i];
	break;
      }
    default:
      fprintf(stderr,"%s: can only be called on ecl_float_type and ecl_double_type - aborting \n",__func__);
      abort();
    }

  }
}


void ecl_kw_inplace_inv(ecl_kw_type * my_kw) {
  int            size = ecl_kw_get_size(my_kw);
  ecl_type_enum type = ecl_kw_get_type(my_kw);
  {
    int i;
    void * my_data        = ecl_kw_get_data_ref(my_kw);

    switch (type) {
    case(ecl_double_type):
      {
	double *my_double        = (double *) my_data;
	for (i=0; i < size; i++)
	  my_double[i] = 1.0/ my_double[i];
	break;
      }
    case(ecl_float_type):
      {
	float *my_float        = (float *)       my_data;
	for (i=0; i < size; i++)
	  my_float[i] = 1.0 / my_float[i];
	break;
      }
    default:
      fprintf(stderr,"%s: can only be called on ecl_float_type and ecl_double_type - aborting \n",__func__);
      abort();
    }
  }
}



static void ecl_kw_inplace_add__(ecl_kw_type * my_kw , int my_offset , const ecl_kw_type * add_kw , bool different_size_ok) {
  ecl_type_enum type  = ecl_kw_get_type(my_kw);
  int my_size         = ecl_kw_get_size(my_kw);
  int add_size        = ecl_kw_get_size(add_kw);
  int my_last_index   = my_offset + add_size;

  if (different_size_ok) {
    if (my_last_index >= my_size) {
      fprintf(stderr,"%s: the last index of the adder will extend beyond the size - aborting \n",__func__);
      abort();
    }
  } else {
    if (my_size != add_size || my_offset != 0) {
      fprintf(stderr,"%s: attempt to add to fields of different size - aborting \n",__func__);
      abort();
    }
  }

  if (type != ecl_kw_get_type(add_kw)) {
    fprintf(stderr,"%s: trying to add fields of different type - aborting \n",__func__);
    abort();
  }

  {
    int i;
    void * my_data        = ecl_kw_get_data_ref(my_kw);
    const void * add_data = ecl_kw_get_data_ref(add_kw);

    switch (type) {
    case(ecl_double_type):
      {
	double *my_double        = (double *) my_data;
	const double *add_double = (const double *) add_data;
	for (i=0; i < add_size; i++)
	  my_double[i + my_offset] += add_double[i];
	break;
      }

    case(ecl_float_type):
      {
	float *my_float        = (float *)       my_data;
	const float *add_float = (const float *) add_data;
	for (i=0; i < add_size; i++)
	  my_float[i + my_offset] += add_float[i];
	break;
      }
      
    default:
      fprintf(stderr,"%s: can only be called on ecl_float_type and ecl_double_type - aborting \n",__func__);
      abort();
    }

  }
}


void ecl_kw_inplace_add(ecl_kw_type * my_kw , const ecl_kw_type * add_kw) {
  ecl_kw_inplace_add__(my_kw , 0 , add_kw , false);
}


void ecl_kw_inplace_add_subkw(ecl_kw_type * my_kw , int my_offset , const ecl_kw_type * add_kw) {
  ecl_kw_inplace_add__(my_kw , my_offset , add_kw , true);
}



void ecl_kw_merge(ecl_kw_type * main_kw , const ecl_kw_type * sub_kw , const ecl_box_type * ecl_box) {
  if (main_kw->sizeof_ctype != sub_kw->sizeof_ctype) {
    fprintf(stderr,"%s: trying to combine two different underlying datatypes - aborting \n",__func__);
    abort();
  }
  if (ecl_kw_get_size(main_kw) != ecl_box_get_total_size(ecl_box)) {
    fprintf(stderr,"%s box size and total_kw mismatch - aborting \n",__func__);
    abort();
  }
  if (ecl_kw_get_size(sub_kw)   != ecl_box_get_box_size(ecl_box)) {
    fprintf(stderr,"%s box size and total_kw mismatch - aborting \n",__func__);
    abort();
  }

  ecl_box_set_values(ecl_box , ecl_kw_get_data_ref(main_kw) , ecl_kw_get_data_ref(sub_kw) , main_kw->sizeof_ctype);
}



/******************************************************************/

bool ecl_kw_is_kw_file(FILE * stream , bool fmt_file , bool endian_flip) {
  const long int init_pos = ftell(stream);
  bool kw_file;
  
  {
    ecl_kw_type * ecl_kw = ecl_kw_alloc_empty(fmt_file , endian_flip);
    fortio_type * fortio = fortio_alloc_FILE_wrapper(NULL , endian_flip , stream);

    if (fmt_file) 
      kw_file = ecl_kw_fread_header(ecl_kw , fortio);
    else {
      if (fortio_is_fortio_file(fortio)) 
	kw_file = ecl_kw_fread_header(ecl_kw , fortio);
      else
	kw_file = false;
    } 

    fortio_free_FILE_wrapper(fortio);
  }
  
  fseek(stream , init_pos , SEEK_SET);
  return kw_file;
}





bool ecl_kw_is_grdecl_file(FILE * stream) {
  const long int init_pos = ftell(stream);
  bool grdecl_file;
  bool at_eof = false;
  /*printf("%s: starting at_eof:%d \n",__func__,at_eof);*/
  util_fskip_chars(stream ,  " \r\n\t"  , &at_eof); /* Skipping intial space */
  /*printf("%s: Have skipped initial characters pos:%d at_eof:%d \n",__func__ , ftell(stream) , at_eof);*/
  util_fskip_cchars(stream , " \r\n\t" , &at_eof); /* Skipping PORO/PERMX/... */
  /*printf("%s: Have skipped keyword name \n",__func__);
  printf("%s current position:%d at_eof:%d \n",__func__,ftell(stream) , at_eof);
  */
  if (at_eof) 
    grdecl_file = false;
  else {
    grdecl_file = true;
    {
      int c;
      do {
	/*printf("%s: inner loop \n",__func__);*/
	c = fgetc(stream);
	if (c == '\r' || c == '\n') 
	  break;
	else {
	  if (c != ' ') {
	    grdecl_file = false;
	    break;
	  }
	}
      } while (c == ' ');
    }
  }
  fseek(stream , init_pos , SEEK_SET);
  return grdecl_file;
}

  


#define KW_MAX_MIN(type)                       		 \
{                                              		 \
  type * data = ecl_kw_get_data_ref(ecl_kw);   		 \
  type max = -data[0];                         		 \
  type min =  data[0];                         		 \
  int i;                                       		 \
  for (i=1; i < ecl_kw_get_size(ecl_kw); i++)  		 \
      util_update_ ## type ## _max_min(data[i] , &max , &min); \
  memcpy(_max , &max , ecl_kw->sizeof_ctype);            \
  memcpy(_min , &min , ecl_kw->sizeof_ctype);            \
}



void ecl_kw_max_min(const ecl_kw_type * ecl_kw , void * _max , void *_min) {
  switch (ecl_kw->ecl_type) {
  case(ecl_float_type):
    KW_MAX_MIN(float);
    break;
  case(ecl_double_type):
    KW_MAX_MIN(double);
    break;
  case(ecl_int_type):
    KW_MAX_MIN(int);
    break;
  default:
    fprintf(stderr,"%s: invalid type for element sum \n",__func__);
    abort();
  }
}

#undef KW_MAX_MIN




#define KW_SUM(type)                           \
{                                              \
  type * data = ecl_kw_get_data_ref(ecl_kw);   \
  type sum = 0;                                \
  int i;                                       \
  for (i=0; i < ecl_kw_get_size(ecl_kw); i++)  \
     sum += data[i];                           \
  memcpy(_sum , &sum , ecl_kw->sizeof_ctype);  \
}



void ecl_kw_element_sum(const ecl_kw_type * ecl_kw , void * _sum) {
  switch (ecl_kw->ecl_type) {
  case(ecl_float_type):
    KW_SUM(float);
    break;
  case(ecl_double_type):
    KW_SUM(double);
    break;
  case(ecl_int_type):
    KW_SUM(int);
    break;
  default:
    fprintf(stderr,"%s: invalid type for element sum \n",__func__);
    abort();
  }
}

#undef KW_SUM
