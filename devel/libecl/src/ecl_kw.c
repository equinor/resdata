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
#include <ecl_box.h>


#define DEBUG 1

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))



struct ecl_kw_struct {
  bool  fmt_file;
  bool  endian_convert;
  
  int   size;
  int 	sizeof_ctype;
  int   fmt_linesize;
  int   blocksize;
  ecl_type_enum ecl_type;
  char  *header;
  char  *read_fmt, *write_fmt;
  char  *data;
  bool  shared_data;       /* Whether this keyword has shared data or not. */ 
  long  int _start_pos;    /* The in-file position of the start of this keyword. */
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
  if (index < 0 || index >= ecl_kw->size) 
    util_abort("%s: Invalid index lookup. \n",caller);
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
    util_abort("%s: Fatal error: eclipse_type:%s not %d characters long - aborting \n",__func__,ecl_type_str, ecl_type_len);
  }

  {
    int i, ecl_type;
    ecl_type = -1;
    for (i = 0; i < ecl_Ntype; i++) {
      if (strncmp(ecl_type_str , ecl_type_map[i] , ecl_type_len) == 0)
	 ecl_type = i;
    }
    if (ecl_type == -1) 
      util_abort("%s: Fatal error: eclipse_type :%s not recognized - aborting \n",__func__ , ecl_type_str);
    
    return ecl_type;
  }
}



static void ecl_kw_endian_convert_data(ecl_kw_type *ecl_kw) {
  if (ecl_kw->ecl_type != ecl_char_type && ecl_kw->ecl_type != ecl_mess_type) 
    util_endian_flip_vector(ecl_kw->data , ecl_kw->sizeof_ctype , ecl_kw->size);
}


const char * ecl_kw_get_header_ref(const ecl_kw_type *ecl_kw) { return ecl_kw->header; }


char * ecl_kw_alloc_strip_header(const ecl_kw_type *ecl_kw) {
  return util_alloc_strip_copy(ecl_kw->header);
}

void ecl_kw_get_memcpy_data(const ecl_kw_type *ecl_kw , void *target) {
  memcpy(target , ecl_kw->data , ecl_kw->size * ecl_kw->sizeof_ctype);
}

