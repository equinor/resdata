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
#include <rms_export.h>
%}


/*****************************************************************************
 *  Handle an "argument out" value of an enum type.
 */

%typemap(argout) ecl_file_enum *OutValue {
	PyObject *o;
	o = PyInt_FromLong(*$1);
	if ((!$result) || ($result == Py_None)) 
		$result = o;
	else 
		printf("FUBAR?\n");
}

%typemap(in,numinputs=0) ecl_file_enum *OutValue(ecl_file_enum temp) {
	$1 = &temp;
}


/*****************************************************************************
 * This sections handles the events where a functions takes a list as an
 * in argument. In that case the python list has to be converted to a C array.
 */

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

%typemap(freearg) float *values {
	free($1);
}


%typemap(in) ecl_kw_type ** {
  if (PyList_Check($input)) {
    int size = PyList_Size($input);
    int i = 0;
    $1 = malloc((size + 1) * (sizeof *$1));
    for (i = 0; i < size; i++) {
      PyObject *o = PyList_GetItem($input,i);
      SWIG_ConvertPtr(o, (void **) &$1[i], SWIGTYPE_p_ecl_kw_type, SWIG_POINTER_EXCEPTION);
    }
  } else {
    PyErr_SetString(PyExc_TypeError,"not a list");
    return NULL;
  }
}

