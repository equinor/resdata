#include <cmath>
#include <ctime>
#include <cstring>
#include <cstdlib>
#include <cstdio>

#include <vector>
#include <algorithm>
#include <string>

#include <ert/util/util.hpp>

#include <resdata/rd_kw.hpp>
#include <resdata/rd_kw_magic.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/rd_file_view.hpp>
#include <resdata/rd_rft_node.hpp>
#include <resdata/rd_rft_cell.hpp>
#include <resdata/rd_type.hpp>

/**
    The RFT's from several wells, and possibly also several timesteps
    are lumped together in one .RFT file. The rd_rft_node
    implemented in this file contains the information for one
    well/report step.
*/

/*
  If the type is RFT, PLT or SEGMENT depends on the options used when
  the .RFT file is created. RFT and PLT are quite similar, SEGMENT is
  not really supported.
*/

#define RD_RFT_NODE_ID 887195
struct rd_rft_node_struct {
    UTIL_TYPE_ID_DECLARATION;
    std::string well_name;

    rd_rft_enum data_type; /* What type of data: RFT|PLT|SEGMENT */
    time_t recording_date; /* When was the RFT recorded - date.*/
    double days; /* When was the RFT recorded - days after simulation start. */
    bool MSW;

    std::vector<rd_rft_cell_type *> cells;
};

/*
  The implementation of cell types based on _either_ RFT data or PLT
  data is based on a misunderstanding and is currently WRONG. One
  section in an RFT file can contain RFT data, PLT data and SEGMENT
  data. The @data_type string should therefore not be interpreted as a
  type string, but rather as a "bit mask":


    "R"  => Section contains only RFT data.
    "P"  => Section contains only PLT data.
    "RP" => Section contains *BOTH* RFT data and PLT data.
*/

/**
   Will return nullptr if the data_type_string is equal to "SEGMENT" -
   that is not (yet) supported.
*/
static rd_rft_enum
translate_from_sting_to_rd_rft_enum(const char *data_type_string) {
    rd_rft_enum data_type = SEGMENT;
    /* According to the ECLIPSE documentation. */
    if (strchr(data_type_string, 'P') != nullptr)
        data_type = PLT;
    else if (strchr(data_type_string, 'R') != nullptr)
        data_type = RFT;
    else if (strchr(data_type_string, 'S') != nullptr)
        data_type = SEGMENT;
    else
        util_abort(
            "%s: Could not determine type of RFT/PLT/SEGMENT data - aborting\n",
            __func__);

    return data_type;
}

rd_rft_node_type *rd_rft_node_alloc_new(const char *well_name,
                                        const char *data_type_string,
                                        const time_t recording_date,
                                        const double days) {
    rd_rft_enum data_type =
        translate_from_sting_to_rd_rft_enum(data_type_string);
    rd_rft_node_type *rft_node = new rd_rft_node_type();

    UTIL_TYPE_ID_INIT(rft_node, RD_RFT_NODE_ID);
    rft_node->well_name = std::string(well_name);
    rft_node->recording_date = recording_date;
    rft_node->days = days;
    rft_node->data_type = data_type;

    return rft_node;
}

static rd_rft_node_type *rd_rft_node_alloc_empty(const char *data_type_string) {
    rd_rft_enum data_type =
        translate_from_sting_to_rd_rft_enum(data_type_string);

    /* Can return nullptr */
    if (data_type == SEGMENT) {
        fprintf(
            stderr,
            "%s: sorry SEGMENT PLT/RFT is not supported - file a complaint. \n",
            __func__);
        return nullptr;
    }

    {
        rd_rft_node_type *rft_node = new rd_rft_node_type();

        UTIL_TYPE_ID_INIT(rft_node, RD_RFT_NODE_ID);
        rft_node->data_type = data_type;

        return rft_node;
    }
}

UTIL_SAFE_CAST_FUNCTION(rd_rft_node, RD_RFT_NODE_ID);
UTIL_IS_INSTANCE_FUNCTION(rd_rft_node, RD_RFT_NODE_ID);

static void rd_rft_node_append_cell(rd_rft_node_type *rft_node,
                                    rd_rft_cell_type *cell) {
    if (rft_node->MSW) {
        auto pos_iter =
            std::upper_bound(rft_node->cells.begin(), rft_node->cells.end(),
                             cell, rd_rft_cell_lt);
        rft_node->cells.insert(pos_iter, cell);
    } else
        rft_node->cells.push_back(cell);
}