/** Allocates a untyped buffer with exactly the same content as the ecl_kw instances data. */
void * ecl_kw_alloc_data_copy(const ecl_kw_type * ecl_kw) {
  void * buffer = util_alloc_copy( ecl_kw->data , ecl_kw->size * ecl_kw->sizeof_ctype , __func__);
  return buffer;
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
  if (len1 > ecl_str_len) 
    util_abort("%s : eclipse keyword:%s is too long - aborting \n",__func__ , long_kw);
  
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


/**
   This function compares two ecl_kw instances, and returns true if they are equal.
*/

bool ecl_kw_equal(const ecl_kw_type *ecl_kw1, const ecl_kw_type *ecl_kw2) {
  bool  equal = true;

  if (strcmp(ecl_kw1->header , ecl_kw2->header) != 0)            
    equal  = false;
  else if (ecl_kw1->size != ecl_kw2->size) 
    equal = false;
  else if (ecl_kw1->ecl_type != ecl_kw2->ecl_type) 
    equal = false;
  else if (memcmp(ecl_kw1->data , ecl_kw2->data , ecl_kw1->size * ecl_kw1->sizeof_ctype) != 0) /** OK the headers are identical - time to compare the data content. */
    equal = false;

  return equal;
}



void ecl_kw_set_shared_ref(ecl_kw_type * ecl_kw , void *data_ptr) {
  if (!ecl_kw->shared_data) 
    util_abort("%s: trying to set shared data reference for ecl_kw object which has been allocated with private storage - aborting \n",__func__);
  
  ecl_kw->data = data_ptr;
}


static void ecl_kw_set_shared(ecl_kw_type * ecl_kw) {
  if (!ecl_kw->shared_data) {
    if (ecl_kw->data != NULL) 
      util_abort("%s: can not change to shared for keyword with allocated storage - aborting \n",__func__);
  }
  ecl_kw->shared_data = true;
}




ecl_kw_type * ecl_kw_alloc_complete(const char * header ,  int size, ecl_type_enum ecl_type , const void * data) {
  ecl_kw_type *ecl_kw;
  ecl_kw = ecl_kw_alloc_empty();
  ecl_kw_set_header(ecl_kw , header , size , __get_ecl_str_type(ecl_type));
  ecl_kw_alloc_data(ecl_kw);
  ecl_kw_set_memcpy_data(ecl_kw , data);
  return ecl_kw;
}


ecl_kw_type * ecl_kw_alloc_scalar(const char * header , int size , ecl_type_enum ecl_type , double init_value) {
  ecl_kw_type *ecl_kw;
  ecl_kw = ecl_kw_alloc_empty();
  ecl_kw_set_header(ecl_kw , header , size , __get_ecl_str_type(ecl_type));
  ecl_kw_alloc_data(ecl_kw);
  ecl_kw_scalar_init(ecl_kw , init_value);
  return ecl_kw;
}



ecl_kw_type * ecl_kw_alloc_complete_shared(const char * header ,  int size, ecl_type_enum ecl_type , void * data) {
  ecl_kw_type *ecl_kw;
  ecl_kw = ecl_kw_alloc_empty();
  ecl_kw_set_header(ecl_kw , header , size , __get_ecl_str_type(ecl_type));
  ecl_kw_set_shared(ecl_kw);
  ecl_kw_set_shared_ref(ecl_kw , data);
  return ecl_kw;
}



ecl_kw_type * ecl_kw_alloc_empty() {
  ecl_kw_type *ecl_kw;

  ecl_kw = util_malloc(sizeof *ecl_kw , __func__);
  ecl_kw->header       = NULL;
  ecl_kw->read_fmt     = NULL;
  ecl_kw->write_fmt    = NULL;
  ecl_kw->data 	       = NULL;
  ecl_kw->shared_data  = false;
  ecl_kw->size         = 0;
  ecl_kw->sizeof_ctype = 0;
  
  return ecl_kw;
}



void ecl_kw_free(ecl_kw_type *ecl_kw) {
  if (ecl_kw->read_fmt != NULL) free(ecl_kw->read_fmt);
  free(ecl_kw->write_fmt);
  free(ecl_kw->header);
  ecl_kw_free_data(ecl_kw);
  free(ecl_kw);
}

void ecl_kw_free__(void *void_ecl_kw) {
  ecl_kw_free((ecl_kw_type *) void_ecl_kw);
}


void ecl_kw_memcpy(ecl_kw_type *target, const ecl_kw_type *src) {
  target->size         	      = src->size;
  target->sizeof_ctype 	      = src->sizeof_ctype;
  target->fmt_linesize 	      = src->fmt_linesize;
  target->blocksize           = src->blocksize;
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


/**
   Will return a double value for underlying data types of double and float.
*/
double ecl_kw_iget_as_double(const ecl_kw_type * ecl_kw , int i) {
  if (ecl_kw->ecl_type == ecl_float_type) 
    return ecl_kw_iget_float( ecl_kw , i); /* Here the compiler will silently insert a float -> double conversion. */
  else if (ecl_kw->ecl_type == ecl_double_type)
    return ecl_kw_iget_double( ecl_kw, i);
  else {
    util_abort("%s: can not be converted to double - no data for you! \n",__func__);
    return -1;
  }
}


#define ECL_KW_IGET_TYPED(type)                                						    \
type ecl_kw_iget_ ## type(const ecl_kw_type * ecl_kw, int i) { 						    \
  type value;                                                  						    \
  if (ecl_kw_get_type(ecl_kw) != ecl_ ## type ## _type)             					    \
    util_abort("%s: Keyword: %s is wrong type - aborting \n",__func__ , ecl_kw_get_header_ref(ecl_kw));     \
  ecl_kw_iget_static(ecl_kw , i , &value);                                                                  \
  return value;                                                                                             \
}                                                                                                           \

ECL_KW_IGET_TYPED(double);
ECL_KW_IGET_TYPED(float);
ECL_KW_IGET_TYPED(int);
#undef ECL_KW_IGET_TYPED


#define ECL_KW_ISET_TYPED(type)                                						    \
void ecl_kw_iset_ ## type(ecl_kw_type * ecl_kw, int i, type value) {    			            \
  if (ecl_kw_get_type(ecl_kw) != ecl_ ## type ## _type)             					    \
    util_abort("%s: Keyword: %s is wrong type - aborting \n",__func__ , ecl_kw_get_header_ref(ecl_kw));     \
  ecl_kw_iset_static(ecl_kw , i , &value);                                                                  \
}                                                                                                           \

ECL_KW_ISET_TYPED(double);
ECL_KW_ISET_TYPED(float);
ECL_KW_ISET_TYPED(int);
#undef ECL_KW_ISET_TYPED


/*****************************************************************/
/* Various ways to get pointers to the underlying data. */

#define ECL_KW_GET_TYPED_PTR(type)                                					    \
type * ecl_kw_get_ ## type ## _ptr(const ecl_kw_type * ecl_kw) {       		                            \
  if (ecl_kw_get_type(ecl_kw) != ecl_ ## type ## _type)             					    \
    util_abort("%s: Keyword: %s is wrong type - aborting \n",__func__ , ecl_kw_get_header_ref(ecl_kw));     \
  return (type *) ecl_kw->data;                                                                             \
}                                                                                                           

ECL_KW_GET_TYPED_PTR(double);
ECL_KW_GET_TYPED_PTR(float);
ECL_KW_GET_TYPED_PTR(int);
#undef ECL_KW_GET_TYPED_PTR

void * ecl_kw_get_void_ptr(const ecl_kw_type * ecl_kw) {
  return ecl_kw->data;
}

/*****************************************************************/


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
    util_abort("%s: Internal error: internal eclipse_type: %d not recognized - aborting \n",__func__ , ecl_kw->ecl_type);
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

static void ecl_kw_fread_data(ecl_kw_type *ecl_kw, fortio_type *fortio) {
  const char null_char         = '\0';
  const char char_true         = 84;
  const char char_false        = 70;
  const int  fortran_int_true  = -1;
  const int  fortran_int_false = 0;
  bool fmt_file                = fortio_fmt_file( fortio );
  if (ecl_kw->size > 0) {
    if (fmt_file) {
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
	      if (iread != 1) 
		util_abort("%s: after reading %d values reading of keyword:%s from:%s failed - aborting \n",__func__ , offset / ecl_kw->sizeof_ctype , ecl_kw->header , fortio_filename_ref(fortio));
	    }
	    break;
	  case(ecl_float_type): 
	    {
	      int iread = fscanf(stream , ecl_kw->read_fmt , (float *) &ecl_kw->data[offset]);
	      if (iread != 1) 
		util_abort("%s: after reading %d values reading of keyword:%s from:%s failed - aborting \n",__func__ , offset / ecl_kw->sizeof_ctype , ecl_kw->header , fortio_filename_ref(fortio));
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
	      char   token[32];
	      double value;
	      int iread = fscanf(stream , "%s" , token);
	      if (iread == 1) {
		value = strtod(token , &end_ptr1);
		if (end_ptr1[0] == 'D') {
		  int power = strtod(end_ptr1 + 1 , &end_ptr2);
		  value *= exp(log(10.0) * power);
		  end_ptr1 = end_ptr2;
		}
		if (end_ptr1[0] != '\0') 
		  util_abort("%s: 2: after reading %d values reading of keyword:%s failed - aborting (FILE: %s)\n",__func__ , offset / ecl_kw->sizeof_ctype , ecl_kw->header , fortio_filename_ref(fortio));
	      } else 
		util_abort("%s: after reading %d values reading of keyword:%s failed - aborting (FILE: %s) \n",__func__ , offset / ecl_kw->sizeof_ctype , ecl_kw->header , fortio_filename_ref(fortio));

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
	      else 
		util_abort("%s: Logical value: [%c] not recogniced - aborting \n", __func__ , bool_char);
	    }
	    break;
	  case(ecl_mess_type):
	    ecl_kw_fscanf_qstring(&ecl_kw->data[offset] , ecl_kw->read_fmt , 8 , stream);
	    break;
	  default:
	    util_abort("%s: Internal error: internal eclipse_type: %d not recognized - aborting \n",__func__ , ecl_kw->ecl_type);
	  }
	  offset += ecl_kw->sizeof_ctype;
	  index++;
	}
      }
    } else {
      if (ecl_kw->ecl_type == ecl_char_type || ecl_kw->ecl_type == ecl_mess_type) {
	const int blocks = ecl_kw->size / ecl_kw->blocksize + (ecl_kw->size % ecl_kw->blocksize == 0 ? 0 : 1);
	int ib;
	for (ib = 0; ib < blocks; ib++) {
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
	} 
      } else
	/**
	   This function handles the fuc***g blocks transparently at a
	   low level.
	*/
	fortio_fread_buffer(fortio , ecl_kw->data , ecl_kw->size * ecl_kw->sizeof_ctype);
      
      if (fortio_endian_flip(fortio))
	ecl_kw_endian_convert_data(ecl_kw);
    }
  }
}


