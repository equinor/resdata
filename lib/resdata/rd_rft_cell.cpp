#include <math.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#include <ert/util/util.hpp>
#include <ert/util/type_macros.hpp>

#include <resdata/rd_kw.hpp>
#include <resdata/rd_kw_magic.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/rd_rft_cell.hpp>

#define RD_RFT_CELL_TYPE_ID 99164012
#define RFT_DATA_TYPE_ID 66787166
#define PLT_DATA_TYPE_ID 87166667

struct rd_rft_cell_struct {
    UTIL_TYPE_ID_DECLARATION;
    int i, j, k;
    double pressure;
    double depth;

    void *data;
};

typedef struct plt_data_struct plt_data_type;
typedef struct rft_data_struct rft_data_type;

struct rft_data_struct {
    UTIL_TYPE_ID_DECLARATION;
    double swat;
    double sgas;
};

struct plt_data_struct {
    UTIL_TYPE_ID_DECLARATION;
    double orat;
    double wrat;
    double grat;
    double connection_start;
    double connection_end;
    double flowrate;
    double oil_flowrate;
    double gas_flowrate;
    double water_flowrate;
};

static rft_data_type *rft_data_alloc(double swat, double sgas) {
    rft_data_type *data = (rft_data_type *)util_malloc(sizeof *data);
    UTIL_TYPE_ID_INIT(data, RFT_DATA_TYPE_ID);

    data->swat = swat;
    data->sgas = sgas;

    return data;
}

static void rft_data_free(rft_data_type *data) { free(data); }

static UTIL_TRY_CAST_FUNCTION_CONST(
    rft_data,
    RFT_DATA_TYPE_ID) static UTIL_IS_INSTANCE_FUNCTION(rft_data,
                                                       RFT_DATA_TYPE_ID)

    static plt_data_type *plt_data_alloc(double orat, double grat, double wrat,
                                         double connection_start,
                                         double connection_end, double flowrate,
                                         double oil_flowrate,
                                         double gas_flowrate,
                                         double water_flowrate) {
    plt_data_type *data = (plt_data_type *)util_malloc(sizeof *data);
    UTIL_TYPE_ID_INIT(data, PLT_DATA_TYPE_ID);

    data->orat = orat;
    data->grat = grat;
    data->wrat = wrat;
    data->connection_start = connection_start;
    data->connection_end = connection_end;
    data->flowrate = flowrate;
    data->oil_flowrate = oil_flowrate;
    data->gas_flowrate = gas_flowrate;
    data->water_flowrate = water_flowrate;

    return data;
}

static void plt_data_free(plt_data_type *data) { free(data); }

static UTIL_TRY_CAST_FUNCTION_CONST(
    plt_data,
    PLT_DATA_TYPE_ID) static UTIL_IS_INSTANCE_FUNCTION(plt_data,
                                                       PLT_DATA_TYPE_ID)

    static UTIL_SAFE_CAST_FUNCTION(
        rd_rft_cell,
        RD_RFT_CELL_TYPE_ID) static UTIL_SAFE_CAST_FUNCTION_CONST(rd_rft_cell,
                                                                  RD_RFT_CELL_TYPE_ID)
        UTIL_IS_INSTANCE_FUNCTION(rd_rft_cell, RD_RFT_CELL_TYPE_ID)

            static rd_rft_cell_type *rd_rft_cell_alloc_common(int i, int j,
                                                              int k,
                                                              double depth,
                                                              double pressure) {
    rd_rft_cell_type *cell = (rd_rft_cell_type *)util_malloc(sizeof *cell);
    UTIL_TYPE_ID_INIT(cell, RD_RFT_CELL_TYPE_ID);

    cell->i = i;
    cell->j = j;
    cell->k = k;
    cell->depth = depth;
    cell->pressure = pressure;

    return cell;
}

rd_rft_cell_type *rd_rft_cell_alloc_RFT(int i, int j, int k, double depth,
                                        double pressure, double swat,
                                        double sgas) {
    rd_rft_cell_type *cell = rd_rft_cell_alloc_common(i, j, k, depth, pressure);

    cell->data = rft_data_alloc(swat, sgas);
    return cell;
}

rd_rft_cell_type *
rd_rft_cell_alloc_PLT(int i, int j, int k, double depth, double pressure,
                      double orat, double grat, double wrat,
                      double connection_start, double connection_end,
                      double flowrate, double oil_flowrate, double gas_flowrate,
                      double water_flowrate) {

    rd_rft_cell_type *cell = rd_rft_cell_alloc_common(i, j, k, depth, pressure);

    cell->data =
        plt_data_alloc(orat, grat, wrat, connection_start, connection_end,
                       flowrate, oil_flowrate, gas_flowrate, water_flowrate);
    return cell;
}

void rd_rft_cell_free(rd_rft_cell_type *cell) {
    if (rft_data_is_instance(cell->data))
        rft_data_free((rft_data_type *)cell->data);
    else if (plt_data_is_instance(cell->data))
        plt_data_free((plt_data_type *)cell->data);

    free(cell);
}

