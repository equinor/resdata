#include <math.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#include <vector>
#include <algorithm>
#include <string>

#include <ert/util/util.h>

#include <resdata/rd_kw.hpp>
#include <resdata/rd_kw_magic.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/rd_file_view.hpp>
#include <resdata/rd_rft_node.hpp>
#include <resdata/rd_rft_cell.hpp>
#include <resdata/rd_type.hpp>

/**
    The RFT's from several wells, and possibly also several timesteps
    are lumped togeheter in one .RFT file. The rd_rft_node
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
    double days; /* When was the RFT recorded - days after simulaton start. */
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
   Will return NULL if the data_type_string is equal to "SEGMENT" -
   that is not (yet) supported.
*/
static rd_rft_enum
translate_from_sting_to_rd_rft_enum(const char *data_type_string) {
    rd_rft_enum data_type = SEGMENT;
    /* According to the ECLIPSE documentaton. */
    if (strchr(data_type_string, 'P') != NULL)
        data_type = PLT;
    else if (strchr(data_type_string, 'R') != NULL)
        data_type = RFT;
    else if (strchr(data_type_string, 'S') != NULL)
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

    /* Can return NULL */
    if (data_type == SEGMENT) {
        fprintf(
            stderr,
            "%s: sorry SEGMENT PLT/RFT is not supported - file a complaint. \n",
            __func__);
        return NULL;
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

void rd_rft_node_append_cell(rd_rft_node_type *rft_node,
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

    {
        int c;
        for (c = 0; c < rd_kw_get_size(conipos); c++) {
            /* The connection coordinates are shifted -= 1; i.e. all internal usage is offset 0. */
            rd_rft_cell_type *cell = rd_rft_cell_alloc_RFT(
                i[c] - 1, j[c] - 1, k[c] - 1, depth[c], P[c], SW[c], SG[c]);
            rd_rft_node_append_cell(rft_node, cell);
        }
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
    const float *connection_start = NULL;
    const float *connection_end = NULL;

    /* The keywords CONLENST_KW and CONLENEN_KW are ONLY present if we are dealing with a MSW well. */
    if (rd_file_view_has_kw(rft_view, CONLENST_KW))
        connection_start = rd_kw_get_float_ptr(
            rd_file_view_iget_named_kw(rft_view, CONLENST_KW, 0));

    if (rd_file_view_has_kw(rft_view, CONLENEN_KW))
        connection_end = rd_kw_get_float_ptr(
            rd_file_view_iget_named_kw(rft_view, CONLENEN_KW, 0));

    {
        int c;
        for (c = 0; c < rd_kw_get_size(conipos); c++) {
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

    if (rft_node != NULL) {
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

void rd_rft_node_free__(void *void_node) {
    rd_rft_node_free(rd_rft_node_safe_cast(void_node));
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
            return NULL;
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

static void rd_rft_node_fill_welletc(rd_kw_type *welletc,
                                     ert_rd_unit_enum unit_set) {
    if (unit_set == RD_METRIC_UNITS) {
        rd_kw_iset_string8(welletc, 0, "  DAYS");
        rd_kw_iset_string8(welletc, 2, "");
        rd_kw_iset_string8(welletc, 3, " METRES");
        rd_kw_iset_string8(welletc, 4, "  BARSA");
        rd_kw_iset_string8(welletc, 6, "STANDARD");
        rd_kw_iset_string8(welletc, 7, " SM3/DAY");
        rd_kw_iset_string8(welletc, 8, " SM3/DAY");
        rd_kw_iset_string8(welletc, 9, " RM3/DAY");
        rd_kw_iset_string8(welletc, 10, " M/SEC");
        rd_kw_iset_string8(welletc, 11, "");
        rd_kw_iset_string8(welletc, 12, "   CP");
        rd_kw_iset_string8(welletc, 13, " KG/SM3");
        rd_kw_iset_string8(welletc, 14, " KG/DAY");
        rd_kw_iset_string8(welletc, 15, "  KG/KG");
    } else if (unit_set == RD_FIELD_UNITS) {
        rd_kw_iset_string8(welletc, 0, "  DAYS");
        rd_kw_iset_string8(welletc, 2, "");
        rd_kw_iset_string8(welletc, 3, "  FEET");
        rd_kw_iset_string8(welletc, 4, "  PISA");
        rd_kw_iset_string8(welletc, 6, "STANDARD");
        rd_kw_iset_string8(welletc, 7, " STB/DAY");
        rd_kw_iset_string8(welletc, 8, " MSCF/DAY");
        rd_kw_iset_string8(welletc, 9, " RB/DAY");
        rd_kw_iset_string8(welletc, 10, " FT/SEC");
        rd_kw_iset_string8(welletc, 11, "");
        rd_kw_iset_string8(welletc, 12, "   CP");
        rd_kw_iset_string8(welletc, 13, " LB/STB");
        rd_kw_iset_string8(welletc, 14, " LB/DAY");
        rd_kw_iset_string8(welletc, 15, "  LB/LB");

    } else if (unit_set == RD_LAB_UNITS) {
        rd_kw_iset_string8(welletc, 0, "   HR");
        rd_kw_iset_string8(welletc, 2, "");
        rd_kw_iset_string8(welletc, 3, "   CM");
        rd_kw_iset_string8(welletc, 4, "  ATMA");
        rd_kw_iset_string8(welletc, 6, "STANDARD");
        rd_kw_iset_string8(welletc, 7, " SCC/HR");
        rd_kw_iset_string8(welletc, 8, " SCC/HR");
        rd_kw_iset_string8(welletc, 9, " RCC/SCC");
        rd_kw_iset_string8(welletc, 10, " CM/SEC");
        rd_kw_iset_string8(welletc, 11, "");
        rd_kw_iset_string8(welletc, 12, "   CP");
        rd_kw_iset_string8(welletc, 13, " GM/SCC");
        rd_kw_iset_string8(welletc, 14, " GH/HR");
        rd_kw_iset_string8(welletc, 15, "  GM/GM");
    }
}

void rd_rft_node_fwrite(const rd_rft_node_type *rft_node, fortio_type *fortio,
                        ert_rd_unit_enum unit_set) {
    rd_rft_enum type = rd_rft_node_get_type(rft_node);
    if (type != RFT)
        util_abort(
            "%s: sorry - only writing of simple RFT is currently implemented",
            __func__);

    {
        rd_kw_type *time = rd_kw_alloc(TIME_KW, 1, RD_FLOAT);
        rd_kw_iset_float(time, 0, rd_rft_node_get_days(rft_node));
        rd_kw_fwrite(time, fortio);
        rd_kw_free(time);
    }

    {
        rd_kw_type *datevalue = rd_kw_alloc(DATE_KW, 3, RD_INT);
        time_t date = rd_rft_node_get_date(rft_node);
        int day;
        int month;
        int year;
        rd_set_date_values(date, &day, &month, &year);
        rd_kw_iset_int(datevalue, 0, day);
        rd_kw_iset_int(datevalue, 1, month);
        rd_kw_iset_int(datevalue, 2, year);
        rd_kw_fwrite(datevalue, fortio);
        rd_kw_free(datevalue);
    }

    {
        rd_kw_type *welletc = rd_kw_alloc(WELLETC_KW, 16, RD_CHAR);
        rd_rft_enum type = rd_rft_node_get_type(rft_node);

        rd_kw_iset_string8(welletc, 1, rd_rft_node_get_well_name(rft_node));

        if (type == PLT) {
            rd_kw_iset_string8(welletc, 5, "P");
        } else if (type == RFT) {
            rd_kw_iset_string8(welletc, 5, "R");
        } else if (type == SEGMENT) {
            rd_kw_iset_string8(welletc, 5, "S");
        }
        rd_rft_node_fill_welletc(welletc, unit_set);
        rd_kw_fwrite(welletc, fortio);
        rd_kw_free(welletc);
    }

    {
        int size_cells = rd_rft_node_get_size(rft_node);
        rd_kw_type *conipos = rd_kw_alloc(CONIPOS_KW, size_cells, RD_INT);
        rd_kw_type *conjpos = rd_kw_alloc(CONJPOS_KW, size_cells, RD_INT);
        rd_kw_type *conkpos = rd_kw_alloc(CONKPOS_KW, size_cells, RD_INT);
        rd_kw_type *hostgrid = rd_kw_alloc(HOSTGRID_KW, size_cells, RD_CHAR);
        rd_kw_type *depth = rd_kw_alloc(DEPTH_KW, size_cells, RD_FLOAT);
        rd_kw_type *pressure = rd_kw_alloc(PRESSURE_KW, size_cells, RD_FLOAT);
        rd_kw_type *swat = rd_kw_alloc(SWAT_KW, size_cells, RD_FLOAT);
        rd_kw_type *sgas = rd_kw_alloc(SGAS_KW, size_cells, RD_FLOAT);

        for (int i = 0; i < size_cells; i++) {
            const rd_rft_cell_type *cell = rft_node->cells[i];
            rd_kw_iset_int(conipos, i, rd_rft_cell_get_i(cell) + 1);
            rd_kw_iset_int(conjpos, i, rd_rft_cell_get_j(cell) + 1);
            rd_kw_iset_int(conkpos, i, rd_rft_cell_get_k(cell) + 1);
            rd_kw_iset_float(depth, i, rd_rft_cell_get_depth(cell));
            rd_kw_iset_float(pressure, i, rd_rft_cell_get_pressure(cell));
            rd_kw_iset_float(swat, i, rd_rft_cell_get_swat(cell));
            rd_kw_iset_float(sgas, i, rd_rft_cell_get_sgas(cell));
        }
        rd_kw_fwrite(conipos, fortio);
        rd_kw_fwrite(conjpos, fortio);
        rd_kw_fwrite(conkpos, fortio);
        rd_kw_fwrite(hostgrid, fortio);
        rd_kw_fwrite(depth, fortio);
        rd_kw_fwrite(pressure, fortio);
        rd_kw_fwrite(swat, fortio);
        rd_kw_fwrite(sgas, fortio);

        rd_kw_free(conipos);
        rd_kw_free(conjpos);
        rd_kw_free(conkpos);
        rd_kw_free(hostgrid);
        rd_kw_free(depth);
        rd_kw_free(pressure);
        rd_kw_free(swat);
        rd_kw_free(sgas);
    }
}

int rd_rft_node_cmp(const rd_rft_node_type *n1, const rd_rft_node_type *n2) {
    time_t val1 = rd_rft_node_get_date(n1);
    time_t val2 = rd_rft_node_get_date(n2);

    if (val1 < val2)
        return -1;
    else if (val1 == val2)
        return 0;
    else
        return 1;
}

bool rd_rft_node_lt(const rd_rft_node_type *n1, const rd_rft_node_type *n2) {
    return (rd_rft_node_cmp(n1, n2) < 0);
}