static rd_kw_type *rd_rft_node_get_pressure_kw(rd_rft_node_type *rft_node,
                                               const rd_file_view_type *rft) {
    if (rft_node->data_type == RFT)
        return rd_file_view_iget_named_kw(rft, PRESSURE_KW, 0);
    else {
        rd_kw_type *conpres_kw = rd_file_view_iget_named_kw(rft, CONPRES_KW, 0);
        if (rd_kw_element_sum_float(conpres_kw) > 0.0)
            return conpres_kw;
        else if (rd_file_view_has_kw(rft, PRESSURE_KW))
            return rd_file_view_iget_named_kw(rft, PRESSURE_KW, 0);
        else {
            fprintf(stderr,
                    "WARNING: %s returned a CONPRES_KW with all values at "
                    "zero. PRESSURE_KW not found.\n",
                    __func__);
            return conpres_kw;
        }
    }
}

static void rd_rft_node_init_RFT_cells(rd_rft_node_type *rft_node,
                                       const rd_file_view_type *rft_view) {
    const rd_kw_type *conipos =
        rd_file_view_iget_named_kw(rft_view, CONIPOS_KW, 0);
    const rd_kw_type *conjpos =
        rd_file_view_iget_named_kw(rft_view, CONJPOS_KW, 0);
    const rd_kw_type *conkpos =
        rd_file_view_iget_named_kw(rft_view, CONKPOS_KW, 0);
    const rd_kw_type *depth_kw =
        rd_file_view_iget_named_kw(rft_view, DEPTH_KW, 0);
    const rd_kw_type *swat_kw =
        rd_file_view_iget_named_kw(rft_view, SWAT_KW, 0);
    const rd_kw_type *sgas_kw =
        rd_file_view_iget_named_kw(rft_view, SGAS_KW, 0);
    const rd_kw_type *pressure_kw =
        rd_rft_node_get_pressure_kw(rft_node, rft_view);

    const float *SW = rd_kw_get_float_ptr(swat_kw);
    const float *SG = rd_kw_get_float_ptr(sgas_kw);
    const float *P = rd_kw_get_float_ptr(pressure_kw);
    const float *depth = rd_kw_get_float_ptr(depth_kw);
    const int *i = rd_kw_get_int_ptr(conipos);
    const int *j = rd_kw_get_int_ptr(conjpos);
    const int *k = rd_kw_get_int_ptr(conkpos);

    for (int c = 0; c < rd_kw_get_size(conipos); c++) {
        /* The connection coordinates are shifted -= 1; i.e. all internal usage is offset 0. */
        rd_rft_cell_type *cell = rd_rft_cell_alloc_RFT(
            i[c] - 1, j[c] - 1, k[c] - 1, depth[c], P[c], SW[c], SG[c]);
        rd_rft_node_append_cell(rft_node, cell);
    }
}

