#ifdef __BYTE_ORDER
#if  __BYTE_ORDER == __LITTLE_ENDIAN 
#define ENDIAN_FLIP true
#else
#define ENDIAN_FLIP false
#endif               
#else
ABORT: macro __BYTE_ORDER not defined?
#endif 