void ecl_kw_rewind(const ecl_kw_type *ecl_kw , fortio_type *fortio) {
  fseek(fortio_get_FILE(fortio) , ecl_kw->_start_pos , SEEK_SET);
}


void ecl_kw_fskip_data(ecl_kw_type *ecl_kw, fortio_type *fortio) {
  bool fmt_file = fortio_fmt_file(fortio);
  if (ecl_kw->size > 0) {
    if (fmt_file) {
      /* Formatted skipping actually involves reading the data - nice ??? */
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
	fortio_fskip_record(fortio);
    }
  }
} 


bool ecl_kw_fread_header(ecl_kw_type *ecl_kw , fortio_type *fortio) {
  const char null_char = '\0';
  FILE *stream  = fortio_get_FILE(fortio);
  bool fmt_file = fortio_fmt_file( fortio );
  char header[ecl_str_len + 1];
  char ecl_type_str[ecl_type_len + 1];
  int record_size;
  int size;
  bool OK;

  ecl_kw->_start_pos = ftell(stream);
  if (fmt_file) {
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
      if (fortio_endian_flip(fortio)) 
	util_endian_flip_vector(&size , sizeof size , 1);
    } else 
      OK = false;
  }
  if (OK) 
    ecl_kw_set_header(ecl_kw , header , size , ecl_type_str);

  return OK;
}


