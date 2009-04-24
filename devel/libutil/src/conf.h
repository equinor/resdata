#ifndef __CONF_H__
#define __CONF_H__

#include <stringlist.h>
#ifdef __cplusplus
extern "C" {
#endif

#define CONF_OK                      0
#define CONF_PARSE_ERROR             1
#define CONF_RECURSIVE_INCLUDE_ERROR 2
#define CONF_UNEXPECTED_EOF_ERROR    3
#define CONF_UNABLE_TO_OPEN_FILE     4

const 


typedef struct spec_struct spec_type;
typedef struct conf_struct conf_type;

typedef int (validator_ftype)(conf_type * conf);



int create_token_buffer(
  stringlist_type ** tokens,
  stringlist_type ** src_files,
  const char       * filename
);

#ifdef __cplusplus
}
#endif
#endif


