#include <stdbool.h>
#include <stdlib.h>

#include <ert/util/util.hpp>

#include <resdata/rd_io_config.hpp>
#include <resdata/rd_util.hpp>

/**
   This file implements a pathetically small struct which is used to
   pack three booleans representing eclipse IO configuration. The
   three config items which are stored are:

    * formatted   : whether to use formatted files.
    * unified     : whether unified summary || restart files should be used.

   All types are implemented by an internal enum which supports a
   undefined type. The rationale for this is to provide functionality
   to 'guess' type based on arbitrary input. If for instance the input
   file is formatted, it is impossible to infer whether we should flip
   endian ness.
*/

typedef enum { UNIFIED = 0, MULTIPLE = 1, UNIF_UNDEFINED = 2 } unified_type;

typedef enum {
    FORMATTED = 0,
    UNFORMATTED = 1,
    FMT_UNDEFINED = 2
} formatted_type;

struct rd_io_config_struct {
    formatted_type formatted;
    unified_type unified_restart;
    unified_type unified_summary;
};

static rd_io_config_type *rd_io_config_alloc__() {
    rd_io_config_type *rd_io_config =
        (rd_io_config_type *)util_malloc(sizeof *rd_io_config);

    rd_io_config->formatted = FMT_UNDEFINED;
    rd_io_config->unified_restart = UNIF_UNDEFINED;
    rd_io_config->unified_summary = UNIF_UNDEFINED;

    return rd_io_config;
}

void rd_io_config_set_formatted(rd_io_config_type *io_config, bool formatted) {
    if (formatted)
        io_config->formatted = FORMATTED;
    else
        io_config->formatted = UNFORMATTED;
}

void rd_io_config_set_unified_restart(rd_io_config_type *io_config,
                                      bool unified) {
    if (unified)
        io_config->unified_restart = UNIFIED;
    else
        io_config->unified_restart = MULTIPLE;
}

void rd_io_config_set_unified_summary(rd_io_config_type *io_config,
                                      bool unified) {
    if (unified)
        io_config->unified_summary = UNIFIED;
    else
        io_config->unified_summary = MULTIPLE;
}

bool rd_io_config_get_formatted(rd_io_config_type *io_config) {
    if (io_config->formatted == FORMATTED)
        return true;
    else if (io_config->formatted == UNFORMATTED)
        return false;
    else {
        util_abort("%s: formatted_state == undefined - sorry \n", __func__);
        return false; /* Compiler shut up */
    }
}

bool rd_io_config_get_unified_summary(rd_io_config_type *io_config) {
    if (io_config->unified_summary == UNIFIED)
        return true;
    else if (io_config->unified_summary == MULTIPLE)
        return false;
    else {
        util_abort("%s: unified_state == undefined - sorry \n", __func__);
        return false; /* Compiler shut up */
    }
}

bool rd_io_config_get_unified_restart(rd_io_config_type *io_config) {
    if (io_config->unified_restart == UNIFIED)
        return true;
    else if (io_config->unified_restart == MULTIPLE)
        return false;
    else {
        util_abort("%s: formatted_state == undefined - sorry \n", __func__);
        return false; /* Compiler shut up */
    }
}

rd_io_config_type *rd_io_config_alloc(bool formatted, bool unified_summary,
                                      bool unified_restart) {
    rd_io_config_type *rd_io_config = rd_io_config_alloc__();

    rd_io_config_set_formatted(rd_io_config, formatted);
    rd_io_config_set_unified_restart(rd_io_config, unified_restart);
    rd_io_config_set_unified_summary(rd_io_config, unified_summary);

    return rd_io_config;
}

void rd_io_config_free(rd_io_config_type *io_config) { free(io_config); }