/**
   Will seek through the open fortio file and search for a keyword with
   header 'kw'.It will always start the search from the present
   position in the file, but if rewind is true it will rewind the
   fortio file if not finding 'kw' between current offset and EOF.

   If the kw is found the fortio pointer is positioned at the
   beginning of the keyword, and the function returns true. If the the
   'kw' is NOT found the file will be repositioned to the initial
   position, and the function will return false; unlessa bort_on_error
   == true in which case the function will abort if the 'kw' s not found.
*/
   

bool ecl_kw_fseek_kw(const char * kw , bool rewind , bool abort_on_error , fortio_type *fortio) {
  ecl_kw_type *tmp_kw = ecl_kw_alloc_empty();
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
    if (abort_on_error) 
      util_abort("%s: failed to locate keyword:%s in file:%s - aborting \n",__func__ , kw , fortio_filename_ref(fortio));
    
    fseek(stream , init_pos , SEEK_SET);
  }
  
  ecl_kw_free(tmp_kw);
  return kw_found;
}


bool ecl_kw_ifseek_kw(const char * kw , fortio_type * fortio , int index) {
  int i = 0;
  do {
    ecl_kw_fseek_kw(kw , false , true , fortio);
    i++;
  } while (i <= index);
  return true;
}


bool ecl_kw_fseek_last_kw(const char * kw , bool abort_on_error , fortio_type *fortio) {
  FILE *stream      = fortio_get_FILE(fortio);
  long int init_pos = ftell(stream);
  bool kw_found     = false;

  fseek(stream , 0L , SEEK_SET);
  kw_found = ecl_kw_fseek_kw(kw ,  false , false , fortio);
  if (kw_found) {
    bool cont = true;
    do {
      long int current_pos = ftell(stream);
      ecl_kw_fskip(fortio);
      cont = ecl_kw_fseek_kw(kw , false , false , fortio);
      if (!cont) fseek(stream , current_pos , SEEK_SET);
    } while (cont);
  } else {
    if (abort_on_error) 
      util_abort("%s: could not locate keyword:%s - aborting \n",__func__ , kw);
    else
      fseek(stream , init_pos , SEEK_SET);
  }
  return kw_found;
}


/** 
  This function will search through a GRDECL file to look for the
  'kw'; input variables and return vales are similar to
  ecl_kw_fseek_kw(). The filename argument is ONLY used for printing a
  sensible error message when/if aborting.

  Observe that the GRDECL files are exteremly weakly structured, it is
  therefor veeeery easy to fool this function with a malformed GRDECL
  file. The current implementation just does string-search for 'kw'.
*/

bool ecl_kw_grdecl_fseek_kw(const char * kw , bool rewind , bool abort_on_error , FILE * stream, const char * filename) {
  char *file_kw     = util_alloc_string_copy( kw );
  long int init_pos = ftell(stream);
  bool cont, kw_found;
  
  cont     = true;
  kw_found = false;
  while (cont) {
    int c;
    bool at_EOF   = false;
    do {
      c = getc( stream );
    } while (c != kw[0] && c != EOF);
    if (c == EOF) at_EOF = true;

    if ( !at_EOF ) {
      /*
	Now we have found a character which is equal to the first character in kw - this might be it! 
      */
      if (fread(&file_kw[1] , 1 , strlen(kw) - 1 , stream) == (strlen(kw) - 1)) {
	/* OK - we have read in the remaining number of characters - let us compare! */
	if (strcmp(file_kw , kw) == 0) {
	  kw_found = true;
	  cont = false;
	} else
	  fseek(stream , -(strlen(kw) - 1) , SEEK_CUR);
      } else at_EOF = true;
    }
    
    if (!kw_found) {
      if (at_EOF) {
	if (rewind) {
	  fseek(stream , 0L , SEEK_SET); /* Go to beginning of file */
	  rewind = false;                /* No more rewinds ... */
	} else
	  cont = false;                  /* OK - give up with kw_found == false. */
      }
    }
  }
	  
  if (!kw_found) {
    if (abort_on_error) 
      util_abort("%s: failed to locate keyword:%s in file:%s - aborting \n",__func__ , kw , filename);
    
    fseek(stream , init_pos , SEEK_SET);     /* Repositioning to the initial position. */
  } else
    fseek(stream , -strlen(kw) , SEEK_CUR);  /* Reposition to the beginning of kw */
  
  free(file_kw);
  return kw_found;
}



void ecl_kw_alloc_data(ecl_kw_type *ecl_kw) {
  if (ecl_kw->shared_data) 
    util_abort("%s: trying to allocate data for ecl_kw object which has been declared with shared storage - aborting \n",__func__);
  
  ecl_kw->data = util_realloc(ecl_kw->data , ecl_kw->size * ecl_kw->sizeof_ctype , __func__);
}



