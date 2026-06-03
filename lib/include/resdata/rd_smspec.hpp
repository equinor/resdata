#pragma once

#include <ctime>

#include <memory>
#include <vector>
#include <string>

#include <ert/util/float_vector.hpp>
#include <ert/util/stringlist.hpp>

#include <resdata/rd_util.hpp>
#include <resdata/smspec_node.hpp>

typedef struct rd_smspec_struct rd_smspec_type;

const rd::smspec_node &
rd_smspec_iget_node_w_node_index(const rd_smspec_type *smspec, int node_index);
const rd::smspec_node &
rd_smspec_iget_node_w_params_index(const rd_smspec_type *smspec,
                                   int params_index);
/**
   These are the different variable types, see table 3.4 in the
   ECLIPSE file format docuemntation for naming conventions.

   Only the variable types marked with "X" below are supported in the
   remaining implementation. To add support for a new variable type
   the functions smspec_node_alloc(), rd_smsepec_fread_header() and
   rd_smspec_install_gen_key() must be updated.
*/

rd_smspec_var_type rd_smspec_identify_var_type(const char *var);
rd_smspec_type *rd_smspec_alloc_restart_writer(
    const char *key_join_string, const char *restart_case, int restart_step,
    time_t sim_start, bool time_in_days, int nx, int ny, int nz);

rd_smspec_type *rd_smspec_alloc_writer(const char *key_join_string,
                                       time_t sim_start, bool time_in_days,
                                       int nx, int ny, int nz);
rd_smspec_type *rd_smspec_fread_alloc(const std::string &header_file,
                                      const std::string &key_join_string,
                                      bool include_restart);
void rd_smspec_free(rd_smspec_type *);

int rd_smspec_num_nodes(const rd_smspec_type *smspec);
bool rd_smspec_equal(const rd_smspec_type *self, const rd_smspec_type *other);
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
