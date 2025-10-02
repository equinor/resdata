#ifndef ERT_RD_RFT_NODE_H
#define ERT_RD_RFT_NODE_H
#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>

#include <resdata/rd_file_view.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/rd_rft_cell.hpp>

typedef enum {
    RFT = 1,
    PLT = 2,
    SEGMENT = 3 /* Not really implemented */
} rd_rft_enum;

typedef struct rd_rft_node_struct rd_rft_node_type;

const rd_rft_cell_type *rd_rft_node_iget_cell_sorted(rd_rft_node_type *rft_node,
                                                     int index);
const rd_rft_cell_type *rd_rft_node_iget_cell(const rd_rft_node_type *rft_node,
                                              int index);
const rd_rft_cell_type *rd_rft_node_lookup_ijk(const rd_rft_node_type *rft_node,
                                               int i, int j, int k);
rd_rft_node_type *rd_rft_node_alloc(const rd_file_view_type *rft_view);
void rd_rft_node_free(rd_rft_node_type *);
time_t rd_rft_node_get_date(const rd_rft_node_type *);
int rd_rft_node_get_size(const rd_rft_node_type *);
const char *rd_rft_node_get_well_name(const rd_rft_node_type *rft_node);
void rd_rft_node_iget_ijk(const rd_rft_node_type *rft_node, int index, int *i,
                          int *j, int *k);

bool rd_rft_node_is_RFT(const rd_rft_node_type *rft_node);
bool rd_rft_node_is_PLT(const rd_rft_node_type *rft_node);
bool rd_rft_node_is_SEGMENT(const rd_rft_node_type *rft_node);
bool rd_rft_node_is_MSW(const rd_rft_node_type *rft_node);

double rd_rft_node_iget_pressure(const rd_rft_node_type *rft_node, int index);
double rd_rft_node_iget_depth(const rd_rft_node_type *rft_node, int index);
double rd_rft_node_iget_wrat(const rd_rft_node_type *rft_node, int index);
double rd_rft_node_iget_grat(const rd_rft_node_type *rft_node, int index);
double rd_rft_node_iget_orat(const rd_rft_node_type *rft_node, int index);

double rd_rft_node_iget_swat(const rd_rft_node_type *rft_node, int index);
double rd_rft_node_iget_sgas(const rd_rft_node_type *rft_node, int index);
double rd_rft_node_iget_soil(const rd_rft_node_type *rft_node, int index);
double rd_rft_node_get_days(const rd_rft_node_type *rft_node);
bool rd_rft_node_lt(const rd_rft_node_type *n1, const rd_rft_node_type *n2);

rd_rft_node_type *rd_rft_node_alloc_new(const char *well_name,
                                        const char *data_type_string,
                                        const time_t recording_date,
                                        const double days);

rd_rft_enum rd_rft_node_get_type(const rd_rft_node_type *rft_node);

#ifdef __cplusplus
}
#endif
#endif