void ecl_kw_free_data(ecl_kw_type *ecl_kw) {
  if (!ecl_kw->shared_data) 
    util_safe_free(ecl_kw->data);
  
  ecl_kw->data = NULL;
}



void ecl_kw_set_header_name(ecl_kw_type * ecl_kw , const char * header) {
  ecl_kw->header = realloc(ecl_kw->header , ecl_str_len + 1);
  sprintf(ecl_kw->header , "%-8s" , header);
}



void ecl_kw_set_header(ecl_kw_type *ecl_kw , const char *header ,  int size , const char *ecl_str_type ) {
  ecl_kw->ecl_type = __get_ecl_type(ecl_str_type);
  ecl_kw_init_types(ecl_kw , __get_ecl_type(ecl_str_type));
  if (strlen(header) > ecl_str_len) 
    util_abort("%s: Fatal error: ecl_header_name:%s is longer than eight characters - aborting \n",__func__,header);
  
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


void ecl_kw_fread(ecl_kw_type * ecl_kw , fortio_type * fortio) {
  int current_size = ecl_kw->size;
  if (!ecl_kw_fread_header(ecl_kw , fortio)) 
    util_abort("%s: failed to read header for ecl_kw - aborting \n",__func__);

  if (ecl_kw->size == current_size) 
    ecl_kw_fread_data(ecl_kw , fortio);
  else 
    util_abort("%s: size mismatch - aborting \n",__func__);
}


ecl_kw_type *ecl_kw_fread_alloc(fortio_type *fortio) {
  bool OK;
  ecl_kw_type *ecl_kw = ecl_kw_alloc_empty();
  OK = ecl_kw_fread_realloc(ecl_kw , fortio);
  
  if (!OK) {
    free(ecl_kw);
    ecl_kw = NULL;
  }
  
  return ecl_kw;
}






void ecl_kw_fskip(fortio_type *fortio) {
  ecl_kw_type *tmp_kw;
  tmp_kw = ecl_kw_fread_alloc(fortio );
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
static void ecl_kw_fwrite_data(const ecl_kw_type *_ecl_kw , fortio_type *fortio) {
  ecl_kw_type *ecl_kw = (ecl_kw_type *) _ecl_kw;
  const int blocks    = ecl_kw->size / ecl_kw->blocksize + (ecl_kw->size % ecl_kw->blocksize == 0 ? 0 : 1);
  FILE *stream        = fortio_get_FILE(fortio);
  bool  fmt_file      = fortio_fmt_file( fortio );
  int ib;
  bool local_endian_flip = false;


  if (!fmt_file) { 
    if (fortio_endian_flip(fortio)) {
      ecl_kw_endian_convert_data(ecl_kw);
      local_endian_flip = true;
    }
  }
  
  for (ib = 0; ib < blocks; ib++) {
    int elements = util_int_min((ib + 1)*ecl_kw->blocksize , ecl_kw->size) - ib*ecl_kw->blocksize;
    if (fmt_file) {
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
	util_abort("%s: Internal error:  internal eclipse_type: %d not recognized - aborting \n",__func__ , ecl_kw->ecl_type);
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
	fortio_fwrite_record(fortio , &ecl_kw->data[ib * ecl_kw->blocksize * ecl_kw->sizeof_ctype] , sizeof_ctype * elements);
    }
  }

  /* Convert back - the in-memory representation should always be "correct". */
  if (local_endian_flip) 
    ecl_kw_endian_convert_data(ecl_kw);
  
}




void ecl_kw_fwrite_header(const ecl_kw_type *ecl_kw , fortio_type *fortio) {
  FILE *stream  = fortio_get_FILE(fortio);
  bool fmt_file = fortio_fmt_file(fortio);
  if (fmt_file) 
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
  ecl_kw_fwrite_header(ecl_kw ,  fortio);
  ecl_kw_fwrite_data(ecl_kw   ,  fortio);
}




static void * ecl_kw_get_data_ref(const ecl_kw_type *ecl_kw) {
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

/** 
    The functions cfwrite_header() and cfread_header() and write/read
    *FAR TO MUCH* header information. But now they are out there ...,
    with lot's of file images in place. I guess there is something to
    be learned from this?
    
    Got to stick to this, at least for a while.
*/
    


void ecl_kw_cfwrite_header(const ecl_kw_type * ecl_kw , FILE *stream) {
  bool dummy_bool = false;

  fwrite(&dummy_bool             , sizeof dummy_bool           , 1 , stream);  /* Old ecl_kw->fmt_file */
  fwrite(&ecl_kw->sizeof_ctype 	 , sizeof ecl_kw->sizeof_ctype , 1 , stream);
  fwrite(&ecl_kw->size         	 , sizeof ecl_kw->size         , 1 , stream);
  fwrite(&ecl_kw->fmt_linesize 	 , sizeof ecl_kw->fmt_linesize , 1 , stream);
  fwrite(&ecl_kw->blocksize    	 , sizeof ecl_kw->blocksize    , 1 , stream);
  fwrite(&dummy_bool             , sizeof dummy_bool           , 1 , stream);  /* Old ecl_kw->endian_flip */
  fwrite(&ecl_kw->ecl_type       , sizeof ecl_kw->ecl_type     , 1 , stream);

  util_fwrite_string(ecl_kw->header    , stream);
  util_fwrite_string(ecl_kw->write_fmt , stream);
  util_fwrite_string(ecl_kw->read_fmt  , stream);
}


void ecl_kw_cfwrite(const ecl_kw_type * ecl_kw , FILE *stream) {
  ecl_kw_cfwrite_header(ecl_kw , stream);
  {
    int items_written = fwrite(ecl_kw->data , ecl_kw->sizeof_ctype , ecl_kw->size , stream);
    if (items_written != ecl_kw->size) 
      util_abort("%s: failed to write all data to disk - aborting. \n",__func__);
  }
}


void ecl_kw_cfread_header(ecl_kw_type * ecl_kw , FILE * stream) {
  bool dummy_bool;
  
  fread(&dummy_bool , sizeof dummy_bool , 1, stream);  /* Old ecl_kw->fmt_file */
  fread(&ecl_kw->sizeof_ctype 	 , sizeof ecl_kw->sizeof_ctype , 1 , stream);
  fread(&ecl_kw->size    	 , sizeof ecl_kw->size         , 1 , stream);
  fread(&ecl_kw->fmt_linesize 	 , sizeof ecl_kw->fmt_linesize , 1 , stream);
  fread(&ecl_kw->blocksize    	 , sizeof ecl_kw->blocksize , 1 , stream);
  fread(&dummy_bool , sizeof dummy_bool , 1, stream);  /* Old ecl_kw->fmt_file */
  fread(&ecl_kw->ecl_type        , sizeof ecl_kw->ecl_type   , 1 , stream);

  ecl_kw->header    = util_fread_realloc_string(ecl_kw->header    , stream);
  ecl_kw->write_fmt = util_fread_realloc_string(ecl_kw->write_fmt , stream);
  ecl_kw->read_fmt  = util_fread_realloc_string(ecl_kw->read_fmt  , stream);
}



void ecl_kw_cfread(ecl_kw_type * ecl_kw , FILE *stream) {
  ecl_kw_cfread_header(ecl_kw , stream);
  ecl_kw_alloc_data(ecl_kw);
  {
    int items_written = fread(ecl_kw->data , sizeof *ecl_kw->data , ecl_kw->size , stream);
    if (items_written != ecl_kw->size) 
      util_abort("%s: failed to write all data to disk - aborting. \n",__func__);
  }
}


void ecl_kw_fwrite_compressed(const ecl_kw_type *ecl_kw , FILE * stream) {
  ecl_kw_cfwrite_header(ecl_kw , stream);
  /*
    Dumps out nine characters - including a \0 - for every char variable 
  */
  util_fwrite_compressed(ecl_kw->data , ecl_kw->size * ecl_kw->sizeof_ctype , stream);
}



void ecl_kw_fread_realloc_compressed(ecl_kw_type * ecl_kw , FILE * stream) {
  ecl_kw_cfread_header(ecl_kw , stream); 
  ecl_kw_alloc_data(ecl_kw);
  util_fread_compressed(ecl_kw->data , stream);
}


ecl_kw_type * ecl_kw_fread_alloc_compressed(FILE * stream) {
  ecl_kw_type *ecl_kw = ecl_kw_alloc_empty(false , false);
  ecl_kw_fread_realloc_compressed(ecl_kw , stream);
  return ecl_kw;
}



void ecl_kw_fwrite_param_fortio(fortio_type * fortio, const char * header ,  ecl_type_enum ecl_type , int size, void * data) {
  ecl_kw_type   * ecl_kw = ecl_kw_alloc_complete_shared(header , size , ecl_type , data);
  ecl_kw_fwrite(ecl_kw , fortio);
  ecl_kw_free(ecl_kw);
}
    


void ecl_kw_fwrite_param(const char * filename , bool fmt_file , bool endian_convert , const char * header ,  ecl_type_enum ecl_type , int size, void * data) {
  fortio_type   * fortio = fortio_fopen(filename , "w" , endian_convert , fmt_file);
  ecl_kw_fwrite_param_fortio(fortio , header , ecl_type , size , data);
  fortio_fclose(fortio);
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
      util_abort("%s: ABorting \n",__func__);
    }
  }
}


void ecl_kw_fread_double_param(const char * filename , bool fmt_file , bool endian_convert, double * double_data) {
  fortio_type   * fortio      = fortio_fopen(filename , "r" , endian_convert , fmt_file);
  ecl_kw_type   * ecl_kw      = ecl_kw_fread_alloc(fortio);
  fortio_fclose(fortio);
  
  if (ecl_kw == NULL) 
    util_abort("%s: fatal error: loading parameter from: %s failed - aborting \n",__func__ , filename);

  ecl_kw_get_data_as_double(ecl_kw , double_data);
  ecl_kw_free(ecl_kw);
}
    

void ecl_kw_summarize(const ecl_kw_type * ecl_kw) {
  printf("%8s   %10d:%4s \n",ecl_kw_get_header_ref(ecl_kw),
	 ecl_kw_get_size(ecl_kw),
	 ecl_kw_get_str_type_ref(ecl_kw));
}


void ecl_kw_fprintf_grdecl(ecl_kw_type * ecl_kw , FILE * stream) {
  fortio_type * fortio = fortio_alloc_FILE_wrapper(NULL , false , true , stream);   /* Endian flip should *NOT* be used */
  fprintf(stream,"%s\n" , ecl_kw_get_header_ref(ecl_kw));
  ecl_kw_fwrite_data(ecl_kw , fortio);
  fprintf(stream,"\n/\n"); /* Unsure about the leading newline ?? */
  fortio_free_FILE_wrapper(fortio);
}


/**
   These files are tricky to load - if there is something wrong
   it is nearly impossible to detect.
*/
ecl_kw_type * ecl_kw_fscanf_alloc_grdecl_data(FILE * stream , int size , ecl_type_enum ecl_type) {
  char buffer[9];
  
  ecl_kw_type * ecl_kw = ecl_kw_alloc_empty();
  ecl_kw_init_types(ecl_kw , ecl_type);
  ecl_kw->size     = size;
  ecl_kw_alloc_data(ecl_kw);

  fscanf(stream , "%s" , buffer);
  ecl_kw_set_header_name(ecl_kw , buffer);
  {
    fortio_type * fortio = fortio_alloc_FILE_wrapper(NULL ,true , true , stream);  /* The endian flip is not used. */
    ecl_kw_fread_data(ecl_kw , fortio);
    fscanf(stream , "%s" , buffer);
    
    if (buffer[0] != '/') {
      fprintf(stderr,"\n");
      fprintf(stderr,"Have read:%d items \n",size);
      fprintf(stderr,"File is malformed for some reason ...\n");
      fprintf(stderr,"Looking at: %s \n",buffer);
      fprintf(stderr,"Current buffer position: %ld \n", ftell(stream));
      util_abort("%s: Did not find '/' at end of %s - size mismatch / malformed file ??\n",__func__ , ecl_kw->header);
    }
    fortio_free_FILE_wrapper(fortio);
  }

  return ecl_kw;
}


ecl_kw_type * ecl_kw_fscanf_alloc_parameter(FILE * stream , int size ) {
  return ecl_kw_fscanf_alloc_grdecl_data(stream , size , ecl_float_type);
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
    util_abort("%s: can only be called on ecl_float_type and ecl_double_type - aborting \n",__func__);
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
    util_abort("%s: can only be called on ecl_float_type and ecl_double_type - aborting \n",__func__);
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
    util_abort("%s: can only be called on ecl_float_type and ecl_double_type - aborting \n",__func__);
  }
}



