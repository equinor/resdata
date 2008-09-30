#include <ecl_util.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ecl_io_config.h>
#include <util.h>


/**
   This file implements a pathetically small struct which is used to
   pack three booleans representing eclipse IO configuration. The
   three config items which are stored are:

    * formatted   : whether to use formatted files.
    * endian_flip : whether an endian flip should be performed on binary files.
    * unified     : whether unified summary || restart files should be used.
  
   All types are implemented by an internal enum which supports a
   undefined type. The rationale for this is to provide functionality
   to 'guess' type based on arbitrary input. If for instance the input
   file is formatted, it is impossible to infer whether we should flip
   endian ness.
*/


typedef enum {  flip           = 0,
		noflip         = 1,
		flip_undefined = 2 } endian_flip_type;

typedef enum {  unified        = 0,
	        multiple       = 1, 
	        unif_undefined = 2 } unified_type;

typedef enum {  formatted     = 0,
	        unformatted   = 1,
	        fmt_undefined = 2 } formatted_type;


struct ecl_io_config_struct {
  formatted_type     formatted;
  endian_flip_type   endian_flip;
  unified_type       unified;
};


/*****************************************************************/

static ecl_io_config_type * ecl_io_config_alloc__() {
  ecl_io_config_type * ecl_io_config = util_malloc(sizeof * ecl_io_config , __func__);

  ecl_io_config->formatted   = fmt_undefined;
  ecl_io_config->endian_flip = flip_undefined;
  ecl_io_config->unified     = unif_undefined;

  return ecl_io_config;
}



void ecl_io_config_set_formatted(ecl_io_config_type * io_config, bool formatted) {
  if (formatted)
    io_config->formatted = formatted;
  else
    io_config->formatted = unformatted;
}



void ecl_io_config_set_endian_flip(ecl_io_config_type * io_config, bool endian_flip) {
  if (endian_flip)
    io_config->endian_flip = flip;
  else
    io_config->endian_flip = noflip;
}



void ecl_io_config_set_unified(ecl_io_config_type * io_config, bool unified) {
  if (unified)
    io_config->unified = unified;
  else
    io_config->unified = multiple;
}


bool ecl_io_config_get_formatted(ecl_io_config_type * io_config) {
  if (io_config->formatted == formatted)
    return true;
  else if (io_config->formatted == unformatted)
    return false;
  else {
    util_abort("%s: formatted_state == undefined - sorry \n",__func__);
    return false; /* Compiler shut up */
  }
}


bool ecl_io_config_get_unified(ecl_io_config_type * io_config) {
  if (io_config->unified == unified)
    return true;
  else if (io_config->unified == multiple)
    return false;
  else {
    util_abort("%s: formatted_state == undefined - sorry \n",__func__);
    return false; /* Compiler shut up */
  }
}


bool ecl_io_config_get_endian_flip(ecl_io_config_type * io_config) {
  if (io_config->endian_flip == flip)
    return true;
  else if (io_config->endian_flip == noflip)
    return false;
  else {
    util_abort("%s: formatted_state == undefined - sorry \n",__func__);
    return false; /* Compiler shut up */
  }
}



ecl_io_config_type * ecl_io_config_alloc(bool formatted , bool endian_flip , bool unified) {
  ecl_io_config_type * ecl_io_config = ecl_io_config_alloc__();

  ecl_io_config_set_formatted( ecl_io_config , formatted );
  ecl_io_config_set_unified( ecl_io_config , unified );
  ecl_io_config_set_endian_flip( ecl_io_config , endian_flip );

  return ecl_io_config;
}
  
  
  
  
void ecl_io_config_free(ecl_io_config_type * io_config) {
  free(io_config);
}
  
