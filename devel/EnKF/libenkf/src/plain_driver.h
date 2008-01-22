#ifndef __PLAIN_DRIVER_H__
#define __PLAIN_DRIVER_H__
#include <path_fmt.h>



typedef struct plain_driver_struct plain_driver_type;


plain_driver_type * plain_driver_alloc(path_fmt_type * path);

#endif
