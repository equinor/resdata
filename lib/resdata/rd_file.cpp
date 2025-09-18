#include <stdio.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <time.h>

#include <ert/util/hash.hpp>
#include <ert/util/util.hpp>
#include <ert/util/vector.hpp>
#include <ert/util/int_vector.hpp>
#include <ert/util/stringlist.hpp>

#include <resdata/fortio.h>
#include <resdata/rd_kw.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/rd_file_view.hpp>
#include <resdata/rd_endian_flip.hpp>
#include <resdata/rd_kw_magic.hpp>
#include <resdata/rd_rsthead.hpp>
#include <resdata/rd_file_kw.hpp>
#include <resdata/rd_type.hpp>

/**
   This file implements functionality to load a file in
   restart format. The implementation works by first searching through
   the complete file to create an index over all the keywords present
   in the file. The actual keyword data is not loaded before they are
   explicitly requested.

   The rd_file_type is the middle layer of abstraction in the resdata
   hierarchy (see the file overview.txt in this directory); it works
   with a collection of rd_kw instances and has various query
   functions, however it does not utilize knowledge of the
   structure/content of the files in the way e.g. rd_grid.c does[1].

   The main datatype here is the rd_file type, but in addition each
   rd_kw instance is wrapped in an rd_file_kw (implemented in
   rd_file_kw.c) structure and all the indexing is implemented with
   the rd_file_view type. The rd_file_view type is not used outside this file.

   When the file is opened an index of all the keywords is created and
   stored in the field global_map, and the field active_view is set to
   point to global_map, i.e. all query/get operations on the rd_file
   will be based on the complete index:

   In many cases (in particular for unified restart files) it is quite
   painful to work with this large and unvieldy index, and it is
   convenient to create a sub index based on a subset of the
   keywords. The creation of these sub indices is based on identifying
   a keyword from name and occurence number, and then including all
   keywords up to the next occurence of the same keyword:

      SEQHDR            ---\
      MINISTEP  0          |
      PARAMS    .....      |
      MINISTEP  1          |   Block 0
      PARAMS    .....      |
      MINISTEP  2          |
      PARAMS    .....      |
      SEQHDR            ---+
      MINISTEP  3          |
      PARAMS    .....      |
      MINISTEP  4          |   Block 1
      PARAMS    .....      |
      MINISTEP  5          |
      SEQHDR            ---+
      MINISTEP  6          |   Block 2
      PARAMS    ....       |
      SEQHDR            ---+
      MINISTEP  7          |
      PARAMS    ....       |   Block 3
      MINISTEP  8          |
      PARAMS    ....       |

   For the unified summary file depicted here e.g. the call

      rd_file_get_blockmap( rd_file , "SEQHDR" , 2 )

   Will create a sub-index consisting of the (three) keywords in what
   is called 'Block 2' in the figure above. In particular for restart
   files this abstraction is very convenient, because an extra layer
   of functionality is required to get from natural time coordinates
   (i.e. simulation time or report step) to the occurence number (see
   rd_rstfile for more details).

   [1]: This is not entirely true - in the file rd_rstfile.c; which
        is included from this file are several specialized function
        for working with restart files. However the restart files are
        still treated as collections of rd_kw instances, and not
        internalized as in e.g. rd_sum.
*/

#define RD_FILE_ID 776107

struct rd_file_struct {
    UTIL_TYPE_ID_DECLARATION;
    fortio_type *fortio; /* The source of all the keywords - must be retained
                                       open for reading for the entire lifetime of the
                                       rd_file object. */
    rd_file_view_type
        *global_view; /* The index of all the rd_kw instances in the file. */
    rd_file_view_type *active_view; /* The currently active index. */
    bool read_only;
    int flags;
    vector_type *map_stack;
    inv_map_type *inv_view;
};

/*
  This illustrates the indexing. The rd_file instance contains in
  total 7 rd_kw instances, the global index [0...6] is the internal
  way to access the various keywords. The kw_index is a hash table
  with entries 'SEQHDR', 'MINISTEP' and 'PARAMS'. Each entry in the
  hash table is an integer vector which again contains the internal
  index of the various occurences:

   ------------------
   SEQHDR            \
   MINISTEP  0        |
   PARAMS    .....    |
   MINISTEP  1        |
   PARAMS    .....    |
   MINISTEP  2        |
   PARAMS    .....   /
   ------------------

   kw_index    = {"SEQHDR": [0], "MINISTEP": [1,3,5], "PARAMS": [2,4,6]}    <== This is hash table.
   kw_list     = [SEQHDR , MINISTEP , PARAMS , MINISTEP , PARAMS , MINISTEP , PARAMS]

*/