static void rd_rft_node_init_PLT_cells(rd_rft_node_type *rft_node,
                                       const rd_file_view_type *rft_view) {
    /* For PLT there is quite a lot of extra information which is not yet internalized. */
    const rd_kw_type *conipos =
        rd_file_view_iget_named_kw(rft_view, CONIPOS_KW, 0);
    const rd_kw_type *conjpos =
        rd_file_view_iget_named_kw(rft_view, CONJPOS_KW, 0);
    const rd_kw_type *conkpos =
        rd_file_view_iget_named_kw(rft_view, CONKPOS_KW, 0);

    const int *i = rd_kw_get_int_ptr(conipos);
    const int *j = rd_kw_get_int_ptr(conjpos);
    const int *k = rd_kw_get_int_ptr(conkpos);

    const float *WR = rd_kw_get_float_ptr(
        rd_file_view_iget_named_kw(rft_view, CONWRAT_KW, 0));
    const float *GR = rd_kw_get_float_ptr(
        rd_file_view_iget_named_kw(rft_view, CONGRAT_KW, 0));
    const float *OR = rd_kw_get_float_ptr(
        rd_file_view_iget_named_kw(rft_view, CONORAT_KW, 0));
    const float *P =
        rd_kw_get_float_ptr(rd_rft_node_get_pressure_kw(rft_node, rft_view));
    const float *depth = rd_kw_get_float_ptr(
        rd_file_view_iget_named_kw(rft_view, CONDEPTH_KW, 0));
    const float *flowrate = rd_kw_get_float_ptr(
        rd_file_view_iget_named_kw(rft_view, CONVTUB_KW, 0));
    const float *oil_flowrate = rd_kw_get_float_ptr(
        rd_file_view_iget_named_kw(rft_view, CONOTUB_KW, 0));
    const float *gas_flowrate = rd_kw_get_float_ptr(
        rd_file_view_iget_named_kw(rft_view, CONGTUB_KW, 0));
    const float *water_flowrate = rd_kw_get_float_ptr(
        rd_file_view_iget_named_kw(rft_view, CONWTUB_KW, 0));
    const float *connection_start = nullptr;
    const float *connection_end = nullptr;

    /* The keywords CONLENST_KW and CONLENEN_KW are ONLY present if we are dealing with a MSW well. */
    if (rd_file_view_has_kw(rft_view, CONLENST_KW))
        connection_start = rd_kw_get_float_ptr(
            rd_file_view_iget_named_kw(rft_view, CONLENST_KW, 0));

    if (rd_file_view_has_kw(rft_view, CONLENEN_KW))
        connection_end = rd_kw_get_float_ptr(
            rd_file_view_iget_named_kw(rft_view, CONLENEN_KW, 0));

    for (int c = 0; c < rd_kw_get_size(conipos); c++) {
        rd_rft_cell_type *cell;
        double cs = 0;
        double ce = 0;

        if (connection_start)
            cs = connection_start[c];

        if (connection_end)
            ce = connection_end[c];

        /* The connection coordinates are shifted -= 1; i.e. all internal usage is offset 0. */
        cell = rd_rft_cell_alloc_PLT(i[c] - 1, j[c] - 1, k[c] - 1, depth[c],
                                     P[c], OR[c], GR[c], WR[c], cs, ce,
                                     flowrate[c], oil_flowrate[c],
                                     gas_flowrate[c], water_flowrate[c]);
        rd_rft_node_append_cell(rft_node, cell);
    }
}

static void rd_rft_node_init_cells(rd_rft_node_type *rft_node,
                                   const rd_file_view_type *rft_view) {

    if (rft_node->data_type == RFT)
        rd_rft_node_init_RFT_cells(rft_node, rft_view);
    else if (rft_node->data_type == PLT)
        rd_rft_node_init_PLT_cells(rft_node, rft_view);
}

rd_rft_node_type *rd_rft_node_alloc(const rd_file_view_type *rft_view) {
    rd_kw_type *welletc = rd_file_view_iget_named_kw(rft_view, WELLETC_KW, 0);
    rd_rft_node_type *rft_node = rd_rft_node_alloc_empty(
        (const char *)rd_kw_iget_ptr(welletc, WELLETC_TYPE_INDEX));

    if (rft_node != nullptr) {
        rd_kw_type *date_kw = rd_file_view_iget_named_kw(rft_view, DATE_KW, 0);
        {
            char *tmp = util_alloc_strip_copy(
                (const char *)rd_kw_iget_ptr(welletc, WELLETC_NAME_INDEX));
            rft_node->well_name = std::string(tmp);
            free(tmp);
        }
        /* Time information. */
        {
            int *time = rd_kw_get_int_ptr(date_kw);
            rft_node->recording_date =
                rd_make_date(time[DATE_DAY_INDEX], time[DATE_MONTH_INDEX],
                             time[DATE_YEAR_INDEX]);
        }
        rft_node->days = rd_kw_iget_float(
            rd_file_view_iget_named_kw(rft_view, TIME_KW, 0), 0);
        if (rd_file_view_has_kw(rft_view, CONLENST_KW))
            rft_node->MSW = true;
        else
            rft_node->MSW = false;

        rd_rft_node_init_cells(rft_node, rft_view);
    }
    return rft_node;
}

const char *rd_rft_node_get_well_name(const rd_rft_node_type *rft_node) {
    return rft_node->well_name.c_str();
}

void rd_rft_node_free(rd_rft_node_type *rft_node) {
    for (auto cell_ptr : rft_node->cells)
        rd_rft_cell_free(cell_ptr);

    delete rft_node;
}

int rd_rft_node_get_size(const rd_rft_node_type *rft_node) {
    return rft_node->cells.size();
}
time_t rd_rft_node_get_date(const rd_rft_node_type *rft_node) {
    return rft_node->recording_date;
}
rd_rft_enum rd_rft_node_get_type(const rd_rft_node_type *rft_node) {
    return rft_node->data_type;
}

