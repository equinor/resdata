#ifndef ERT_SSIZE_T_H
#define ERT_SSIZE_T_H

#ifdef _MSC_VER
/* maximum number of bytes addressable */
#ifdef _WIN64
typedef __int64 ssize_t;
#else
typedef long ssize_t;
#endif
#else
/* POSIX 2008 states that it should be defined here */
#include <sys/types.h>
#endif

#endif