UTIL_SAFE_CAST_FUNCTION(rd_file, RD_FILE_ID)
UTIL_IS_INSTANCE_FUNCTION(rd_file, RD_FILE_ID)

static rd_file_type *rd_file_alloc_empty(int flags) {
    rd_file_type *rd_file = (rd_file_type *)util_malloc(sizeof *rd_file);
    UTIL_TYPE_ID_INIT(rd_file, RD_FILE_ID);
    rd_file->map_stack = vector_alloc_new();
    rd_file->inv_view = inv_map_alloc();
    rd_file->flags = flags;
    return rd_file;
}

void rd_file_fwrite_fortio(const rd_file_type *rd_file, fortio_type *target,
                           int offset) {
    rd_file_view_fwrite(rd_file->active_view, target, offset);
}

/**
   Here comes several functions for querying the rd_file instance, and
   getting pointers to the rd_kw content of the rd_file. For getting
   rd_kw instances there are two principally different access methods:

   * rd_file_iget_named_kw(): This function will take a keyword
   (char *) and an integer as input. The integer corresponds to the
   ith occurence of the keyword in the file.

   * rd_file_iget_kw(): This function just takes an integer index as
   input, and returns the corresponding rd_kw instance - without
   considering which keyword it is.

*/

/*
  Will return the number of times a particular keyword occurs in a
  rd_file instance. Will return 0 if the keyword can not be found.
*/

int rd_file_get_num_named_kw(const rd_file_type *rd_file, const char *kw) {
    return rd_file_view_get_num_named_kw(rd_file->active_view, kw);
}

/**
    Returns the total number of rd_kw instances in the rd_file
    instance.
*/
int rd_file_get_size(const rd_file_type *rd_file) {
    return rd_file_view_get_size(rd_file->active_view);
}

/**
   Returns true if the rd_file instance has at-least one occurence of
   rd_kw 'kw'.
*/
bool rd_file_has_kw(const rd_file_type *rd_file, const char *kw) {
    return rd_file_view_has_kw(rd_file->active_view, kw);
}

const char *rd_file_get_src_file(const rd_file_type *rd_file) {
    return fortio_filename_ref(rd_file->fortio);
}

rd_kw_type *rd_file_iget_kw(const rd_file_type *file, int global_index) {
    return rd_file_view_iget_kw(file->active_view, global_index);
}

/*
   This function will return the ith occurence of 'kw' in
   rd_file. Will abort hard if the request can not be satisifed - use
   query functions if you can not take that.
*/

rd_kw_type *rd_file_iget_named_kw(const rd_file_type *file, const char *kw,
                                  int ith) {
    return rd_file_view_iget_named_kw(file->active_view, kw, ith);
}

rd_file_view_type *rd_file_get_global_view(rd_file_type *rd_file) {
    return rd_file->global_view;
}

// Very deprecated ...
rd_file_view_type *rd_file_get_active_view(rd_file_type *rd_file) {
    return rd_file->active_view;
}

rd_file_view_type *rd_file_get_global_blockview(rd_file_type *rd_file,
                                                const char *kw, int occurence) {
    rd_file_view_type *view =
        rd_file_view_add_blockview(rd_file->global_view, kw, occurence);
    return view;
}

static rd_file_view_type *rd_file_alloc_global_blockview2(rd_file_type *rd_file,
                                                          const char *start_kw,
                                                          const char *end_kw,
                                                          int occurence) {
    rd_file_view_type *view = rd_file_view_alloc_blockview2(
        rd_file->global_view, start_kw, end_kw, occurence);
    return view;
}

rd_file_view_type *rd_file_alloc_global_blockview(rd_file_type *rd_file,
                                                  const char *kw,
                                                  int occurence) {
    return rd_file_alloc_global_blockview2(rd_file, kw, kw, occurence);
}

rd_file_view_type *rd_file_get_restart_view(rd_file_type *rd_file,
                                            int input_index, int report_step,
                                            time_t sim_time, double sim_days) {
    rd_file_view_type *view = rd_file_view_add_restart_view(
        rd_file->global_view, input_index, report_step, sim_time, sim_days);
    return view;
}

