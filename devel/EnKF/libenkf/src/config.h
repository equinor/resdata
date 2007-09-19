#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <stdio.h>
#include <stdlib.h>
#include <enkf_types.h>

#define CONFIG_STD_FIELDS \
int serial_size;          \
int data_size;            \
int serial_offset;        \
char * ecl_kw_name;       \
enkf_var_type var_type;   \
char * ensfile;           \
char * eclfile;          


#define VOID_CONFIG_FREE(prefix)            void prefix ## _config_free__(void *void_arg) { prefix ## _config_free((prefix ## _config_type *) void_arg); }
#define VOID_CONFIG_FREE_HEADER(prefix)     void prefix ## _config_free__(void *)

/*****************************************************************/

#define GET_SERIAL_SIZE(prefix)                                              \
int prefix ## _config_get_serial_size (const prefix ## _config_type *arg) {  \
   return arg->serial_size;                                                  \
}
#define GET_SERIAL_SIZE_HEADER(prefix)      int prefix ## _config_get_serial_size (const prefix ## _config_type *)
#define VOID_GET_SERIAL_SIZE(prefix)        int prefix ## _config_get_serial_size__ (const void *void_arg ) { return prefix ## _config_get_serial_size((const prefix ## _config_type *) void_arg); }
#define VOID_GET_SERIAL_SIZE_HEADER(prefix) int prefix ## _config_get_serial_size__ (const void *)

/*****************************************************************/
#define GET_SERIAL_OFFSET(prefix)                                              \
int prefix ## _config_get_serial_offset (const prefix ## _config_type *arg) {  \
   return arg->serial_offset;                                                  \
}
#define GET_SERIAL_OFFSET_HEADER(prefix)      int prefix ## _config_get_serial_offset (const prefix ## _config_type *)
/*****************************************************************/

#define SET_SERIAL_OFFSET(prefix)                                                           \
void prefix ## _config_set_serial_offset (prefix ## _config_type *arg, int offset) {        \
   arg->serial_offset = offset;                                                             \
}
#define SET_SERIAL_OFFSET_HEADER(prefix)      void prefix ## _config_set_serial_offset   ( prefix ## _config_type * , int)
#define VOID_SET_SERIAL_OFFSET(prefix)        void prefix ## _config_set_serial_offset__ ( void *void_arg , int offset) { prefix ## _config_set_serial_offset(( prefix ## _config_type *) void_arg , offset); }
#define VOID_SET_SERIAL_OFFSET_HEADER(prefix) void prefix ## _config_set_serial_offset__ ( void *, int )

/*****************************************************************/

#define GET_DATA_SIZE(prefix)               int prefix ## _config_get_data_size (const prefix ## _config_type *arg) { return arg->data_size; }
#define GET_DATA_SIZE_HEADER(prefix)        int prefix ## _config_get_data_size (const prefix ## _config_type *)
/*
  #define VOID_GET_DATA_SIZE(prefix)          int prefix ## _config_get_data_size__ (const void *void_arg) { return prefix ## _config_get_data_size((const prefix ## _config_type *) void_arg); }
  #define VOID_GET_DATA_SIZE_HEADER(prefix)   int prefix ## _config_get_data_size__ (const void *)
*/


/*****************************************************************/


#define CONFIG_SET_ECLFILE(prefix)                                                            \
void prefix ## _config_set_eclfile(prefix ## _config_type *config , const char * file) {       \
  config->eclfile = realloc(config->eclfile , strlen(file) + 1);                             \
  strcpy(config->eclfile , file);                                                             \
}

#define CONFIG_SET_ENSFILE(prefix)                                                            \
void prefix ## _config_set_ensfile(prefix ## _config_type *config , const char * file) {       \
  config->ensfile = realloc(config->ensfile , strlen(file) + 1);                             \
  strcpy(config->ensfile , file);                                                             \
}


#define CONFIG_SET_ENSFILE_VOID(prefix)                                            \
void prefix ## _config_set_ensfile__(void *void_config , const char * file) {       \
   prefix ## _config_type * config = (prefix ## _config_type *) void_config;        \
   prefix ## _config_set_ensfile(config , file);                                    \
}

