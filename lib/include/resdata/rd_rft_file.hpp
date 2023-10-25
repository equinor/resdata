#ifndef ERT_RD_RFT_FILE_H
#define ERT_RD_RFT_FILE_H
#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>

#include <ert/util/stringlist.hpp>

#include <resdata/rd_rft_node.hpp>
#include <ert/util/vector.hpp>
#include <ert/util/int_vector.hpp>

typedef struct rd_rft_file_struct rd_rft_file_type;

char *rd_rft_file_alloc_case_filename(const char *case_input);
const char *rd_rft_file_get_filename(const rd_rft_file_type *rft_file);
rd_rft_file_type *rd_rft_file_alloc_case(const char *case_input);
bool rd_rft_file_case_has_rft(const char *case_input);
rd_rft_file_type *rd_rft_file_alloc(const char *);
void rd_rft_file_free(rd_rft_file_type *);
void rd_rft_file_block(const rd_rft_file_type *, double, const char *, int,
                       const double *, int *, int *, int *);
void rd_rft_file_fprintf_rft_obs(const rd_rft_file_type *, double, const char *,
                                 const char *, const char *, double);
rd_rft_node_type *rd_rft_file_get_node(const rd_rft_file_type *, const char *);

int rd_rft_file_get_size__(const rd_rft_file_type *rft_file,
                           const char *well_pattern, time_t recording_time);
int rd_rft_file_get_size(const rd_rft_file_type *rft_file);
rd_rft_node_type *
rd_rft_file_get_well_time_rft(const rd_rft_file_type *rft_file,
                              const char *well, time_t recording_time);
rd_rft_node_type *rd_rft_file_iget_node(const rd_rft_file_type *rft_file,
                                        int index);
rd_rft_node_type *rd_rft_file_iget_well_rft(const rd_rft_file_type *rft_file,
                                            const char *well, int index);
bool rd_rft_file_has_well(const rd_rft_file_type *rft_file, const char *well);
int rd_rft_file_get_well_occurences(const rd_rft_file_type *rft_file,
                                    const char *well);
stringlist_type *rd_rft_file_alloc_well_list(const rd_rft_file_type *rft_file);
int rd_rft_file_get_num_wells(const rd_rft_file_type *rft_file);
void rd_rft_file_free__(void *arg);
void rd_rft_file_update(const char *rft_file_name, rd_rft_node_type **nodes,
                        int num_nodes, ert_rd_unit_enum unit_set);

#ifdef __cplusplus
}
#endif
#endif
