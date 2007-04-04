#ifndef __ECL_CONFIG_H__
#define __ECL_CONFIG_H__


typedef struct ecl_config_struct ecl_config_type;

ecl_config_type * ecl_config_alloc(const char *);
char            * ecl_config_alloc_eclname(const ecl_config_type * , const char * );
void              ecl_config_free(ecl_config_type *);


#endif
