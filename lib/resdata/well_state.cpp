/**
   The well_state_type structure contains state information about one
   well for one particular point in time.
*/

#include "resdata/rd_file_view.hpp"
#include <cstdlib>
#include <ctime>

#include <ert/util/util.hpp>
#include <ert/util/vector.hpp>
#include <ert/util/hash.hpp>
#include <ert/util/int_vector.hpp>
#include <ert/util/type_macros.hpp>

#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <map>

#include <resdata/rd_rsthead.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/rd_kw_magic.hpp>
#include <resdata/rd_util.hpp>
#include <resdata/rd_grid.hpp>
#include <resdata/rd_units.hpp>
#include <resdata/rd_util.hpp>

#include <resdata/well/well_const.hpp>
#include <resdata/well/well_conn.hpp>
#include <resdata/well/well_state.hpp>
#include <resdata/well/well_segment_collection.hpp>
#include <resdata/well/well_branch_collection.hpp>
#include <resdata/well/well_rseg_loader.hpp>

/*

Connections, segments and branches
----------------------------------


   +-----+
   |     |  <- Wellhead
   |     |
   +-----+ _________ Segment 2
      |\  /
      | \/         Segment 1               Segment 0
      |  \-----0---------------0--<----------------------O           <-- Branch: 0
      \        |               |      |                  |
       \    +-----+         +-----++-----+            +-----+
        \   | C3  |         | C2  || C1  |            | C0  |
         \  |     |         |     ||     |            |     |
          \ +-----+         +-----++-----+            +-----+
           \
Segment 5   \
             \
              \        Segment 4                Segment 3
               \-<--O-------<-------O----------------<------------O   <-- Branch: 1
                    |               |          |                  |
                 +-----+         +-----+    +-----+            +-----+
                 | C7  |         | C6  |    | C5  |            | C4  |
                 |     |         |     |    |     |            |     |
                 +-----+         +-----+    +-----+            +-----+




The boxes show connections C0 - C7; the connections serve as sinks (or
sources in the case of injectors) removing fluids from the
reservoir. As indicated by the use of isolated boxes the model
contains no geomtric concept linking the different connections into a
connected 'well-like' object.

Ordinary wells in the model are just a collection of
connections like these illustrated boxes, and to draw a connected 1D
object heuristics of some kind must be used to determine how the
various connections should be connected. In particular for wells which
consist of multiple branches this heuristic is non obvious.

More advanced (i.e. newer) wells are modelles as multisegment wells;
the important thing about multisegment wells is that the 1D segments
constituting the flowpipe are modelled explicitly as 'segments', and
the equations of fluid flow are solved by ECLIPSE in these 1D
domains. The figure above shows a multisegment well with six segments
marked as Segment0 ... Segment5. The segments themselves are quite
abstract objects not directly linked to the grid, but indriectly
through the connections. In the figure the segment <-> connection
links are as follows:

  Segment0: C0, C1
  Segment1: C2
  Segment2: C3
  Segment3: C4, C5
  Segment4: C6state_free(
  Segment5: C7

Each segment has an outlet segment, which will link the segments
together into a geometry.

The connection can in general be both to the main global grid, and to
an LGR. Hence all questions about connections must be LGR aware. This
is in contrast to the segments and branches which are geometric
objects, not directly coupled to a specific grid; however the segments
have a collection of connections - and these connections are coupled to
a grid.
*/

#define WELL_STATE_TYPE_ID 613307832

struct well_state_struct {
    UTIL_TYPE_ID_DECLARATION;
    std::string name;
    time_t valid_from_time;
    int valid_from_report;
    int global_well_nr;
    bool open;
    well_type_enum type;
    bool is_MSW_well;
    double oil_rate;
    double gas_rate;
    double water_rate;
    double volume_rate;
    ert_rd_unit_enum unit_system;

    std::map<std::string, std::vector<well_conn_ptr>> connections;
    well_segment_collection_ptr segments{nullptr, well_segment_collection_free};
    well_branch_collection_ptr branches{nullptr, well_branch_collection_free};

