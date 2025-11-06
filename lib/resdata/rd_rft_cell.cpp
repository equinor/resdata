#include <cmath>
#include <cstdlib>
#include <variant>

#include <ert/util/util.hpp>
#include <ert/util/type_macros.hpp>

#include <resdata/rd_kw.hpp>
#include <resdata/rd_kw_magic.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/rd_rft_cell.hpp>

struct rft_data_struct {
    double swat;
    double sgas;
};

struct plt_data_struct {
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

struct rd_rft_cell_struct {
    int i, j, k;
    double pressure;
    double depth;

    std::variant<rft_data_struct, plt_data_struct> data;
};

static rd_rft_cell_type *
rd_rft_cell_alloc_common(int i, int j, int k, double depth, double pressure) {
    auto cell = new rd_rft_cell_type;

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

    cell->data = rft_data_struct{.swat = swat, .sgas = sgas};
    return cell;
}

rd_rft_cell_type *
rd_rft_cell_alloc_PLT(int i, int j, int k, double depth, double pressure,
                      double orat, double grat, double wrat,
                      double connection_start, double connection_end,
                      double flowrate, double oil_flowrate, double gas_flowrate,
                      double water_flowrate) {

    rd_rft_cell_type *cell = rd_rft_cell_alloc_common(i, j, k, depth, pressure);

    cell->data = plt_data_struct{
        .orat = orat,
        .wrat = wrat,
        .grat = grat,
        .connection_start = connection_start,
        .connection_end = connection_end,
        .flowrate = flowrate,
        .oil_flowrate = oil_flowrate,
        .gas_flowrate = gas_flowrate,
        .water_flowrate = water_flowrate,
    };
    return cell;
}

void rd_rft_cell_free(rd_rft_cell_type *cell) { delete cell; }

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
    if (std::holds_alternative<rft_data_struct>(cell->data))
        return std::get<rft_data_struct>(cell->data).swat;
    else
        return RD_RFT_CELL_INVALID_VALUE;
}

double rd_rft_cell_get_sgas(const rd_rft_cell_type *cell) {
    if (std::holds_alternative<rft_data_struct>(cell->data))
        return std::get<rft_data_struct>(cell->data).sgas;
    else
        return RD_RFT_CELL_INVALID_VALUE;
}

double rd_rft_cell_get_soil(const rd_rft_cell_type *cell) {
    if (std::holds_alternative<rft_data_struct>(cell->data)) {
        auto d = std::get<rft_data_struct>(cell->data);
        return 1 - (d.swat + d.sgas);
    } else
        return RD_RFT_CELL_INVALID_VALUE;
}

double rd_rft_cell_get_orat(const rd_rft_cell_type *cell) {
    if (std::holds_alternative<plt_data_struct>(cell->data))
        return std::get<plt_data_struct>(cell->data).orat;
    else
        return RD_RFT_CELL_INVALID_VALUE;
}

double rd_rft_cell_get_grat(const rd_rft_cell_type *cell) {
    if (std::holds_alternative<plt_data_struct>(cell->data))
        return std::get<plt_data_struct>(cell->data).grat;
    else
        return RD_RFT_CELL_INVALID_VALUE;
}

double rd_rft_cell_get_wrat(const rd_rft_cell_type *cell) {
    if (std::holds_alternative<plt_data_struct>(cell->data))
        return std::get<plt_data_struct>(cell->data).wrat;
    else
        return RD_RFT_CELL_INVALID_VALUE;
}

double rd_rft_cell_get_connection_start(const rd_rft_cell_type *cell) {
    if (std::holds_alternative<plt_data_struct>(cell->data))
        return std::get<plt_data_struct>(cell->data).connection_start;
    else
        return RD_RFT_CELL_INVALID_VALUE;
}

double rd_rft_cell_get_connection_end(const rd_rft_cell_type *cell) {
    if (std::holds_alternative<plt_data_struct>(cell->data))
        return std::get<plt_data_struct>(cell->data).connection_end;
    else
        return RD_RFT_CELL_INVALID_VALUE;
}

double rd_rft_cell_get_flowrate(const rd_rft_cell_type *cell) {
    if (std::holds_alternative<plt_data_struct>(cell->data))
        return std::get<plt_data_struct>(cell->data).flowrate;
    else
        return RD_RFT_CELL_INVALID_VALUE;
}

double rd_rft_cell_get_oil_flowrate(const rd_rft_cell_type *cell) {
    if (std::holds_alternative<plt_data_struct>(cell->data))
        return std::get<plt_data_struct>(cell->data).oil_flowrate;
    else
        return RD_RFT_CELL_INVALID_VALUE;
}

double rd_rft_cell_get_gas_flowrate(const rd_rft_cell_type *cell) {
    if (std::holds_alternative<plt_data_struct>(cell->data))
        return std::get<plt_data_struct>(cell->data).gas_flowrate;
    else
        return RD_RFT_CELL_INVALID_VALUE;
}

double rd_rft_cell_get_water_flowrate(const rd_rft_cell_type *cell) {
    if (std::holds_alternative<plt_data_struct>(cell->data))
        return std::get<plt_data_struct>(cell->data).water_flowrate;
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
