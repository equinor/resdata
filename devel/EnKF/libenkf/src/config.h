#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <stdio.h>
#include <stdlib.h>

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


#endif
