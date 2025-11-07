#pragma once

#include <ert/util/stringlist.hpp>

#include <resdata/rd_rft_node.hpp>
#include <ert/util/vector.hpp>
#include <ert/util/int_vector.hpp>

typedef struct rd_rft_file_struct rd_rft_file_type;

extern "C" rd_rft_file_type *rd_rft_file_alloc_case(const char *case_input);
rd_rft_file_type *rd_rft_file_alloc(const char *);
extern "C" void rd_rft_file_free(rd_rft_file_type *);
extern "C" int rd_rft_file_get_size__(const rd_rft_file_type *rft_file,
                                      const char *well_pattern,
                                      time_t recording_time);
int rd_rft_file_get_size(const rd_rft_file_type *rft_file);
extern "C" rd_rft_node_type *
rd_rft_file_get_well_time_rft(const rd_rft_file_type *rft_file,
                              const char *well, time_t recording_time);
extern "C" rd_rft_node_type *
rd_rft_file_iget_node(const rd_rft_file_type *rft_file, int index);
extern "C" int rd_rft_file_get_num_wells(const rd_rft_file_type *rft_file);
