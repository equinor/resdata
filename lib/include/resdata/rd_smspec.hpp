#ifndef ERT_RD_SMSPEC
#define ERT_RD_SMSPEC

#include <time.h>
#include <stdbool.h>

#include <ert/util/float_vector.hpp>
#include <ert/util/stringlist.hpp>

#include <resdata/rd_util.hpp>
#include <resdata/smspec_node.hpp>

typedef struct rd_smspec_struct rd_smspec_type;

#ifdef __cplusplus
#include <vector>
const std::vector<float> &
rd_smspec_get_params_default(const rd_smspec_type *rd_smspec);
const rd::smspec_node &
rd_smspec_get_well_completion_var_node(const rd_smspec_type *rd_smspec,
                                       const char *well, const char *var,
                                       int cell_nr);
const rd::smspec_node &
rd_smspec_get_general_var_node(const rd_smspec_type *smspec,
                               const char *lookup_kw);
const rd::smspec_node &
rd_smspec_iget_node_w_node_index(const rd_smspec_type *smspec, int node_index);
const rd::smspec_node &
rd_smspec_iget_node_w_params_index(const rd_smspec_type *smspec,
                                   int params_index);
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
   These are the different variable types, see table 3.4 in the
   ECLIPSE file format docuemntation for naming conventions.

   Only the variable types marked with "X" below are supported in the
   remaining implementation. To add support for a new variable type
   the functions smspec_node_alloc(), rd_smsepec_fread_header() and
   rd_smspec_install_gen_key() must be updated.
*/

int *rd_smspec_alloc_mapping(const rd_smspec_type *self,
                             const rd_smspec_type *other);
const int *rd_smspec_get_index_map(const rd_smspec_type *smspec);
rd_smspec_var_type rd_smspec_identify_var_type(const char *var);
rd_smspec_type *rd_smspec_alloc_restart_writer(
    const char *key_join_string, const char *restart_case, int restart_step,
    time_t sim_start, bool time_in_days, int nx, int ny, int nz);

rd_smspec_type *rd_smspec_alloc_writer(const char *key_join_string,
                                       time_t sim_start, bool time_in_days,
                                       int nx, int ny, int nz);
void rd_smspec_fwrite(const rd_smspec_type *smspec, const char *rd_case,
                      bool fmt_file);

rd_smspec_type *rd_smspec_fread_alloc(const char *header_file,
                                      const char *key_join_string,
                                      bool include_restart);
void rd_smspec_free(rd_smspec_type *);

int rd_smspec_get_date_day_index(const rd_smspec_type *smspec);
int rd_smspec_get_date_month_index(const rd_smspec_type *smspec);
int rd_smspec_get_date_year_index(const rd_smspec_type *smspec);

int rd_smspec_get_general_var_params_index(const rd_smspec_type *rd_smspec,
                                           const char *lookup_kw);
bool rd_smspec_has_general_var(const rd_smspec_type *rd_smspec,
                               const char *lookup_kw);
void rd_smspec_select_matching_general_var_list(const rd_smspec_type *smspec,
                                                const char *pattern,
                                                stringlist_type *keys);
stringlist_type *
rd_smspec_alloc_matching_general_var_list(const rd_smspec_type *smspec,
                                          const char *pattern);

int rd_smspec_get_time_seconds(const rd_smspec_type *rd_smspec);
int rd_smspec_get_time_index(const rd_smspec_type *rd_smspec);
time_t rd_smspec_get_start_time(const rd_smspec_type *);
bool rd_smspec_get_formatted(const rd_smspec_type *rd_smspec);
const char *rd_smspec_get_header_file(const rd_smspec_type *rd_smspec);
stringlist_type *rd_smspec_alloc_well_list(const rd_smspec_type *smspec,
                                           const char *pattern);
stringlist_type *rd_smspec_alloc_group_list(const rd_smspec_type *smspec,
                                            const char *pattern);
int rd_smspec_get_first_step(const rd_smspec_type *rd_smspec);
int rd_smspec_get_restart_step(const rd_smspec_type *rd_smspec);
const char *rd_smspec_get_restart_case(const rd_smspec_type *rd_smspec);
const int *rd_smspec_get_grid_dims(const rd_smspec_type *smspec);
int rd_smspec_get_params_size(const rd_smspec_type *smspec);
int rd_smspec_num_nodes(const rd_smspec_type *smspec);
bool rd_smspec_equal(const rd_smspec_type *self, const rd_smspec_type *other);
ert_rd_unit_enum rd_smspec_get_unit_system(const rd_smspec_type *smspec);

#ifdef __cplusplus
}
#endif

const rd::smspec_node *rd_smspec_add_node(rd_smspec_type *rd_smspec,
                                          const rd::smspec_node &node);
const rd::smspec_node *rd_smspec_add_node(rd_smspec_type *rd_smspec,
                                          const char *keyword, int num,
                                          const char *unit,
                                          float default_value);
const rd::smspec_node *rd_smspec_add_node(rd_smspec_type *rd_smspec,
                                          const char *keyword, const char *unit,
                                          float default_value);
const rd::smspec_node *rd_smspec_add_node(rd_smspec_type *rd_smspec,
                                          const char *keyword,
                                          const char *wgname, const char *unit,
                                          float default_value);
const rd::smspec_node *rd_smspec_add_node(rd_smspec_type *rd_smspec,
                                          const char *keyword,
                                          const char *wgname, int num,
                                          const char *unit,
                                          float default_value);
const rd::smspec_node *rd_smspec_add_node(rd_smspec_type *rd_smspec,
                                          const char *keyword, int num,
                                          const char *unit,
                                          float default_value);

const rd::smspec_node *rd_smspec_add_node(rd_smspec_type *rd_smspec,
                                          int params_index, const char *keyword,
                                          const char *wgname, int num,
                                          const char *unit,
                                          float default_value);

const rd::smspec_node *rd_smspec_add_node(rd_smspec_type *rd_smspec,
                                          int params_index, const char *keyword,
                                          const char *wgname, int num,
                                          const char *unit, const char *lgr,
                                          int lgr_i, int lgr_j, int lgr_k,
                                          float default_value);

#endif
