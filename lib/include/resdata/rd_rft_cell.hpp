#ifndef ERT_RD_RFT_CELL_H
#define ERT_RD_RFT_CELL_H
#ifdef __cplusplus
extern "C" {
#endif

#include <ert/util/type_macros.hpp>

#define RD_RFT_CELL_INVALID_VALUE -1

typedef struct rd_rft_cell_struct rd_rft_cell_type;

UTIL_IS_INSTANCE_HEADER(rd_rft_cell);

rd_rft_cell_type *
rd_rft_cell_alloc_PLT(int i, int j, int k, double depth, double pressure,
                      double orat, double grat, double wrat,
                      double connection_start, double connection_end,
                      double flowrate, double oil_flowrate, double gas_flowrate,
                      double water_flowrate);

rd_rft_cell_type *rd_rft_cell_alloc_RFT(int i, int j, int k, double depth,
                                        double pressure, double swat,
                                        double sgas);
void rd_rft_cell_free(rd_rft_cell_type *cell);
void rd_rft_cell_free__(void *arg);

bool rd_rft_cell_ijk_equal(const rd_rft_cell_type *cell, int i, int j, int k);
void rd_rft_cell_get_ijk(const rd_rft_cell_type *cell, int *i, int *j, int *k);
int rd_rft_cell_get_i(const rd_rft_cell_type *cell);
int rd_rft_cell_get_j(const rd_rft_cell_type *cell);
int rd_rft_cell_get_k(const rd_rft_cell_type *cell);
double rd_rft_cell_get_depth(const rd_rft_cell_type *cell);
double rd_rft_cell_get_pressure(const rd_rft_cell_type *cell);

double rd_rft_cell_get_swat(const rd_rft_cell_type *cell);
double rd_rft_cell_get_sgas(const rd_rft_cell_type *cell);
double rd_rft_cell_get_soil(const rd_rft_cell_type *cell);

double rd_rft_cell_get_wrat(const rd_rft_cell_type *cell);
double rd_rft_cell_get_grat(const rd_rft_cell_type *cell);
double rd_rft_cell_get_orat(const rd_rft_cell_type *cell);
double rd_rft_cell_get_connection_start(const rd_rft_cell_type *cell);
double rd_rft_cell_get_connection_end(const rd_rft_cell_type *cell);
double rd_rft_cell_get_flowrate(const rd_rft_cell_type *cell);
double rd_rft_cell_get_oil_flowrate(const rd_rft_cell_type *cell);
double rd_rft_cell_get_gas_flowrate(const rd_rft_cell_type *cell);
double rd_rft_cell_get_water_flowrate(const rd_rft_cell_type *cell);

int rd_rft_cell_cmp__(const void *arg1, const void *arg2);
int rd_rft_cell_cmp(const rd_rft_cell_type *cell1,
                    const rd_rft_cell_type *cell2);
bool rd_rft_cell_lt(const rd_rft_cell_type *cell1,
                    const rd_rft_cell_type *cell2);
#ifdef __cplusplus
}
#endif

#endif
