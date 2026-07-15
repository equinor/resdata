#include <cstdlib>
#include <ctime>

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <iostream>
#include <fmt/format.h>

#include <resdata/rd_rsthead.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/rd_kw_magic.hpp>
#include <resdata/rd_util.hpp>
#include <resdata/rd_grid.hpp>
#include <resdata/rd_file_view.hpp>

#include <resdata/well/well_const.hpp>
#include <resdata/well/well_conn.hpp>
#include <resdata/well/well_state.hpp>
#include <resdata/well/well_segment_collection.hpp>
#include <resdata/well/well_branch_collection.hpp>
#include <resdata/well/well_rseg_loader.hpp>

WellState::WellState(std::string well_name, int global_well_nr, bool open,
                     WellType type, int report_nr, time_t valid_from)
    : name(well_name), valid_from_time(valid_from),
      valid_from_report(report_nr), global_well_nr(global_well_nr), open(open),
      type(type), is_MSW_well(false), oil_rate(0.0), gas_rate(0.0),
      water_rate(0.0), volume_rate(0.0), unit_system(RD_METRIC_UNITS) {
    segments.reset(well_segment_collection_alloc());
    branches.reset(well_branch_collection_alloc());
    /* See documentation of the 'IWEL_UNDOCUMENTED_ZERO' in well_const.h */
    if ((type == WellType::ZERO) && open)
        throw std::invalid_argument("Invalid type value for open wells.");
}

void WellState::add_wellhead(const RSTHead &header, const rd_kw_type *iwel_kw,
                             int well_nr, const std::string &grid_name,
                             int grid_nr) {
    auto wellhead = WellConnection::read_wellhead(iwel_kw, header, well_nr);

    if (wellhead) {
        if (grid_nr >= static_cast<int>(this->index_wellhead.size()))
            this->index_wellhead.resize(grid_nr + 1);
        this->name_wellhead[grid_name] = wellhead;
        this->index_wellhead[grid_nr] = std::move(wellhead);
    }
}

bool WellState::add_rates(rd::FileView *rst_view, int well_nr) {

    bool has_xwel_kw = rst_view->has_kw(XWEL_KW);
    if (has_xwel_kw) {
        const rd_kw_type *xwel_kw = rst_view->get_kw(XWEL_KW, 0);
        auto header = RSTHead::read(rst_view, -1);
        int offset = header.nxwelz * well_nr;

        this->unit_system = header.unit_system;
        this->oil_rate =
            rd_kw_iget_double(xwel_kw, offset + XWEL_RES_ORAT_ITEM);
        this->gas_rate =
            rd_kw_iget_double(xwel_kw, offset + XWEL_RES_GRAT_ITEM);
        this->water_rate =
            rd_kw_iget_double(xwel_kw, offset + XWEL_RES_WRAT_ITEM);
        this->volume_rate = rd_kw_iget_double(xwel_kw, offset + XWEL_RESV_ITEM);
    }
    return has_xwel_kw;
}

/*
  This function assumes that the rd_file state has been restricted
  to one LGR block with the rd_file_subselect_block() function.

  Return value: -1 means that the well is not found in this LGR at
  all.
*/