const rd_rft_cell_type *rd_rft_node_iget_cell(const rd_rft_node_type *rft_node,
                                              int index) {
    return rft_node->cells[index];
}

const rd_rft_cell_type *rd_rft_node_iget_cell_sorted(rd_rft_node_type *rft_node,
                                                     int index) {
    return rft_node->cells[index];
}

double rd_rft_node_iget_depth(const rd_rft_node_type *rft_node, int index) {
    const rd_rft_cell_type *cell = rd_rft_node_iget_cell(rft_node, index);
    return rd_rft_cell_get_depth(cell);
}

double rd_rft_node_iget_pressure(const rd_rft_node_type *rft_node, int index) {
    const rd_rft_cell_type *cell = rd_rft_node_iget_cell(rft_node, index);
    return rd_rft_cell_get_pressure(cell);
}

void rd_rft_node_iget_ijk(const rd_rft_node_type *rft_node, int index, int *i,
                          int *j, int *k) {
    const rd_rft_cell_type *cell = rd_rft_node_iget_cell(rft_node, index);

    rd_rft_cell_get_ijk(cell, i, j, k);
}

const rd_rft_cell_type *rd_rft_node_lookup_ijk(const rd_rft_node_type *rft_node,
                                               int i, int j, int k) {
    int index = 0;
    int size = rd_rft_node_get_size(rft_node);
    while (true) {
        const rd_rft_cell_type *cell = rd_rft_node_iget_cell(rft_node, index);

        if (rd_rft_cell_ijk_equal(cell, i, j, k))
            return cell;

        index++;
        if (index == size) /* Could not find it. */
            return nullptr;
    }
}

static void assert_type_and_index(const rd_rft_node_type *rft_node,
                                  rd_rft_enum target_type, int index) {
    if (rft_node->data_type != target_type)
        util_abort("%s: wrong type \n", __func__);

    if ((index < 0) || (index >= static_cast<int>(rft_node->cells.size())))
        util_abort("%s: invalid index:%d \n", __func__, index);
}

double rd_rft_node_iget_sgas(const rd_rft_node_type *rft_node, int index) {
    assert_type_and_index(rft_node, RFT, index);
    {
        const rd_rft_cell_type *cell = rd_rft_node_iget_cell(rft_node, index);
        return rd_rft_cell_get_sgas(cell);
    }
}

double rd_rft_node_iget_swat(const rd_rft_node_type *rft_node, int index) {
    assert_type_and_index(rft_node, RFT, index);
    {
        const rd_rft_cell_type *cell = rft_node->cells[index];
        return rd_rft_cell_get_swat(cell);
    }
}

double rd_rft_node_get_days(const rd_rft_node_type *rft_node) {
    return rft_node->days;
}

double rd_rft_node_iget_soil(const rd_rft_node_type *rft_node, int index) {
    assert_type_and_index(rft_node, RFT, index);
    {
        const rd_rft_cell_type *cell = rft_node->cells[index];
        return rd_rft_cell_get_soil(cell);
    }
}

double rd_rft_node_iget_orat(const rd_rft_node_type *rft_node, int index) {
    assert_type_and_index(rft_node, PLT, index);
    {
        const rd_rft_cell_type *cell = rft_node->cells[index];
        return rd_rft_cell_get_orat(cell);
    }
}

double rd_rft_node_iget_wrat(const rd_rft_node_type *rft_node, int index) {
    assert_type_and_index(rft_node, PLT, index);
    {
        const rd_rft_cell_type *cell = rft_node->cells[index];
        return rd_rft_cell_get_wrat(cell);
    }
}

double rd_rft_node_iget_grat(const rd_rft_node_type *rft_node, int index) {
    assert_type_and_index(rft_node, PLT, index);
    {
        const rd_rft_cell_type *cell = rft_node->cells[index];
        return rd_rft_cell_get_grat(cell);
    }
}

bool rd_rft_node_is_MSW(const rd_rft_node_type *rft_node) {
    return rft_node->MSW;
}

bool rd_rft_node_is_PLT(const rd_rft_node_type *rft_node) {
    if (rft_node->data_type == PLT)
        return true;
    else
        return false;
}

bool rd_rft_node_is_SEGMENT(const rd_rft_node_type *rft_node) {
    if (rft_node->data_type == SEGMENT)
        return true;
    else
        return false;
}

bool rd_rft_node_is_RFT(const rd_rft_node_type *rft_node) {
    if (rft_node->data_type == RFT)
        return true;
    else
        return false;
}
