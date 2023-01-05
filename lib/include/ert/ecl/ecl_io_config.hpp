#ifndef ERT_ECL_IO_CONFIG_H
#define ERT_ECL_IO_CONFIG_H
#ifdef __cplusplus
extern "C" {
#endif

typedef struct ecl_io_config_struct ecl_io_config_type;

/* Modifiers */
void ecl_io_config_set_formatted(ecl_io_config_type *, bool);
void ecl_io_config_set_unified_restart(ecl_io_config_type *, bool);
void ecl_io_config_set_unified_summary(ecl_io_config_type *, bool);

/* Accesors */
bool ecl_io_config_get_formatted(ecl_io_config_type *);
bool ecl_io_config_get_unified_restart(ecl_io_config_type *);
bool ecl_io_config_get_unified_summary(ecl_io_config_type *);

/* Allocater & destructor */
ecl_io_config_type *ecl_io_config_alloc(bool, bool, bool);
void ecl_io_config_free(ecl_io_config_type *);

#ifdef __cplusplus
}
#endif
#endif