int WellState::get_lgr_well_nr(rd::FileView *file_view) {
    int well_nr = -1;

    if (file_view->has_kw(ZWEL_KW)) {
        auto header = RSTHead::read(file_view, -1);
        const rd_kw_type *zwel_kw = file_view->get_kw(ZWEL_KW, 0);
        int num_wells = header.nwells;
        well_nr = 0;
        while (true) {
            bool found = false;
            std::string lgr_well_name =
                rd_kw_iget_stripped_string(zwel_kw, well_nr * header.nzwelz);

            if (this->name == lgr_well_name)
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

WellType well_state_translate_rd_type_int(int int_type) {
    auto type = WellType::ZERO;

    switch (int_type) {
        /* See documentation of the 'IWEL_UNDOCUMENTED_ZERO' in well_const.h */
    case (IWEL_UNDOCUMENTED_ZERO):
        type = WellType::ZERO;
        break;
    case (IWEL_PRODUCER):
        type = WellType::PRODUCER;
        break;
    case (IWEL_OIL_INJECTOR):
        type = WellType::OIL_INJECTOR;
        break;
    case (IWEL_GAS_INJECTOR):
        type = WellType::GAS_INJECTOR;
        break;
    case (IWEL_WATER_INJECTOR):
        type = WellType::WATER_INJECTOR;
        break;
    default:
        throw std::invalid_argument(
            fmt::format("Invalid type value {}", int_type));
    }
    return type;
}

/*
  This function assumes that the rd_file state has been restricted
  to one LGR block with the rd_file_subselect_block() function.
*/

void WellState::add_connections(rd::FileView *rst_view,
                                const std::string &grid_name, int grid_nr,
                                int well_nr) {

    auto header = RSTHead::read(rst_view, -1);
    const rd_kw_type *iwel_kw = rst_view->get_kw(IWEL_KW, 0);

    add_wellhead(header, iwel_kw, well_nr, grid_name, grid_nr);

    if (rst_view->has_kw(ICON_KW)) {
        const rd_kw_type *icon_kw = rst_view->get_kw(ICON_KW, 0);
        if (!has_grid_connections(grid_name))
            this->connections[grid_name];

        {
            rd_kw_type *scon_kw = nullptr;
            if (rst_view->has_kw(SCON_KW))
                scon_kw = rst_view->get_kw(SCON_KW, 0);

            rd_kw_type *xcon_kw = nullptr;
            if (rst_view->has_kw(XCON_KW)) {
                xcon_kw = rst_view->get_kw(XCON_KW, 0);
            }

            const int iwel_offset = header.niwelz * well_nr;
            int num_connections =
                rd_kw_iget_int(iwel_kw, iwel_offset + IWEL_CONNECTIONS_INDEX);

            for (int iconn = 0; iconn < num_connections; iconn++) {
                try {
                    this->connections[grid_name].push_back(
                        WellConnection::from_keywords(icon_kw, scon_kw, xcon_kw,
                                                      header, well_nr, iconn));
                } catch (const InvalidDirection &e) {
                    std::cerr << e.what() << std::endl;
                } catch (const InvalidConnection &e) {
                    std::cerr << e.what() << std::endl;
                }
            }
        }
    }
}

void WellState::add_global_connections(rd::FileView *rst_view, int well_nr) {
    add_connections(rst_view, RD_GRID_GLOBAL_GRID, 0, well_nr);
}

void WellState::add_LGR_connections(const rd_grid_type *grid,
                                    rd::FileView *file_view) {
    // Go through all the LGRs and add connections; both in the bulk
    // grid and as wellhead.

    int num_lgr = rd_grid_get_num_lgr(grid);
    for (int lgr_index = 0; lgr_index < num_lgr; lgr_index++) {
        auto lgr_view = file_view->blockview(LGR_KW, LGR_KW, lgr_index);
        /* Even though the grid has LGR information the restart file is not required
           to have corresponding LGR information. This has for a long time been
           unchecked, and there might be bugs lurking based on the incorrect
           assumption that if the grid has LGR information then the corresponding LGR
           information can also be found in the restart file. */
        if (lgr_view) {
            const char *grid_name = rd_grid_iget_lgr_name(grid, lgr_index);
            int well_nr = get_lgr_well_nr(lgr_view.get());
            if (well_nr >= 0)
                add_connections(lgr_view.get(), grid_name, lgr_index + 1,
                                well_nr);
        }
    }
}

void WellState::add_connections(const rd_grid_type *grid,
                                rd::FileView *rst_view, int well_nr) {

    add_global_connections(rst_view, well_nr);
    add_LGR_connections(grid, rst_view);
}

bool WellState::add_MSW(rd::FileView *rst_view, int well_nr,
                        bool load_segment_information) {

    if (rst_view->has_kw(ISEG_KW)) {
        auto rst_head = RSTHead::read(rst_view, -1);
        const rd_kw_type *iwel_kw = rst_view->get_kw(IWEL_KW, 0);
        const rd_kw_type *iseg_kw = rst_view->get_kw(ISEG_KW, 0);
        std::unique_ptr<well_rseg_loader_type, decltype(&well_rseg_loader_free)>
            rseg_loader(nullptr, well_rseg_loader_free);

        if (rst_view->has_kw(RSEG_KW)) {
            if (load_segment_information)
                rseg_loader.reset(well_rseg_loader_alloc(rst_view));

            int segment_count = well_segment_collection_load_from_kw(
                this->segments.get(), well_nr, iwel_kw, iseg_kw,
                rseg_loader.get(), rst_head, load_segment_information,
                &this->is_MSW_well);

            if (segment_count > 0) {

                auto it = this->connections.begin();
                while (it != this->connections.end()) {
                    well_segment_collection_add_connections(
                        this->segments.get(), it->first.c_str(), it->second);
                    it++;
                }

                well_segment_collection_link(this->segments.get());
                well_segment_collection_add_branches(this->segments.get(),
                                                     this->branches.get());
            }
            return true;
        }
    }
    return false;
}

std::shared_ptr<WellState> WellState::read_wells_in_restart(
    rd::FileView *file_view, const rd_grid_type *grid, int report_nr,
    int global_well_nr, bool load_segment_information) {
    if (file_view->has_kw(IWEL_KW)) {
        auto global_header = RSTHead::read(file_view, -1);
        const rd_kw_type *global_iwel_kw = file_view->get_kw(IWEL_KW, 0);
        const rd_kw_type *global_zwel_kw = file_view->get_kw(ZWEL_KW, 0);

        const int iwel_offset = global_header.niwelz * global_well_nr;

        bool open =
            rd_kw_iget_int(global_iwel_kw, iwel_offset + IWEL_STATUS_INDEX) > 0;

        auto type = WellType::ZERO;

        {
            int int_type =
                rd_kw_iget_int(global_iwel_kw, iwel_offset + IWEL_TYPE_INDEX);
            type = well_state_translate_rd_type_int(int_type);
        }

        const int zwel_offset = global_header.nzwelz * global_well_nr;
        std::string name =
            rd_kw_iget_stripped_string(global_zwel_kw, zwel_offset);

        auto well_state =
            std::make_shared<WellState>(name, global_well_nr, open, type,
                                        report_nr, global_header.sim_time);

        well_state->add_connections(grid, file_view, global_well_nr);
        if (file_view->has_kw(ISEG_KW))
            well_state->add_MSW(file_view, global_well_nr,
                                load_segment_information);

        well_state->add_rates(file_view, global_well_nr);
        return well_state;
    } else
        /* This seems a bit weird - have come over E300 restart files without the IWEL keyword. */
        return nullptr;
}