#define CONFIG_SET_ECLFILE_VOID(prefix)                                            \
void prefix ## _config_set_eclfile__(void *void_config , const char * file) {       \
   prefix ## _config_type * config = (prefix ## _config_type *) void_config;        \
   prefix ## _config_set_eclfile(config , file);                                    \
}

/*****************************************************************/

#define CONFIG_SET_ECLFILE_HEADER(prefix) 	void prefix ## _config_set_eclfile  (prefix ## _config_type *, const char * );
#define CONFIG_SET_ENSFILE_HEADER(prefix) 	void prefix ## _config_set_ensfile  (prefix ## _config_type *, const char * );
#define CONFIG_SET_ECLFILE_HEADER_VOID(prefix) void prefix ## _config_set_eclfile__(void *, const char * );
#define CONFIG_SET_ENSFILE_HEADER_VOID(prefix) void prefix ## _config_set_ensfile__(void *, const char * );

/*
  #define CONFIG_INIT_STD_FIELDS     config->ecl_file = NULL; config->ens_file = NULL;
*/

#define CONFIG_FREE_STD_FIELDS  \
if (config->ensfile != NULL) free(config->ensfile);    \
if (config->eclfile != NULL) free(config->eclfile);


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

/*****************************************************************/
#define VOID_ALLOC_ENSFILE(prefix)                                     \
char * prefix ## _alloc_ensfile__(const void * void_arg, const char *path)        {\
   return prefix ## _alloc_ensfile((const prefix ##_type *) void_arg , path); \
}

#define VOID_ALLOC_ENSFILE_HEADER(prefix) char * prefix ## _alloc_ensfile__(const void * , const char *)

/*****************************************************************/

#define VOID_SWAPIN(prefix) \
void prefix ## _swapin__(void *void_arg , const char * file) { \
   prefix ## _swapin((prefix ## _type *) void_arg , file);     \
}                                                            

#define VOID_SWAPOUT(prefix) \
char * prefix ## _swapout__(void *void_arg , const char * file) { \
   return prefix ## _swapout((prefix ## _type *) void_arg , file);     \
}                                                            


#define VOID_SWAPIN_HEADER(prefix)  void prefix ## _swapin__(void * , const char * );
#define VOID_SWAPOUT_HEADER(prefix) char * prefix ## _swapout__(void * , const char * );

/*****************************************************************/

#define CONFIG_GET_ECL_KW_NAME(prefix)        const char * prefix ## _config_get_ecl_kw_name(const prefix ## _config_type * config) { return config->ecl_kw_name; }
#define CONFIG_GET_ECL_KW_NAME_HEADER(prefix) const char * prefix ## _config_get_ecl_kw_name(const prefix ## _config_type * )


/*****************************************************************/

#define VOID_SERIALIZE(prefix)     \
void prefix ## _serialize__(const void *void_arg, double *serial_data , size_t *_offset) { \
   const prefix ## _type  *arg = (const prefix ## _type *) void_arg;       \
   prefix ## _serialize (arg , serial_data , _offset);   \
}

#define VOID_SERIALIZE_HEADER(prefix) void prefix ## _serialize__(const void *, double *, size_t *);

/*****************************************************************/

#define VOID_GET_OBS(prefix)   \
void prefix ## _get_observations__(const void * void_arg , int report_step, obs_data_type * obs_data) {   \
   prefix ## _get_observations((prefix ## _type *) void_arg , report_step , obs_data); \
}

#define VOID_GET_OBS_HEADER(prefix) void prefix ## _get_observations__(const void * , int , obs_data_type *)

/*****************************************************************/

#define VOID_MEASURE(prefix)   \
void prefix ## _measure__(const void * void_arg ,  const double * serial_data ,meas_data_type * meas_data) {   \
   prefix ## _measure((const prefix ## _type *) void_arg , serial_data , meas_data); \
}

#define VOID_MEASURE_HEADER(prefix) void prefix ## _measure__(const void * ,  const double * , meas_data_type *)


/*****************************************************************/

#define VOID_TRUNCATE(prefix)         void prefix ## _truncate__(void * void_arg) { prefix ## _truncate( (prefix ## _type *) void_arg); }
#define VOID_TRUNCATE_HEADER(prefix)  void prefix ## _truncate__(void * )

#endif
