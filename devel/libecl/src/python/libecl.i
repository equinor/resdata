
%module libecl
%include "typemaps.i"

%{
#include <ecl_util.h>
#include <ecl_sum.h>
#include <ecl_file.h>
#include <ecl_kw.h>
#include <ecl_grid.h>
#include <fortio.h>
#include <stdio.h>
%}

%typemap(argout) ecl_file_enum *OutValue {
	PyObject *o;
	o = PyInt_FromLong(*$1);
	if ((!$result) || ($result == Py_None)) 
		$result = o;
	else 
		printf("FUBAR?\n");
}


%typemap(in,numinputs=0) ecl_file_enum *OutValue(ecl_file_enum temp) {
	// Set the input argument to point to a temporary variable 
	$1 = &temp;
}

%typemap(in) float * values {
  if (PyList_Check($input)) {
    int size = PyList_Size($input);
    int i = 0;
    $1 = malloc((size+1)*sizeof(float));
    for (i = 0; i < size; i++) {
      PyObject *o = PyList_GetItem($input,i);
      if (PyFloat_Check(o))  {
		$1[i] = PyFloat_AsDouble(o);
      } else {
		printf("error: there was a non-float element!\n");
		free($1);
		return NULL;
      }
    }
  }
}

%typemap(in) double * values {
  if (PyList_Check($input)) {
    int size = PyList_Size($input);
    int i = 0;
    $1 = malloc((size+1)*sizeof(double));
    for (i = 0; i < size; i++) {
      PyObject *o = PyList_GetItem($input,i);
      if (PyFloat_Check(o))  {
		$1[i] = PyFloat_AsDouble(o);
      } else {
		printf("error: there was a non-float element!\n");
		free($1);
		return NULL;
      }
    }
  }
}

%typemap(freearg) double *values {
	free($1);
}


// This tells SWIG to treat char ** as a special case
%typemap(in) char ** {
  if (PyList_Check($input)) {
    int size = PyList_Size($input);
    int i = 0;
    $1 = malloc((size+1)*sizeof(char *));
    for (i = 0; i < size; i++) {
      PyObject *o = PyList_GetItem($input,i);
      if (PyString_Check(o))
		$1[i] = PyString_AsString(PyList_GetItem($input,i));
      else {
		PyErr_SetString(PyExc_TypeError,"list must contain strings");
		free($1);
		return NULL;
      }
    }
    $1[i] = 0;
  } else {
    PyErr_SetString(PyExc_TypeError,"not a list");
    return NULL;
  }
}

// This cleans up the char ** array we malloced before the function call
%typemap(freearg) char ** {
	free($1);
}

%typemap(in) FILE * {
	$1 = PyFile_AsFile($input); 
}

%typemap(out) time_t {
	$result = PyLong_FromLong($1);
}

%newobject ecl_sum_fread_alloc_case;
%typemap(newfree) ecl_sum_type * {
//	printf("Finished ecl_sum_fread_alloc_case\n");
}

%typemap(freearg) ecl_sum_type * {
//	printf("SWIG: ecl_sum_type * got freearg\n");
}


%typemap(out) ecl_kw_data_wrapper * {
	return SWIG_NewPointerObj($1, SWIGTYPE_p_ecl_kw_data_wrapper, 0);
}

%typemap(out) ecl_kw_data_wrapper_float * {
	return SWIG_NewPointerObj($1, SWIGTYPE_p_ecl_kw_data_wrapper_float, 0);
}

%typemap(out) ecl_kw_data_wrapper_void * {
	return SWIG_NewPointerObj($1, SWIGTYPE_p_ecl_kw_data_wrapper_void, 0);
}

