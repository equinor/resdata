#ifndef __ECL_IO_CONFIG_H__
#define __ECL_IO_CONFIG_H__


typedef struct ecl_io_config_struct ecl_io_config_type;

/* Modifiers */
void 		     ecl_io_config_set_formatted(ecl_io_config_type *, bool );
void 		     ecl_io_config_set_endian_flip(ecl_io_config_type *, bool );
void 		     ecl_io_config_set_unified(ecl_io_config_type *, bool );


/* Accesors */
bool 		     ecl_io_config_get_formatted(ecl_io_config_type *);
bool 		     ecl_io_config_get_unified(ecl_io_config_type *);
bool 		     ecl_io_config_get_endian_flip(ecl_io_config_type *);


/* Allocater & destructor */
ecl_io_config_type * ecl_io_config_alloc(bool ,bool , bool);
void                 ecl_io_config_free(ecl_io_config_type * );

#endif