rd_file_view_type *rd_file_get_summary_view(rd_file_type *rd_file,
                                            int report_step) {
    rd_file_view_type *view =
        rd_file_view_add_summary_view(rd_file->global_view, report_step);
    return view;
}

/*
  Different functions to open and close a file.
*/

/**
   The rd_file_scan() function will scan through the whole file and build up an
   index of all the kewyords. The map created from this scan will be stored
   under the 'global_view' field; and all subsequent lookup operations will
   ultimately be based on the global map.

   The rd_file_scan function will scan through the file as long as it finds
   valid rd_kw instances on the disk; it will return when EOF is encountered or
   an invalid rd_kw instance is detected. This implies that for a partly broken
   file the rd_file_scan function will index the valid keywords which are in
   the file, possible garbage at the end will be ignored.
*/

static void rd_file_scan(rd_file_type *rd_file) {
    fortio_fseek(rd_file->fortio, 0, SEEK_SET);
    {
        rd_kw_type *work_kw = rd_kw_alloc_new("WORK-KW", 0, RD_INT, NULL);

        while (true) {
            if (fortio_read_at_eof(rd_file->fortio))
                break;

            {
                offset_type current_offset = fortio_ftell(rd_file->fortio);
                rd_read_status_enum read_status =
                    rd_kw_fread_header(work_kw, rd_file->fortio);
                if (read_status == RD_KW_READ_FAIL)
                    break;

                if (read_status == RD_KW_READ_OK) {
                    rd_file_kw_type *file_kw =
                        rd_file_kw_alloc(work_kw, current_offset);

                    if (rd_file_kw_fskip_data(file_kw, rd_file->fortio))
                        rd_file_view_add_kw(rd_file->global_view, file_kw);
                    else {
                        rd_file_kw_free(file_kw);
                        break;
                    }
                }
            }
        }

        rd_kw_free(work_kw);
    }
    rd_file_view_make_index(rd_file->global_view);
}

static void rd_file_select_global(rd_file_type *rd_file) {
    rd_file->active_view = rd_file->global_view;
}

/**
   The fundamental open file function; all alternative open()
   functions start by calling this one. This function will read
   through the complete file, extract all the keyword headers and
   create the map/index stored in the global_view field of the rd_file
   structure. No keyword data will be loaded from the file.

   The rd_file instance will retain an open fortio reference to the
   file until rd_file_close() is called.
*/

static fortio_type *rd_file_alloc_fortio(const char *filename, int flags) {
    fortio_type *fortio = NULL;
    bool fmt_file;

    rd_fmt_file(filename, &fmt_file);

    if (rd_file_view_check_flags(flags, RD_FILE_WRITABLE))
        fortio = fortio_open_readwrite(filename, fmt_file, RD_ENDIAN_FLIP);
    else
        fortio = fortio_open_reader(filename, fmt_file, RD_ENDIAN_FLIP);

    return fortio;
}

rd_file_type *rd_file_open(const char *filename, int flags) {
    fortio_type *fortio = rd_file_alloc_fortio(filename, flags);

    if (fortio) {
        rd_file_type *rd_file = rd_file_alloc_empty(flags);
        rd_file->fortio = fortio;
        rd_file->global_view = rd_file_view_alloc(
            rd_file->fortio, &rd_file->flags, rd_file->inv_view, true);

        rd_file_scan(rd_file);
        rd_file_select_global(rd_file);

        if (rd_file_view_check_flags(rd_file->flags, RD_FILE_CLOSE_STREAM))
            fortio_fclose_stream(rd_file->fortio);

        return rd_file;
    } else
        return NULL;
}

bool rd_file_writable(const rd_file_type *rd_file) {
    return rd_file_view_check_flags(rd_file->flags, RD_FILE_WRITABLE);
}

/**
   The rd_file_close() function will close the fortio instance and
   free all the data created by the rd_file instance; this includes
   the rd_kw instances which have been loaded on demand.
*/

void rd_file_close(rd_file_type *rd_file) {
    if (rd_file->fortio != NULL)
        fortio_fclose(rd_file->fortio);

    if (rd_file->global_view)
        rd_file_view_free(rd_file->global_view);

    inv_map_free(rd_file->inv_view);
    vector_free(rd_file->map_stack);
    free(rd_file);
}

bool rd_file_load_all(rd_file_type *rd_file) {
    return rd_file_view_load_all(rd_file->active_view);
}