    // The index_wellhead will own the reference to the well_conn
    // and the name_wellhead has a non-owning reference
    std::vector<well_conn_ptr>
        index_wellhead; // An well_conn_type instance representing the wellhead - indexed by grid_nr.
    std::map<std::string, well_conn_type *>
        name_wellhead; // An well_conn_type instance representing the wellhead - indexed by lgr_name.
};

UTIL_IS_INSTANCE_FUNCTION(well_state, WELL_STATE_TYPE_ID)

well_state_type *well_state_alloc(const char *well_name, int global_well_nr,
                                  bool open, well_type_enum type, int report_nr,
                                  time_t valid_from) {
    auto well_state = std::make_unique<well_state_type>();
    UTIL_TYPE_ID_INIT(well_state, WELL_STATE_TYPE_ID);

    well_state->name = well_name;
    well_state->valid_from_time = valid_from;
    well_state->valid_from_report = report_nr;
    well_state->open = open;
    well_state->type = type;
    well_state->global_well_nr = global_well_nr;
    well_state->segments.reset(well_segment_collection_alloc());
    well_state->branches.reset(well_branch_collection_alloc());
    well_state->is_MSW_well = false;
    well_state->oil_rate = 0;
    well_state->gas_rate = 0;
    well_state->water_rate = 0;
    well_state->volume_rate = 0;
    well_state->unit_system = RD_METRIC_UNITS;

    /* See documentation of the 'IWEL_UNDOCUMENTED_ZERO' in well_const.h */
    if ((type == RD_WELL_ZERO) && open)
        util_abort("%s: Invalid type value for open wells.\n", __func__);
    return well_state.release();
}

double well_state_get_oil_rate(const well_state_type *well_state) {
    return well_state->oil_rate;
}

double well_state_get_gas_rate(const well_state_type *well_state) {
    return well_state->gas_rate;
}

double well_state_get_water_rate(const well_state_type *well_state) {
    return well_state->water_rate;
}

double well_state_get_volume_rate(const well_state_type *well_state) {
    return well_state->volume_rate;
}

double well_state_get_oil_rate_si(const well_state_type *well_state) {
    double conversion_factor =
        liquid_conversion_factor(well_state->unit_system);
    return well_state->oil_rate * conversion_factor;
}

double well_state_get_gas_rate_si(const well_state_type *well_state) {
    double conversion_factor = gas_conversion_factor(well_state->unit_system);
    return well_state->gas_rate * conversion_factor;
}

double well_state_get_water_rate_si(const well_state_type *well_state) {
    double conversion_factor =
        liquid_conversion_factor(well_state->unit_system);
    return well_state->water_rate * conversion_factor;
}

double well_state_get_volume_rate_si(const well_state_type *well_state) {
    double conversion_factor =
        liquid_conversion_factor(well_state->unit_system);
    return well_state->volume_rate * conversion_factor;
}

static void well_state_add_wellhead(well_state_type *well_state,
                                    const RSTHead &header,
                                    const rd_kw_type *iwel_kw, int well_nr,
                                    const char *grid_name, int grid_nr) {
    well_conn_ptr wellhead(well_conn_alloc_wellhead(iwel_kw, header, well_nr));

    if (wellhead) {
        if (grid_nr >= static_cast<int>(well_state->index_wellhead.size()))
            well_state->index_wellhead.resize(grid_nr + 1);
        well_state->name_wellhead[grid_name] = wellhead.get();
        well_state->index_wellhead[grid_nr] = std::move(wellhead);
    }
}

static bool well_state_add_rates(well_state_type *well_state,
                                 rd_file_view_type *rst_view, int well_nr) {

    bool has_xwel_kw = rd_file_view_has_kw(rst_view, XWEL_KW);
    if (has_xwel_kw) {
        const rd_kw_type *xwel_kw =
            rd_file_view_iget_named_kw(rst_view, XWEL_KW, 0);
        auto header = RSTHead::read(rst_view, -1);
        int offset = header.nxwelz * well_nr;

        well_state->unit_system = header.unit_system;
        well_state->oil_rate =
            rd_kw_iget_double(xwel_kw, offset + XWEL_RES_ORAT_ITEM);
        well_state->gas_rate =
            rd_kw_iget_double(xwel_kw, offset + XWEL_RES_GRAT_ITEM);
        well_state->water_rate =
            rd_kw_iget_double(xwel_kw, offset + XWEL_RES_WRAT_ITEM);
        well_state->volume_rate =
            rd_kw_iget_double(xwel_kw, offset + XWEL_RESV_ITEM);
    }
    return has_xwel_kw;
}

