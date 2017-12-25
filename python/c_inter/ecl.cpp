#include <Python.h>
#include <ert/ecl/ecl_python_init.h>

typedef struct ecl_grid_struct ecl_grid_type;


static PyMethodDef module_methods[] = {
    {NULL, NULL}  /* sentinel */
};


PyDoc_STRVAR(module_doc,
             "A module for loading adn writing eclipse binary files.");


PyMODINIT_FUNC
init_ecl(void)
{
    PyObject *m = NULL;

    if (ecl_python_init() == 0)
      return;

    m = Py_InitModule3("_ecl", module_methods, module_doc);
    if (m == NULL)
      return;
}
