#ifndef ERT_RD_UTIL_H
#define ERT_RD_UTIL_H
#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>
#include <time.h>

#include <ert/util/stringlist.hpp>
#include <ert/util/time_t_vector.hpp>
#include <ert/util/parser.hpp>
#include <resdata/rd_type.hpp>

typedef enum {
    RD_OTHER_FILE = 0,
    RD_RESTART_FILE = 1,
    RD_UNIFIED_RESTART_FILE = 2,
    RD_SUMMARY_FILE = 4,
    RD_UNIFIED_SUMMARY_FILE = 8,
    RD_SUMMARY_HEADER_FILE = 16,
    RD_GRID_FILE = 32,
    RD_EGRID_FILE = 64,
    RD_INIT_FILE = 128,
    RD_RFT_FILE = 256,
    RD_DATA_FILE = 512
} rd_file_enum;

/*
    This enum enumerates the four different ways summary and restart information
    can be stored.
  */

typedef enum {
    RD_INVALID_STORAGE = 0,
    RD_BINARY_UNIFIED = 1,
    RD_FORMATTED_UNIFIED = 2,
    RD_BINARY_NON_UNIFIED = 4,
    RD_FORMATTED_NON_UNIFIED = 8
} rd_storage_enum;

/*
  The resdata library has been built and tested 99.5% with ECLIPSE100
  as context, but in thye gravity code there is some very limited
  functionality related to ECLIPSE100 versus ECLIPSE300 functionality.

  Observe that numerical values found as part of the INTEHAD keyword
  differ from these values, and are found in the rd_kw_magic.h
  header.
*/

typedef enum {
    ECLIPSE_UNDEFINED = 0,
    ECLIPSE100 = 1,
    ECLIPSE300 = 2,
    ECLIPSE300_THERMAL = 3,
    INTERSECT = 4,
    FRONTSIM = 5
} rd_version_enum;

/*
  Observe that the numerical enum VALUES matches those found in item
  14 in the INTEHEAD keyword in the INIT files; i.e. the
  distribution of numerical values 1,2,4 can NOT BE CHANGED.

  The function rd_get_phase_name() can be used to lookup a
  string name from an enum value.

  The phases in a simulation will typically be a sum of these
  fundamental phases, and represented as an integer.
*/

typedef enum {
    RD_OIL_PHASE = 1,
    RD_GAS_PHASE = 2,
    RD_WATER_PHASE = 4
} rd_phase_enum;

#define RD_PHASE_ENUM_DEFS                                                     \
    {.value = 1, .name = "RD_OIL_PHASE"},                                      \
        {.value = 2, .name = "RD_GAS_PHASE"}, {                                \
        .value = 4, .name = "RD_WATER_PHASE"                                   \
    }
#define RD_PHASE_ENUM_SIZE 3

typedef enum {
    RD_METRIC_UNITS = 1,
    RD_FIELD_UNITS = 2,
    RD_LAB_UNITS = 3,
    RD_PVT_M_UNITS = 4
} ert_rd_unit_enum;

// For unformatted files:
#define RD_BOOL_TRUE_INT                                                       \
    -1 // Binary representation: 11111111  11111111  11111111  1111111
#define RD_BOOL_FALSE_INT                                                      \
    0 // Binary representation: 00000000  00000000  00000000  0000000
#define RD_COMMENT_STRING "--"
#define RD_COMMENT_CHAR '-' // Need to consecutive to make an ECLIPSE comment
#define RD_DATA_TERMINATION "/"

bool rd_unified_file(const char *filename);
const char *rd_file_type_name(rd_file_enum file_type);
char *rd_alloc_base_guess(const char *);
int rd_filename_report_nr(const char *);
rd_file_enum rd_inspect_extension(const char *ext, bool *_fmt_file,
                                  int *_report_nr);
rd_file_enum rd_get_file_type(const char *filename, bool *fmt_file,
                              int *report_nr);
char *rd_alloc_filename(const char * /* path */, const char * /* base */,
                        rd_file_enum, bool /* fmt_file */, int /*report_nr*/);
char *rd_alloc_exfilename(const char * /* path */, const char * /* base */,
                          rd_file_enum, bool /* fmt_file */, int /*report_nr*/);
void rd_memcpy_typed_data(void *, const void *, rd_data_type, rd_data_type,
                          int);
void rd_escape_kw(char *kw);
bool rd_alloc_summary_files(const char *, const char *, const char *, char **,
                            stringlist_type *);
void rd_alloc_summary_data_files(const char *path, const char *base,
                                 bool fmt_file, stringlist_type *filelist);
void rd_alloc_restart_files(const char *, const char *, char ***, int *, bool *,
                            bool *);
time_t rd_get_start_date(const char *);
int rd_get_num_cpu(const char *data_file);
bool rd_fmt_file(const char *filename, bool *__fmt_file);
char *rd_alloc_exfilename_anyfmt(const char *path, const char *base,
                                 rd_file_enum file_type, bool start_fmt,
                                 int report_nr);
int rd_get_month_nr(const char *month_name);
int rd_fname_report_cmp(const void *f1, const void *f2);
time_t rd_make_date(int mday, int month, int year);
time_t rd_make_date__(int mday, int month, int year, int *year_offset);
time_t rd_make_datetime(int sec, int min, int hour, int mday, int month,
                        int year);
ert_rd_unit_enum rd_get_unit_set(const char *data_file);

bool rd_valid_basename_fmt(const char *basename_fmt);
bool rd_valid_basename(const char *basename);
const char *rd_get_phase_name(rd_phase_enum phase);

int rd_select_filelist(const char *path, const char *base,
                       rd_file_enum file_type, bool fmt_file,
                       stringlist_type *filelist);
void rd_append_month_range(time_t_vector_type *date_list, time_t start_date,
                           time_t end_date, bool force_append_end);
void rd_init_month_range(time_t_vector_type *date_list, time_t start_date,
                         time_t end_date);
void rd_set_date_values(time_t t, int *mday, int *month, int *year);
void rd_set_datetime_values(time_t t, int *sec, int *min, int *hour, int *mday,
                            int *month, int *year);
bool rd_path_access(const char *rd_case);
#ifdef __cplusplus
}
#endif
#endif