/*
  This function assumes that the rd_file state has been restricted
  to one LGR block with the rd_file_subselect_block() function.

  Return value: -1 means that the well is not found in this LGR at
  all.
*/

static int well_state_get_lgr_well_nr(const well_state_type *well_state,
                                      const rd_file_view_type *file_view) {
    int well_nr = -1;

    if (rd_file_view_has_kw(file_view, ZWEL_KW)) {
        auto header = RSTHead::read(file_view, -1);
        const rd_kw_type *zwel_kw =
            rd_file_view_iget_named_kw(file_view, ZWEL_KW, 0);
        int num_wells = header.nwells;
        well_nr = 0;
        while (true) {
            bool found = false;
            std::string lgr_well_name =
                rd_kw_iget_stripped_string(zwel_kw, well_nr * header.nzwelz);

            if (well_state->name == lgr_well_name)
                found = true;
            else
                well_nr++;

            if (found)
                break;
            else if (well_nr == num_wells) {
                // The well is not in this LGR at all.
                well_nr = -1;
                break;
            }
        }
    }
    return well_nr;
}

well_type_enum well_state_translate_rd_type_int(int int_type) {
    well_type_enum type = RD_WELL_ZERO;

    switch (int_type) {
        /* See documentation of the 'IWEL_UNDOCUMENTED_ZERO' in well_const.h */
    case (IWEL_UNDOCUMENTED_ZERO):
        type = RD_WELL_ZERO;
        break;
    case (IWEL_PRODUCER):
        type = RD_WELL_PRODUCER;
        break;
    case (IWEL_OIL_INJECTOR):
        type = RD_WELL_OIL_INJECTOR;
        break;
    case (IWEL_GAS_INJECTOR):
        type = RD_WELL_GAS_INJECTOR;
        break;
    case (IWEL_WATER_INJECTOR):
        type = RD_WELL_WATER_INJECTOR;
        break;
    default:
        util_abort("%s: Invalid type value %d\n", __func__, int_type);
    }
    return type;
}

/*
  This function assumes that the rd_file state has been restricted
  to one LGR block with the rd_file_subselect_block() function.
*/

static void well_state_add_connections__(well_state_type *well_state,
                                         const rd_file_view_type *rst_view,
                                         const char *grid_name, int grid_nr,
                                         int well_nr) {

    auto header = RSTHead::read(rst_view, -1);
    const rd_kw_type *iwel_kw =
        rd_file_view_iget_named_kw(rst_view, IWEL_KW, 0);

    well_state_add_wellhead(well_state, header, iwel_kw, well_nr, grid_name,
                            grid_nr);

    if (rd_file_view_has_kw(rst_view, ICON_KW)) {
        const rd_kw_type *icon_kw =
            rd_file_view_iget_named_kw(rst_view, ICON_KW, 0);
        if (!well_state_has_grid_connections(well_state, grid_name))
            well_state->connections[grid_name];

        {
            rd_kw_type *scon_kw = NULL;
            if (rd_file_view_has_kw(rst_view, SCON_KW))
                scon_kw = rd_file_view_iget_named_kw(rst_view, SCON_KW, 0);

            rd_kw_type *xcon_kw = NULL;
            if (rd_file_view_has_kw(rst_view, XCON_KW)) {
                xcon_kw = rd_file_view_iget_named_kw(rst_view, XCON_KW, 0);
            }

            const int iwel_offset = header.niwelz * well_nr;
            int num_connections =
                rd_kw_iget_int(iwel_kw, iwel_offset + IWEL_CONNECTIONS_INDEX);

            for (int iconn = 0; iconn < num_connections; iconn++) {
                well_conn_ptr conn(well_conn_alloc_from_kw(
                    icon_kw, scon_kw, xcon_kw, header, well_nr, iconn));
                if (conn)
                    well_state->connections[grid_name].push_back(
                        std::move(conn));
            }
        }
    }
}

