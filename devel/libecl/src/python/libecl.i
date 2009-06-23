
%module libecl
%include "typemaps.i"

%{
#include <ecl_util.h>
#include <ecl_sum.h>
#include <ecl_file.h>
#include <ecl_kw.h>
#include <ecl_grid.h>
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

// This tells SWIG to treat char ** as a special case
%typemap(in) char ** {
  /* Check if is a list */
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
		}
		else 
			printf("Unknown TYPE!\n");
		PyList_Append(list, o);
	}

	free(w);

	return list;
}

ecl_kw_data_wrapper_void *ecl_kw_get_data_wrap_void(const ecl_kw_type * ecl_kw) {
	ecl_kw_data_wrapper_void *w;

	w = malloc(sizeof(ecl_kw_data_wrapper_void));
	//printf("sizeof %d\n", sizeof(ecl_kw_data_wrapper_void));
	//printf("sizeof * %d\n", sizeof(ecl_kw_data_wrapper_void *));
	w->v = ecl_kw_get_void_ptr(ecl_kw);
	w->size = ecl_kw_get_size(ecl_kw);
	w->e = ecl_kw;

	return w;
}

%}


// ecl_util.h
void ecl_util_get_file_type(char *, ecl_file_enum *OutValue, bool *OUTPUT, int *OUTPUT);

%include ../ecl_sum.h

%include ../ecl_file.h

// ecl_kw.h
const char  * ecl_kw_get_header_ref(const ecl_kw_type *);
const char  * ecl_kw_get_str_type_ref(const ecl_kw_type *);
void ecl_kw_get_data_as_double(const ecl_kw_type * ecl_kw , double * double_data);
int           ecl_kw_get_size(const ecl_kw_type *);
const float *ecl_kw_get_data(const ecl_kw_type * ecl_kw);
void * ecl_kw_get_void_ptr(const ecl_kw_type * ecl_kw);
void          ecl_kw_free(ecl_kw_type *);
ecl_kw_type * ecl_kw_alloc_empty();

// ecl_grid.h
ecl_grid_type * ecl_grid_alloc(const char * , bool);
const  char   * ecl_grid_get_name( const ecl_grid_type * );
void            ecl_grid_free(ecl_grid_type * );
void ecl_grid_summarize(const ecl_grid_type * ecl_grid);
void            ecl_grid_get_dims(const ecl_grid_type * , int *OUTPUT, int *OUTPUT , int *OUTPUT , int *OUTPUT);
void ecl_grid_get_ijk1(const ecl_grid_type * grid , int global_index, int *OUTPUT, int *OUTPUT , int *OUTPUT);

// ANSI-C
void free(void *ptr);
