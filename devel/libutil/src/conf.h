#ifndef __CONF_H__
#define __CONF_H__

#include <stringlist.h>
#ifdef __cplusplus
extern "C" {
#endif

#define CONF_OK           0
#define CONF_PARSE_ERROR  1
const 


typedef struct spec_struct spec_type;
typedef struct conf_struct conf_type;

typedef int (validator_ftype)(conf_type * conf);




#ifdef __cplusplus
}
#endif
#endif


