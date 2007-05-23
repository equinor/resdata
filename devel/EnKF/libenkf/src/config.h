#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <stdio.h>
#include <stdlib.h>
#include <enkf_types.h>

#define CONFIG_STD_FIELDS \
int size;                 \
const char * ecl_kw_name; \
enkf_var_type var_type;   \
char * ens_name;          \
char * ens_file;          \
char * ecl_file;          \
char * ecl_name;          

#define CONFIG_GET_SIZE_FUNC(prefix)        int prefix ## _config_get_size (const prefix ## _config_type *arg) { return arg->size; }
#define CONFIG_GET_SIZE_FUNC_HEADER(prefix) int prefix ## _config_get_size (const prefix ## _config_type *)

/*****************************************************************/


#define CONFIG_SET_ECL_FILE(prefix)                                                            \
void prefix ## _config_set_eclfile(prefix ## _config_type *config , const char * path) {       \
  config->ecl_file = realloc(config->ecl_file , strlen(path) + strlen(config->ecl_name) + 2);  \
  sprintf(config->ecl_file , "%s/%s" , path , config->ecl_name);                               \
}


#define CONFIG_SET_ENS_FILE(prefix)                                                            \
void prefix ## _config_set_ensfile(prefix ## _config_type *config , const char * path) {       \
  config->ens_file = realloc(config->ens_file , strlen(path) + strlen(config->ens_name) + 2);  \
  sprintf(config->ens_file , "%s/%s" , path , config->ens_name);                               \
}

#define CONFIG_SET_ENS_FILE_VOID(prefix)                                            \
void prefix ## _config_set_ensfile__(void *void_config , const char * path) {       \
   prefix ## _config_type * config = (prefix ## _config_type *) void_config;        \
   prefix ## _config_set_ensfile(config , path);                                    \
}

#define CONFIG_SET_ECL_FILE_VOID(prefix)                                            \
void prefix ## _config_set_eclfile__(void *void_config , const char * path) {       \
   prefix ## _config_type * config = (prefix ## _config_type *) void_config;        \
   prefix ## _config_set_eclfile(config , path);                                    \
}

/*****************************************************************/

#define CONFIG_SET_ECL_FILE_HEADER(prefix) 	void prefix ## _config_set_eclfile  (prefix ## _config_type *, const char * );
#define CONFIG_SET_ENS_FILE_HEADER(prefix) 	void prefix ## _config_set_ensfile  (prefix ## _config_type *, const char * );
#define CONFIG_SET_ECL_FILE_HEADER_VOID(prefix) void prefix ## _config_set_eclfile__(void *, const char * );
#define CONFIG_SET_ENS_FILE_HEADER_VOID(prefix) void prefix ## _config_set_ensfile__(void *, const char * );


#define CONFIG_INIT_STD_FIELDS     config->ecl_file = NULL; config->ens_file = NULL;

#define CONFIG_FREE_STD_FIELDS  \
if (config->ens_name != NULL) free(config->ens_name);    \
if (config->ens_file != NULL) free(config->ens_file);    \
if (config->ecl_file != NULL) free(config->ecl_file);    \
if (config->ecl_name != NULL) free(config->ecl_name);


/*****************************************************************/

#define VOID_ALLOC(prefix) \
void * prefix ## _alloc__(const void *void_config) {                      \
  return prefix ## _alloc((const prefix ## _config_type *) void_config);  \
}

#define VOID_ALLOC_HEADER(prefix) void * prefix ## _alloc__(const void *)


/*****************************************************************/

#define VOID_ENS_WRITE(prefix) \
void prefix ## _ens_write__(const void * void_arg , const char * path) { \
   prefix ## _ens_write((const prefix ## _type *) void_arg , path);      \
}

#define VOID_ENS_READ(prefix) \
void prefix ## _ens_read__(void * void_arg , const char * path) { \
   prefix ## _ens_read((prefix ## _type *) void_arg , path);      \
}

#define VOID_ENS_WRITE_HEADER(prefix) void prefix ## _ens_write__(const void * , const char * );
#define VOID_ENS_READ_HEADER(prefix) void prefix ## _ens_read__(void * , const char * );


/*****************************************************************/

#define VOID_ECL_WRITE(prefix) \
void prefix ## _ecl_write__(const void * void_arg , const char * path) { \
   prefix ## _ecl_write((const prefix ## _type *) void_arg , path);      \
}

#define VOID_ECL_READ(prefix) \
void prefix ## _ecl_read__(void * void_arg , const char * path) { \
   prefix ## _ecl_read((prefix ## _type *) void_arg , path);      \
}

#define VOID_ECL_WRITE_HEADER(prefix) void prefix ## _ecl_write__(const void * , const char * );
#define VOID_ECL_READ_HEADER(prefix) void prefix ## _ecl_read__(void * , const char * );


/*****************************************************************/

#define VOID_FREE(prefix)                        \
void prefix ## _free__(void * void_arg) {         \
   prefix ## _free((prefix ## _type *) void_arg); \
}

#define VOID_FREE_HEADER(prefix) void prefix ## _free__(void * );


/*****************************************************************/

#define VOID_FREE_DATA(prefix)                        \
void prefix ## _free_data__(void * void_arg) {         \
   prefix ## _free_data((prefix ## _type *) void_arg); \
}

#define VOID_FREE_DATA_HEADER(prefix) void prefix ## _free_data__(void * );

/*****************************************************************/

#define VOID_REALLOC_DATA(prefix)                        \
void prefix ## _realloc_data__(void * void_arg) {         \
   prefix ## _realloc_data((prefix ## _type *) void_arg); \
}

#define VOID_REALLOC_DATA_HEADER(prefix) void prefix ## _realloc_data__(void * );

/*****************************************************************/

#define VOID_COPYC(prefix)                                      \
void * prefix ## _copyc__(const void * void_arg) {    \
   return prefix ## _copyc((const prefix ## _type *) void_arg); \
}

#define VOID_COPYC_HEADER(prefix) void * prefix ## _copyc__(const void * )


#endif
