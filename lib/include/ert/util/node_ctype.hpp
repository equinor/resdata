#ifndef ERT_NODE_CTYPE_H
#define ERT_NODE_CTYPE_H
#ifdef __cplusplus
extern "C" {
#endif

/*
  value : means a scalar which has been packed into the container
          object.

  pointer: means a (typed) pointer which points to a memory location
           outside the container object (however the container can own
           the memory).

*/

typedef enum {
    CTYPE_VOID_POINTER = 1,
    CTYPE_INT_VALUE = 2,
    CTYPE_DOUBLE_VALUE = 3,
    CTYPE_FLOAT_VALUE = 4,
    CTYPE_CHAR_VALUE = 5,
    CTYPE_BOOL_VALUE = 6,
    CTYPE_SIZE_T_VALUE = 7,
    CTYPE_INVALID = 100
} node_ctype;

const char *node_ctype_name(node_ctype);
#ifdef __cplusplus
}
#endif
#endif