/* Functions specialized to work with restart files.  */

/* Query functions. */
/**
   Will look through all the INTEHEAD kw instances of the current
   rd_file and look for @sim_time. If the value is found true is
   returned, otherwise false.
*/

bool rd_file_has_sim_time(const rd_file_type *rd_file, time_t sim_time) {
    return rd_file_view_has_sim_time(rd_file->active_view, sim_time);
}

/*
  This function will determine the restart block corresponding to the
  world time @sim_time; if @sim_time can not be found the function
  will return 0.

  The returned index is the 'occurence number' in the restart file,
  i.e. in the (quite typical case) that not all report steps are
  present the return value will not agree with report step.

  The return value from this function can then be used to get a
  corresponding solution field directly, or the file map can
  restricted to this block.

  Direct access:

     int index = rd_file_get_restart_index( rd_file , sim_time );
     if (index >= 0) {
        rd_kw_type * pressure_kw = rd_file_iget_named_kw( rd_file , "PRESSURE" , index );
        ....
     }


  Specially in the case of LGRs the block restriction should be used.
 */

int rd_file_get_restart_index(const rd_file_type *rd_file, time_t sim_time) {
    int active_index =
        rd_file_view_find_sim_time(rd_file->active_view, sim_time);
    return active_index;
}

/**
   Will look through all the SEQNUM kw instances of the current
   rd_file and look for @report_step. If the value is found true is
   returned, otherwise false.
*/

bool rd_file_has_report_step(const rd_file_type *rd_file, int report_step) {
    return rd_file_view_has_report_step(rd_file->active_view, report_step);
}

/**
   This function will look up the INTEHEAD keyword in a rd_file_type
   instance, and calculate simulation date from this instance.

   Will return -1 if the requested INTEHEAD keyword can not be found.
*/

time_t rd_file_iget_restart_sim_date(const rd_file_type *restart_file,
                                     int index) {
    return rd_file_view_iget_restart_sim_date(restart_file->active_view, index);
}

double rd_file_iget_restart_sim_days(const rd_file_type *restart_file,
                                     int index) {
    return rd_file_view_iget_restart_sim_days(restart_file->active_view, index);
}

/*
  The input @file must be either an INIT file or a restart file. Will
  fail hard if an INTEHEAD kw can not be found - or if the INTEHEAD
  keyword is not sufficiently large.

  The eclipse files can distinguish between ECLIPSE300 ( value == 300)
  and ECLIPSE300-Thermal option (value == 500). This function will
  return ECLIPSE300 in both those cases.
*/

rd_version_enum rd_file_get_rd_version(const rd_file_type *file) {
    rd_kw_type *intehead_kw = rd_file_iget_named_kw(file, INTEHEAD_KW, 0);
    int int_value = rd_kw_iget_int(intehead_kw, INTEHEAD_IPROG_INDEX);

    if (int_value == INTEHEAD_ECLIPSE100_VALUE)
        return ECLIPSE100;

    if (int_value == INTEHEAD_ECLIPSE300_VALUE)
        return ECLIPSE300;

    if (int_value == INTEHEAD_ECLIPSE300THERMAL_VALUE)
        return ECLIPSE300_THERMAL;

    if (int_value == INTEHEAD_INTERSECT_VALUE)
        return INTERSECT;

    if (int_value == INTEHEAD_FRONTSIM_VALUE)
        return FRONTSIM;

    util_abort("%s: Simulator version value:%d not recognized \n", __func__,
               int_value);
    return (rd_version_enum)0;
}

/*
  1: Oil
  2: Water
  3: Oil + water
  4: Gas
  5: Gas + Oil
  6: Gas + water
  7: Gas + Water + Oil
*/

int rd_file_get_phases(const rd_file_type *init_file) {
    rd_kw_type *intehead_kw = rd_file_iget_named_kw(init_file, INTEHEAD_KW, 0);
    int phases = rd_kw_iget_int(intehead_kw, INTEHEAD_PHASE_INDEX);
    return phases;
}

/*
bool rd_file_writable( const rd_file_type * rd_file ) {
  return fortio_writable( rd_file->fortio );
}
*/