void ecl_kw_inplace_sub(ecl_kw_type * my_kw , const ecl_kw_type * sub_kw) {

  int            size = ecl_kw_get_size(my_kw);
  ecl_type_enum type = ecl_kw_get_type(my_kw);
  if ((size != ecl_kw_get_size(sub_kw)) || (type != ecl_kw_get_type(sub_kw))) 
    util_abort("%s: attempt to subtract to fields of different size - aborting \n",__func__);
  
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
      util_abort("%s: can only be called on ecl_float_type and ecl_double_type - aborting \n",__func__);
    }

  }
}


void ecl_kw_inplace_mul(ecl_kw_type * my_kw , const ecl_kw_type * mul_kw) {

  int            size = ecl_kw_get_size(my_kw);
  ecl_type_enum type = ecl_kw_get_type(my_kw);
  if ((size != ecl_kw_get_size(mul_kw)) || (type != ecl_kw_get_type(mul_kw))) 
    util_abort("%s: attempt to multract to fields of different size - aborting \n",__func__);
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
      util_abort("%s: can only be called on ecl_float_type and ecl_double_type - aborting \n",__func__);
    }

  }
}


void ecl_kw_inplace_div(ecl_kw_type * my_kw , const ecl_kw_type * div_kw) {

  int            size = ecl_kw_get_size(my_kw);
  ecl_type_enum type = ecl_kw_get_type(my_kw);
  if ((size != ecl_kw_get_size(div_kw)) || (type != ecl_kw_get_type(div_kw))) 
    util_abort("%s: attempt to divtract to fields of different size - aborting \n",__func__);
    
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
      util_abort("%s: can only be called on ecl_float_type and ecl_double_type - aborting \n",__func__);
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
      util_abort("%s: can only be called on ecl_float_type and ecl_double_type - aborting \n",__func__);
    }
  }
}