static void well_state_add_global_connections(well_state_type *well_state,
                                              const rd_file_view_type *rst_view,
                                              int well_nr) {
    well_state_add_connections__(well_state, rst_view, RD_GRID_GLOBAL_GRID, 0,
                                 well_nr);
}

static void well_state_add_LGR_connections(well_state_type *well_state,
                                           const rd_grid_type *grid,
                                           rd_file_view_type *file_view) {
    // Go through all the LGRs and add connections; both in the bulk
    // grid and as wellhead.

    int num_lgr = rd_grid_get_num_lgr(grid);
    for (int lgr_index = 0; lgr_index < num_lgr; lgr_index++) {
        rd_file_view_type *lgr_view =
            rd_file_view_add_blockview(file_view, LGR_KW, lgr_index);
        /*
      Even though the grid has LGR information the restart file is not required
      to have corresponding LGR information. This has for a long time been
      unchecked, and there might be bugs lurking based on the incorrect
      assumption that if the grid has LGR information then the corresponding LGR
      information can also be found in the restart file.
    */
        if (lgr_view) {
            const char *grid_name = rd_grid_iget_lgr_name(grid, lgr_index);
            int well_nr = well_state_get_lgr_well_nr(well_state, lgr_view);
            if (well_nr >= 0)
                well_state_add_connections__(well_state, lgr_view, grid_name,
                                             lgr_index + 1, well_nr);
        }
    }
}

void well_state_add_connections2(well_state_type *well_state,
                                 const rd_grid_type *grid,
                                 rd_file_view_type *rst_view, int well_nr) {

    well_state_add_global_connections(well_state, rst_view, well_nr);
    well_state_add_LGR_connections(well_state, grid, rst_view);
}

bool well_state_add_MSW2(well_state_type *well_state,
                         rd_file_view_type *rst_view, int well_nr,
                         bool load_segment_information) {

    if (rd_file_view_has_kw(rst_view, ISEG_KW)) {
        auto rst_head = RSTHead::read(rst_view, -1);
        const rd_kw_type *iwel_kw =
            rd_file_view_iget_named_kw(rst_view, IWEL_KW, 0);
        const rd_kw_type *iseg_kw =
            rd_file_view_iget_named_kw(rst_view, ISEG_KW, 0);
        std::unique_ptr<well_rseg_loader_type, decltype(&well_rseg_loader_free)>
            rseg_loader(nullptr, well_rseg_loader_free);

        int segment_count;

        if (rd_file_view_has_kw(rst_view, RSEG_KW)) {
            if (load_segment_information)
                rseg_loader.reset(well_rseg_loader_alloc(rst_view));

            segment_count = well_segment_collection_load_from_kw(
                well_state->segments.get(), well_nr, iwel_kw, iseg_kw,
                rseg_loader.get(), rst_head, load_segment_information,
                &well_state->is_MSW_well);

            if (segment_count > 0) {

                auto it = well_state->connections.begin();
                while (it != well_state->connections.end()) {
                    well_segment_collection_add_connections(
                        well_state->segments.get(), it->first.c_str(),
                        it->second);
                    it++;
                }

                well_segment_collection_link(well_state->segments.get());
                well_segment_collection_add_branches(
                    well_state->segments.get(), well_state->branches.get());
            }
            return true;
        }
    }
    return false;
}

bool well_state_is_MSW(const well_state_type *well_state) {
    return well_state->is_MSW_well;
}

bool well_state_has_segment_data(const well_state_type *well_state) {
    if (well_segment_collection_get_size(well_state->segments.get()) > 0)
        return true;
    else
        return false;
}

well_state_type *well_state_alloc_from_file(rd_file_type *rd_file,
                                            const rd_grid_type *grid,
                                            int report_nr, int global_well_nr,
                                            bool load_segment_information) {
    return well_state_alloc_from_file2(rd_file_get_active_view(rd_file), grid,
                                       report_nr, global_well_nr,
                                       load_segment_information);
}

