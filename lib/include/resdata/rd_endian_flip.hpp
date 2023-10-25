#ifndef ERT_RD_ENDIAN_FLIP_H
#define ERT_RD_ENDIAN_FLIP_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

/**
   This header file checks if the restart format endianness and the hardware
   endianness are equal, and defines the macro RD_ENDIAN_FLIP
   accordingly.

   All the rd_xxx functions will use the RD_ENDIAN_FLIP macro to
   determine whether the endian flip should be performed. When opening
   a fortio instance explicitly you can use the RD_ENDIAN_FLIP macro
   to get the endianness correct.
*/

#define RD_BYTE_ORDER __BIG_ENDIAN // Alternatively: __LITTLE_ENDIAN

#ifdef BYTE_ORDER
#if BYTE_ORDER == RD_BYTE_ORDER
#define RD_ENDIAN_FLIP false
#else
#define RD_ENDIAN_FLIP true
#endif
#else
#ifdef WIN32
#define RD_ENDIAN_FLIP true // Unconditional byte flip on Windows.
#else
#error : The macro BYTE_ORDER is not defined?
#endif
#endif

#undef RD_BYTE_ORDER

#ifdef __cplusplus
}
#endif
#endif