static void ecl_kw_inplace_add__(ecl_kw_type * my_kw , int my_offset , const ecl_kw_type * add_kw , bool different_size_ok) {
  ecl_type_enum type  = ecl_kw_get_type(my_kw);
  int my_size         = ecl_kw_get_size(my_kw);
  int add_size        = ecl_kw_get_size(add_kw);
  int my_last_index   = my_offset + add_size;

  if (different_size_ok) {
    if (my_last_index >= my_size) 
      util_abort("%s: the last index of the adder will extend beyond the size - aborting \n",__func__);
  } else {
    if (my_size != add_size || my_offset != 0) 
      util_abort("%s: attempt to add to fields of different size - aborting \n",__func__);
  }

  if (type != ecl_kw_get_type(add_kw)) 
    util_abort("%s: trying to add fields of different type - aborting \n",__func__);

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
      util_abort("%s: can only be called on ecl_float_type and ecl_double_type - aborting \n",__func__);
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
  if (main_kw->sizeof_ctype != sub_kw->sizeof_ctype) 
    util_abort("%s: trying to combine two different underlying datatypes - aborting \n",__func__);

  if (ecl_kw_get_size(main_kw) != ecl_box_get_total_size(ecl_box)) 
    util_abort("%s box size and total_kw mismatch - aborting \n",__func__);

  if (ecl_kw_get_size(sub_kw)   != ecl_box_get_box_size(ecl_box)) 
    util_abort("%s box size and total_kw mismatch - aborting \n",__func__);

  ecl_box_set_values(ecl_box , ecl_kw_get_data_ref(main_kw) , ecl_kw_get_data_ref(sub_kw) , main_kw->sizeof_ctype);
}