int rd_rft_cell_get_i(const rd_rft_cell_type *cell) { return cell->i; }

int rd_rft_cell_get_j(const rd_rft_cell_type *cell) { return cell->j; }

int rd_rft_cell_get_k(const rd_rft_cell_type *cell) { return cell->k; }

void rd_rft_cell_get_ijk(const rd_rft_cell_type *cell, int *i, int *j, int *k) {
    *i = cell->i;
    *j = cell->j;
    *k = cell->k;
}

double rd_rft_cell_get_depth(const rd_rft_cell_type *cell) {
    return cell->depth;
}

double rd_rft_cell_get_pressure(const rd_rft_cell_type *cell) {
    return cell->pressure;
}

double rd_rft_cell_get_swat(const rd_rft_cell_type *cell) {
    const rft_data_type *data = rft_data_try_cast_const(cell->data);
    if (data)
        return data->swat;
    else
        return RD_RFT_CELL_INVALID_VALUE;
}

double rd_rft_cell_get_sgas(const rd_rft_cell_type *cell) {
    const rft_data_type *data = rft_data_try_cast_const(cell->data);
    if (data)
        return data->sgas;
    else
        return RD_RFT_CELL_INVALID_VALUE;
}

double rd_rft_cell_get_soil(const rd_rft_cell_type *cell) {
    const rft_data_type *data = rft_data_try_cast_const(cell->data);
    if (data)
        return 1 - (data->swat + data->sgas);
    else
        return RD_RFT_CELL_INVALID_VALUE;
}

double rd_rft_cell_get_orat(const rd_rft_cell_type *cell) {
    const plt_data_type *data = plt_data_try_cast_const(cell->data);
    if (data)
        return data->orat;
    else
        return RD_RFT_CELL_INVALID_VALUE;
}

double rd_rft_cell_get_grat(const rd_rft_cell_type *cell) {
    const plt_data_type *data = plt_data_try_cast_const(cell->data);
    if (data)
        return data->grat;
    else
        return RD_RFT_CELL_INVALID_VALUE;
}

double rd_rft_cell_get_wrat(const rd_rft_cell_type *cell) {
    const plt_data_type *data = plt_data_try_cast_const(cell->data);
    if (data)
        return data->wrat;
    else
        return RD_RFT_CELL_INVALID_VALUE;
}

double rd_rft_cell_get_connection_start(const rd_rft_cell_type *cell) {
    const plt_data_type *data = plt_data_try_cast_const(cell->data);
    if (data)
        return data->connection_start;
    else
        return RD_RFT_CELL_INVALID_VALUE;
}

double rd_rft_cell_get_connection_end(const rd_rft_cell_type *cell) {
    const plt_data_type *data = plt_data_try_cast_const(cell->data);
    if (data)
        return data->connection_end;
    else
        return RD_RFT_CELL_INVALID_VALUE;
}

double rd_rft_cell_get_flowrate(const rd_rft_cell_type *cell) {
    const plt_data_type *data = plt_data_try_cast_const(cell->data);
    if (data)
        return data->flowrate;
    else
        return RD_RFT_CELL_INVALID_VALUE;
}

double rd_rft_cell_get_oil_flowrate(const rd_rft_cell_type *cell) {
    const plt_data_type *data = plt_data_try_cast_const(cell->data);
    if (data)
        return data->oil_flowrate;
    else
        return RD_RFT_CELL_INVALID_VALUE;
}

double rd_rft_cell_get_gas_flowrate(const rd_rft_cell_type *cell) {
    const plt_data_type *data = plt_data_try_cast_const(cell->data);
    if (data)
        return data->gas_flowrate;
    else
        return RD_RFT_CELL_INVALID_VALUE;
}

double rd_rft_cell_get_water_flowrate(const rd_rft_cell_type *cell) {
    const plt_data_type *data = plt_data_try_cast_const(cell->data);
    if (data)
        return data->water_flowrate;
    else
        return RD_RFT_CELL_INVALID_VALUE;
}

bool rd_rft_cell_ijk_equal(const rd_rft_cell_type *cell, int i, int j, int k) {
    return ((i == cell->i) && (j == cell->j) && (k == cell->k));
}

/*
  Currently only comparison based on connection length along PLT is supported.
*/
static int rd_rft_cell_cmp(const rd_rft_cell_type *cell1,
                           const rd_rft_cell_type *cell2) {
    double val1 = rd_rft_cell_get_connection_start(cell1);
    double val2 = rd_rft_cell_get_connection_start(cell2);

    if (val1 < val2)
        return -1;
    else if (val1 == val2)
        return 0;
    else
        return 1;
}

bool rd_rft_cell_lt(const rd_rft_cell_type *cell1,
                    const rd_rft_cell_type *cell2) {
    return (rd_rft_cell_cmp(cell1, cell2) < 0);
}
