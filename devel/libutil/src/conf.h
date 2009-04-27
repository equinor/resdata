#ifndef __CONF_H__
#define __CONF_H__

#include <stringlist.h>
#ifdef __cplusplus
extern "C" {
#endif

#define CONF_OK                      0
#define CONF_PARSE_ERROR             1
#define CONF_CIRCULAR_INCLUDE_ERROR  2
#define CONF_UNEXPECTED_EOF_ERROR    3
#define CONF_UNABLE_TO_OPEN_FILE     4



typedef struct conf_struct      conf_type;
typedef struct conf_item_struct conf_item_type; 
typedef struct conf_spec_struct conf_spec_type;

typedef int (validator_ftype)(conf_type * conf);




#ifdef __cplusplus
}
#endif
#endif