/*
  Will save the content of @rd_kw to the on-disk file wrapped by the
  rd_file instance. This function is quite strict:

  1. The actual keyword which should be updated is identified using
     pointer comparison; i.e. the rd_kw argument must be the actual
     return value from an earlier rd_file_get_kw() operation.

  2. The header data of the rd_kw must be unmodified; this is checked
     by the rd_file_kw_inplace_fwrite() function and crash-and-burn
     will ensue if this is not satisfied.

  3. The rd_file must have been opened with one of the _writable()
     open functions.
*/

bool rd_file_save_kw(const rd_file_type *rd_file, const rd_kw_type *rd_kw) {
    rd_file_kw_type *file_kw = inv_map_get_file_kw(
        rd_file->inv_view,
        rd_kw); // We just verify that the input rd_kw points to an rd_kw
    if (file_kw !=
        NULL) { // we manage; from then on we use the reference contained in
        if (fortio_assert_stream_open(
                rd_file->fortio)) { // the corresponding rd_file_kw instance.

            rd_file_kw_inplace_fwrite(file_kw, rd_file->fortio);

            if (rd_file_view_check_flags(rd_file->flags, RD_FILE_CLOSE_STREAM))
                fortio_fclose_stream(rd_file->fortio);

            return true;
        } else
            return false;
    } else {
        util_abort("%s: keyword pointer:%p not found in rd_file instance. \n",
                   __func__, rd_kw);
        return false;
    }
}

static rd_file_view_type *rd_file_get_relative_blockview(rd_file_type *rd_file,
                                                         const char *kw,
                                                         int occurence) {
    rd_file_view_type *view =
        rd_file_view_add_blockview(rd_file->active_view, kw, occurence);
    return view;
}

bool rd_file_subselect_block(rd_file_type *rd_file, const char *kw,
                             int occurence) {
    rd_file_view_type *blockmap =
        rd_file_get_relative_blockview(rd_file, kw, occurence);
    if (blockmap != NULL) {
        rd_file->active_view = blockmap;
        return true;
    } else
        return false;
}

static bool rd_file_index_valid0(const char *file_name,
                                 const char *index_file_name) {
    if (!util_file_exists(file_name))
        return false;

    if (!util_file_exists(index_file_name))
        return false;

    if (util_file_difftime(file_name, index_file_name) > 0)
        return false;

    return true;
}

static bool rd_file_index_valid1(const char *file_name, FILE *stream) {
    bool name_equal;
    char *source_file = util_fread_alloc_string(stream);
    char *input_name = util_split_alloc_filename(file_name);

    name_equal = util_string_equal(source_file, input_name);

    free(source_file);
    free(input_name);
    return name_equal;
}

bool rd_file_index_valid(const char *file_name, const char *index_file_name) {
    if (!rd_file_index_valid0(file_name, index_file_name))
        return false;

    bool valid = false;
    FILE *stream = fopen(index_file_name, "rb");
    if (stream) {
        valid = rd_file_index_valid1(file_name, stream);
        fclose(stream);
    }

    return valid;
}

bool rd_file_write_index(const rd_file_type *rd_file,
                         const char *index_filename) {
    FILE *ostream = fopen(index_filename, "wb");
    if (!ostream)
        return false;
    {
        char *filename =
            util_split_alloc_filename(fortio_filename_ref(rd_file->fortio));
        util_fwrite_string(filename, ostream);
        free(filename);
    }
    rd_file_view_write_index(rd_file->global_view, ostream);
    fclose(ostream);
    return true;
}

rd_file_type *rd_file_fast_open(const char *file_name,
                                const char *index_file_name, int flags) {
    if (!rd_file_index_valid0(file_name, index_file_name))
        return NULL;

    FILE *istream = fopen(index_file_name, "rb");
    if (!istream)
        return NULL;

    rd_file_type *rd_file = NULL;

    if (rd_file_index_valid1(file_name, istream)) {
        fortio_type *fortio = rd_file_alloc_fortio(file_name, flags);
        if (fortio) {
            rd_file = rd_file_alloc_empty(flags);
            rd_file->fortio = fortio;
            rd_file->global_view = rd_file_view_fread_alloc(
                rd_file->fortio, &rd_file->flags, rd_file->inv_view, istream);
            if (rd_file->global_view) {
                rd_file_select_global(rd_file);
                if (rd_file_view_check_flags(rd_file->flags,
                                             RD_FILE_CLOSE_STREAM))
                    fortio_fclose_stream(rd_file->fortio);
            } else {
                rd_file_close(rd_file);
                rd_file = NULL;
            }
        }
    }
    fclose(istream);
    return rd_file;
}
