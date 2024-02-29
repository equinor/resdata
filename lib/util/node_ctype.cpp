#include <stdio.h>
#include <stdlib.h>

#include <ert/util/util.hpp>
#include <ert/util/node_ctype.hpp>

const char *node_ctype_name(node_ctype ctype) {
    const char *name;
    switch (ctype) {
    case (CTYPE_VOID_POINTER):
        name = "void pointer";
        break;
    case (CTYPE_INT_VALUE):
        name = "integer value";
        break;
    case (CTYPE_DOUBLE_VALUE):
        name = "double value";
        break;
    case (CTYPE_FLOAT_VALUE):
        name = "float_value";
        break;
    case (CTYPE_CHAR_VALUE):
        name = "char value";
        break;
    case (CTYPE_BOOL_VALUE):
        name = "bool value";
        break;
    case (CTYPE_SIZE_T_VALUE):
        name = "size_t value";
        break;
    default:
        name = NULL;
        util_abort("%s: fatal internal error node_ctype:%d not recognized - "
                   "aborting. \n",
                   __func__, ctype);
    }
    return name;
}