void ecl_kw_inplace_update_file(const ecl_kw_type * ecl_kw , const char * filename, int index) {
  if (util_file_exists(filename)) {
    bool endian_flip = true;
    bool fmt_file = util_fmt_bit8(filename);

    if (!fmt_file)
      if (!fortio_guess_endian_flip(filename , &endian_flip)) 
	util_abort("%s: could not determine endian ness of: %s \n",__func__ , filename);

    {
      fortio_type * fortio =  fortio_fopen(filename , "r+" , endian_flip , fmt_file);
      ecl_kw_ifseek_kw(ecl_kw_get_header_ref(ecl_kw) , fortio , index);
      {
	ecl_kw_type *file_kw = ecl_kw_alloc_empty();
	ecl_kw_fread_header(file_kw , fortio);
	ecl_kw_rewind(file_kw , fortio);
	
	if (!((file_kw->size == ecl_kw->size) && (file_kw->ecl_type == ecl_kw->ecl_type)))
	  util_abort("%s: header mismatch when trying to update:%s in %s \n",__func__ , ecl_kw_get_header_ref(ecl_kw) , filename);
	ecl_kw_free(file_kw);
      }

  
      fortio_fflush(fortio);
      ecl_kw_fwrite(ecl_kw , fortio);
      fortio_fclose(fortio);
    }
  }
}


/******************************************************************/

bool ecl_kw_is_kw_file(FILE * stream , bool fmt_file , bool endian_flip) {
  const long int init_pos = ftell(stream);
  bool kw_file;
  
  {
    ecl_kw_type * ecl_kw = ecl_kw_alloc_empty();
    fortio_type * fortio = fortio_alloc_FILE_wrapper(NULL , endian_flip , fmt_file , stream);
    
    if (fmt_file) 
      kw_file = ecl_kw_fread_header(ecl_kw , fortio);
    else {
      if (fortio_is_fortio_file(fortio)) 
	kw_file = ecl_kw_fread_header(ecl_kw , fortio);
      else
	kw_file = false;
    } 

    fortio_free_FILE_wrapper(fortio);
    ecl_kw_free(ecl_kw);
  }
  
  fseek(stream , init_pos , SEEK_SET);
  return kw_file;
}





bool ecl_kw_is_grdecl_file(FILE * stream) {
  const long int init_pos = ftell(stream);
  bool grdecl_file;
  bool at_eof = false;
  util_fskip_chars(stream ,  " \r\n\t"  , &at_eof);  /* Skipping intial space */
  util_fskip_cchars(stream , " \r\n\t"  , &at_eof);  /* Skipping PORO/PERMX/... */
  if (at_eof) 
    grdecl_file = false;
  else {
    grdecl_file = true;
    {
      int c;
      do {
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
    util_abort("%s: invalid type for element sum \n",__func__);
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
    util_abort("%s: invalid type for element sum \n",__func__);
  }
}
#undef KW_SUM



