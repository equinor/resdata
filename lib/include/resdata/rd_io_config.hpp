#ifndef ERT_RD_IO_CONFIG_H
#define ERT_RD_IO_CONFIG_H
#ifdef __cplusplus
extern "C" {
#endif

typedef struct rd_io_config_struct rd_io_config_type;

/* Modifiers */
void rd_io_config_set_formatted(rd_io_config_type *, bool);
void rd_io_config_set_unified_restart(rd_io_config_type *, bool);
void rd_io_config_set_unified_summary(rd_io_config_type *, bool);

/* Accesors */
bool rd_io_config_get_formatted(rd_io_config_type *);
bool rd_io_config_get_unified_restart(rd_io_config_type *);
bool rd_io_config_get_unified_summary(rd_io_config_type *);

/* Allocater & destructor */
rd_io_config_type *rd_io_config_alloc(bool, bool, bool);
void rd_io_config_free(rd_io_config_type *);

#ifdef __cplusplus
}
#endif
#endif
