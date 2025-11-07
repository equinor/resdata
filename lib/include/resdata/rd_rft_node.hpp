#pragma once

#include <resdata/rd_file_view.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/rd_rft_cell.hpp>

typedef enum {
    RFT = 1,
    PLT = 2,
    SEGMENT = 3 /* Not really implemented */
} rd_rft_enum;

typedef struct rd_rft_node_struct rd_rft_node_type;

extern "C" const rd_rft_cell_type *
rd_rft_node_iget_cell(const rd_rft_node_type *rft_node, int index);
extern "C" const rd_rft_cell_type *
rd_rft_node_lookup_ijk(const rd_rft_node_type *rft_node, int i, int j, int k);
extern "C" rd_rft_node_type *
rd_rft_node_alloc(const rd_file_view_type *rft_view);
extern "C" void rd_rft_node_free(rd_rft_node_type *);
extern "C" time_t rd_rft_node_get_date(const rd_rft_node_type *);
extern "C" int rd_rft_node_get_size(const rd_rft_node_type *);
extern "C" const char *
rd_rft_node_get_well_name(const rd_rft_node_type *rft_node);
extern "C" void rd_rft_node_iget_ijk(const rd_rft_node_type *rft_node,
                                     int index, int *i, int *j, int *k);
extern "C" bool rd_rft_node_is_RFT(const rd_rft_node_type *rft_node);
extern "C" bool rd_rft_node_is_PLT(const rd_rft_node_type *rft_node);
extern "C" bool rd_rft_node_is_SEGMENT(const rd_rft_node_type *rft_node);
extern "C" bool rd_rft_node_is_MSW(const rd_rft_node_type *rft_node);
extern "C" double rd_rft_node_iget_pressure(const rd_rft_node_type *rft_node,
                                            int index);
extern "C" double rd_rft_node_iget_depth(const rd_rft_node_type *rft_node,
                                         int index);
extern "C" double rd_rft_node_iget_wrat(const rd_rft_node_type *rft_node,
                                        int index);
extern "C" double rd_rft_node_iget_grat(const rd_rft_node_type *rft_node,
                                        int index);
extern "C" double rd_rft_node_iget_orat(const rd_rft_node_type *rft_node,
                                        int index);
extern "C" double rd_rft_node_iget_swat(const rd_rft_node_type *rft_node,
                                        int index);
extern "C" double rd_rft_node_iget_sgas(const rd_rft_node_type *rft_node,
                                        int index);
extern "C" double rd_rft_node_iget_soil(const rd_rft_node_type *rft_node,
                                        int index);
double rd_rft_node_get_days(const rd_rft_node_type *rft_node);

extern "C" rd_rft_node_type *rd_rft_node_alloc_new(const char *well_name,
                                                   const char *data_type_string,
                                                   const time_t recording_date,
                                                   const double days);
extern "C" rd_rft_enum rd_rft_node_get_type(const rd_rft_node_type *rft_node);