well_state_type *well_state_alloc_from_file2(rd_file_view_type *file_view,
                                             const rd_grid_type *grid,
                                             int report_nr, int global_well_nr,
                                             bool load_segment_information) {
    if (rd_file_view_has_kw(file_view, IWEL_KW)) {
        auto global_header = RSTHead::read(file_view, -1);
        const rd_kw_type *global_iwel_kw =
            rd_file_view_iget_named_kw(file_view, IWEL_KW, 0);
        const rd_kw_type *global_zwel_kw =
            rd_file_view_iget_named_kw(file_view, ZWEL_KW, 0);

        const int iwel_offset = global_header.niwelz * global_well_nr;

        bool open;
        well_type_enum type = RD_WELL_ZERO;
        {
            int int_state =
                rd_kw_iget_int(global_iwel_kw, iwel_offset + IWEL_STATUS_INDEX);
            if (int_state > 0)
                open = true;
            else
                open = false;
        }

        {
            int int_type =
                rd_kw_iget_int(global_iwel_kw, iwel_offset + IWEL_TYPE_INDEX);
            type = well_state_translate_rd_type_int(int_type);
        }

        const int zwel_offset = global_header.nzwelz * global_well_nr;
        std::string name =
            rd_kw_iget_stripped_string(global_zwel_kw, zwel_offset);

        std::unique_ptr<well_state_type, decltype(&well_state_free)> well_state(
            well_state_alloc(name.c_str(), global_well_nr, open, type,
                             report_nr, global_header.sim_time),
            well_state_free);

        well_state_add_connections2(well_state.get(), grid, file_view,
                                    global_well_nr);
        if (rd_file_view_has_kw(file_view, ISEG_KW))
            well_state_add_MSW2(well_state.get(), file_view, global_well_nr,
                                load_segment_information);

        well_state_add_rates(well_state.get(), file_view, global_well_nr);
        return well_state.release();
    } else
        /* This seems a bit weird - have come over E300 restart files without the IWEL keyword. */
        return nullptr;
}

void well_state_free(well_state_type *well) { delete well; }

int well_state_get_report_nr(const well_state_type *well_state) {
    return well_state->valid_from_report;
}

time_t well_state_get_sim_time(const well_state_type *well_state) {
    return well_state->valid_from_time;
}

static const well_conn_type *
well_state_get_wellhead(const well_state_type *well_state,
                        const char *grid_name) {
    const auto it = well_state->name_wellhead.find(grid_name);
    if (it != well_state->name_wellhead.end())
        return it->second;
    return NULL;
}

const well_conn_type *
well_state_get_global_wellhead(const well_state_type *well_state) {
    return well_state_get_wellhead(well_state, RD_GRID_GLOBAL_GRID);
}

well_type_enum well_state_get_type(const well_state_type *well_state) {
    return well_state->type;
}

bool well_state_is_open(const well_state_type *well_state) {
    return well_state->open;
}

int well_state_get_well_nr(const well_state_type *well_state) {
    return well_state->global_well_nr;
}

const char *well_state_get_name(const well_state_type *well_state) {
    return well_state->name.c_str();
}

const std::vector<well_conn_ptr> *
well_state_get_grid_connections(const well_state_type *well_state,
                                const std::string &grid_name) {
    auto it = well_state->connections.find(grid_name);
    return it != well_state->connections.end() ? &it->second : nullptr;
}

const std::vector<well_conn_ptr> *
well_state_get_global_connections(const well_state_type *well_state) {
    return well_state_get_grid_connections(well_state, RD_GRID_GLOBAL_GRID);
}

bool well_state_has_grid_connections(const well_state_type *well_state,
                                     const char *grid_name) {

    const auto it = well_state->connections.find(grid_name);
    if (it == well_state->connections.end())
        return false;
    return true;
}

bool well_state_has_global_connections(const well_state_type *well_state) {
    return well_state_has_grid_connections(well_state, RD_GRID_GLOBAL_GRID);
}

well_segment_collection_type *
well_state_get_segments(const well_state_type *well_state) {
    return well_state->segments.get();
}

well_branch_collection_type *
well_state_get_branches(const well_state_type *well_state) {
    return well_state->branches.get();
}