%inline %{

typedef struct _ecl_kw_data_wrapper_void {
	void *v;
	int size;
	const ecl_kw_type *e;
} ecl_kw_data_wrapper_void;

PyObject *get_ecl_kw_data_wrapper_void_list(ecl_kw_data_wrapper_void *w) {
	int i;
	PyObject *list;
	PyObject *o;
	const char *str_type;

	str_type = ecl_kw_get_str_type_ref(w->e);
	list = PyList_New(0);
	for (i = 0; i < w->size; i++) {
		if (!strcmp(str_type, "REAL")) {
			o = PyFloat_FromDouble(((float *)w->v)[i]);
		} else if(!strcmp(str_type,"INTE")) {
			o = PyInt_FromLong(((int *)w->v)[i]);
		} else if (!strcmp(str_type, "DOUB")) {
			o = PyFloat_FromDouble(((double *)w->v)[i]);
		}
		else 
			printf("Typemap error: Unknown TYPE: %s!\n", str_type);
		PyList_Append(list, o);
	}

	free(w);

	return list;
}

ecl_kw_data_wrapper_void *ecl_kw_get_data_wrap_void(const ecl_kw_type * ecl_kw) {
	ecl_kw_data_wrapper_void *w;

	w = malloc(sizeof(ecl_kw_data_wrapper_void));
	w->v = ecl_kw_get_void_ptr(ecl_kw);
	w->size = ecl_kw_get_size(ecl_kw);
	w->e = ecl_kw;

	return w;
}

float ecl_kw_iget_ptr_wrap(const ecl_kw_type * ecl_kw, int index) {
	void *d;
	d = ecl_kw_iget_ptr(ecl_kw, index);
	return ((float *) d)[index];
}

%}


// ecl_util.h
void ecl_util_get_file_type(char *, ecl_file_enum *OutValue, bool *OUTPUT, int *OUTPUT);
bool ecl_util_fmt_file(const char *);

%include ../ecl_sum.h

%include ../ecl_file.h

%include ../fortio.h

// ecl_kw.h
const char  * ecl_kw_get_header_ref(const ecl_kw_type *);
const char  * ecl_kw_get_str_type_ref(const ecl_kw_type *);
void 		  ecl_kw_get_data_as_double(const ecl_kw_type * ecl_kw , double * double_data);
int           ecl_kw_get_size(const ecl_kw_type *);
void 		* ecl_kw_get_void_ptr(const ecl_kw_type * ecl_kw);
void          ecl_kw_free(ecl_kw_type *);
ecl_kw_type * ecl_kw_alloc_empty();

ecl_kw_type * ecl_kw_fread_alloc(fortio_type *);
bool          ecl_kw_fseek_kw(const char * , bool , bool , fortio_type *);
bool          ecl_kw_fread_realloc(ecl_kw_type *, fortio_type *);
void        * ecl_kw_iget_ptr(const ecl_kw_type *, int);
void          ecl_kw_fwrite(const ecl_kw_type *,  fortio_type *);
void          ecl_kw_set_header(ecl_kw_type  * , const char * , int , const char *);
void ecl_kw_set_memcpy_data(ecl_kw_type *ecl_kw , const void *src);
void 		  ecl_kw_set_header_alloc(ecl_kw_type *ecl_kw , const char *header ,  int size , const char *ecl_str_type ); 
ecl_kw_type * ecl_kw_alloc_complete(const char * header ,  int size, ecl_type_enum ecl_type , const void * data);
void          ecl_kw_fprintf_grdecl(ecl_kw_type *  , FILE * );
void          ecl_kw_alloc_double_data(ecl_kw_type * ecl_kw , double * values);
void          ecl_kw_alloc_float_data(ecl_kw_type * ecl_kw , float * values);
double   ecl_kw_iget_as_double(const ecl_kw_type *  , int );
void          ecl_kw_free_data(ecl_kw_type *);


// ecl_grid.h
ecl_grid_type * ecl_grid_alloc(const char * , bool);
const char    * ecl_grid_get_name( const ecl_grid_type * );
void            ecl_grid_free(ecl_grid_type * );
void 			ecl_grid_summarize(const ecl_grid_type * ecl_grid);
void            ecl_grid_get_dims(const ecl_grid_type * , int *OUTPUT, int *OUTPUT , int *OUTPUT , int *OUTPUT);
void 			ecl_grid_get_ijk1(const ecl_grid_type * grid , int global_index, int *OUTPUT, int *OUTPUT , int *OUTPUT);
void            ecl_grid_get_ijk1A(const ecl_grid_type * , int active_index, int *OUTPUT, int *OUTPUT , int *OUTPUT);
double 			ecl_grid_get_property(const ecl_grid_type * ecl_grid , const ecl_kw_type * ecl_kw , int i , int j , int k); 
int             ecl_grid_get_active_size( const ecl_grid_type * ecl_grid );
int             ecl_grid_get_global_size( const ecl_grid_type * ecl_grid );
int ecl_grid_get_active_index1(const ecl_grid_type * ecl_grid , int global_index);
int      ecl_grid_get_global_index3(const ecl_grid_type * , int  , int , int );


// ANSI-C
void free(void *ptr);