%typemap(freearg) ecl_kw_type ** {
	free($1);
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

%typemap(freearg) char ** {
	free($1);
}

/*****************************************************************************
 *  Here comes a few trivial mappings.
 */

%typemap(in) FILE * {
	$1 = PyFile_AsFile($input); 
}

%typemap(out) time_t {
	$result = PyLong_FromLong($1);
}

/*****************************************************************************
 * The following wrappers and maps us beeing used in the ecl_kw().iget_data().
 */

%typemap(out) ecl_kw_data_wrapper_void * {
  /* Return pointer to python */
	return SWIG_NewPointerObj($1, SWIGTYPE_p_ecl_kw_data_wrapper_void, 0);
}

%inline %{

typedef struct _ecl_kw_data_wrapper_void {
	void *v;
	int size;
	const ecl_kw_type *e;
} ecl_kw_data_wrapper_void;


PyObject *get_ecl_kw_data_wrapper_void_list(ecl_kw_data_wrapper_void *w) {
  /* Now we have all the needed information for creating a python list */
	int i;
	PyObject *list;
	PyObject *o;
  ecl_type_enum kwtype;

  kwtype = ecl_kw_get_type(w->e);
	list = PyList_New(0);
	for (i = 0; i < w->size; i++) {
    switch (kwtype) {
      /* Is this grabbing of data safe? :) */
      case (ecl_float_type):
        o = PyFloat_FromDouble(((float *)w->v)[i]);
        break;
      case (ecl_double_type):
        o = PyFloat_FromDouble(((double *)w->v)[i]);
        break;
      case (ecl_int_type):
        o = PyInt_FromLong(((int *)w->v)[i]);
        break;
      default:
        printf("Error: type not implemented in wrapper code!\n");
        exit(-1);
		}

		PyList_Append(list, o);
	}

	free(w);

	return list;
}

ecl_kw_data_wrapper_void *ecl_kw_get_data_wrap_void(const ecl_kw_type * ecl_kw) {
  /* This is the function beeing called from ecl.pyy */
	ecl_kw_data_wrapper_void *w;

	w = malloc(sizeof(ecl_kw_data_wrapper_void));
	w->v = ecl_kw_get_void_ptr(ecl_kw);
	w->size = ecl_kw_get_size(ecl_kw);
	w->e = ecl_kw;

	return w;
}


ecl_kw_type *ecl_kw_fscanf_alloc_grdecl_data_wrap(FILE *stream, int size) {
  return ecl_kw_fscanf_alloc_grdecl_data(stream, size, ecl_float_type);
}

/*************************************************************************/

%}

%include ../ecl_sum.h
%include ../ecl_file.h
%include ../fortio.h

/* ecl_util.h */
void ecl_util_get_file_type(char *, ecl_file_enum *OutValue, bool *OUTPUT, int *OUTPUT);
bool ecl_util_fmt_file(const char *);

/* ecl_kw.h */
const char  * ecl_kw_get_header_ref(const ecl_kw_type *);
const char  * ecl_kw_get_str_type_ref(const ecl_kw_type *);
void          ecl_kw_get_data_as_double(const ecl_kw_type * ecl_kw , double * double_data);
int           ecl_kw_get_size(const ecl_kw_type *);
void 		    * ecl_kw_get_void_ptr(const ecl_kw_type * ecl_kw);
void          ecl_kw_free(ecl_kw_type *);
ecl_kw_type * ecl_kw_alloc_empty();
ecl_kw_type * ecl_kw_fread_alloc(fortio_type *);
bool          ecl_kw_fseek_kw(const char * , bool , bool , fortio_type *);
bool          ecl_kw_fread_realloc(ecl_kw_type *, fortio_type *);
void        * ecl_kw_iget_ptr(const ecl_kw_type *, int);
void          ecl_kw_fwrite(const ecl_kw_type *,  fortio_type *);
void          ecl_kw_set_header(ecl_kw_type  * , const char * , int , const char *);
void          ecl_kw_set_memcpy_data(ecl_kw_type *ecl_kw , const void *src);
void 		      ecl_kw_set_header_alloc(ecl_kw_type *ecl_kw , const char *header , 
                                      int size , const char *ecl_str_type ); 
ecl_kw_type * ecl_kw_alloc_complete(const char * header ,  int size, 
                                    ecl_type_enum ecl_type , const void * data);
void          ecl_kw_fprintf_grdecl(ecl_kw_type *  , FILE * );
void          ecl_kw_alloc_float_data(ecl_kw_type * ecl_kw , float * values);
void          ecl_kw_alloc_double_data(ecl_kw_type * ecl_kw , double * values);
double        ecl_kw_iget_as_double(const ecl_kw_type *  , int );
void          ecl_kw_free_data(ecl_kw_type *);
ecl_kw_type * ecl_kw_fscanf_alloc_grdecl_data(FILE * stream , int size , ecl_type_enum ecl_type);

/* ecl_grid.h */
ecl_grid_type * ecl_grid_alloc(const char * , bool);
const char    * ecl_grid_get_name( const ecl_grid_type * );
void            ecl_grid_free(ecl_grid_type * );
void 			      ecl_grid_summarize(const ecl_grid_type * ecl_grid);
void            ecl_grid_get_dims(const ecl_grid_type * , int *OUTPUT, int *OUTPUT , 
                                  int *OUTPUT , int *OUTPUT);
void 			      ecl_grid_get_ijk1(const ecl_grid_type * grid , int global_index, 
                                  int *OUTPUT, int *OUTPUT , int *OUTPUT);
void            ecl_grid_get_ijk1A(const ecl_grid_type * , int active_index, 
                                   int *OUTPUT, int *OUTPUT , int *OUTPUT);
double 			    ecl_grid_get_property(const ecl_grid_type * ecl_grid , const ecl_kw_type * ecl_kw , 
                                  int i , int j , int k); 
int             ecl_grid_get_active_size( const ecl_grid_type * ecl_grid );
int             ecl_grid_get_global_size( const ecl_grid_type * ecl_grid );
int             ecl_grid_get_active_index1(const ecl_grid_type * ecl_grid , int global_index);
int             ecl_grid_get_active_index3(const ecl_grid_type * ecl_grid , int i , int j , int k);
int             ecl_grid_get_global_index3(const ecl_grid_type * , int  , int , int );

/* rms_export.h */
void            rms_export_roff_from_keyword(const char *filename, ecl_grid_type *ecl_grid, 
                                             ecl_kw_type **ecl_kw, int size);

/* ANSI-C */
void free(void *ptr);

