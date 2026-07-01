#include <ctime>

#include <memory>
#include <stdexcept>
#include <string>
#include <fmt/format.h>

#include <resdata/rd_rsthead.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/rd_file_view.hpp>
#include <resdata/rd_grid.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/rd_kw_magic.hpp>
#include <resdata/rd_util.hpp>
#include <resdata/well/well_const.hpp>
#include <resdata/well/well_state.hpp>
#include <resdata/well/well_info.hpp>

namespace {
struct close_guard {
    explicit close_guard(rd_file_view_type *file_view)
        : file_view(file_view),
          was_set(rd_file_view_drop_flag(file_view, RD_FILE_CLOSE_STREAM)) {}

    ~close_guard() {
        if (was_set)
            rd_file_view_add_flag(file_view, RD_FILE_CLOSE_STREAM);
    }
    close_guard(const close_guard &) = delete;
    close_guard &operator=(const close_guard &) = delete;

    rd_file_view_type *file_view;
    bool was_set;
};

struct transaction_guard {
    explicit transaction_guard(rd_file_view_type *file_view)
        : file_view(file_view),
          transaction(rd_file_view_start_transaction(file_view)) {}

    ~transaction_guard() {
        if (transaction)
            rd_file_view_end_transaction(file_view, transaction);
    }
    transaction_guard(const transaction_guard &) = delete;
    transaction_guard &operator=(const transaction_guard &) = delete;

    rd_file_view_type *file_view;
    rd_file_transaction_type *transaction;
};
} // namespace

void WellInfo::add_wells(rd_file_type *rst_file, int report_nr,
                         bool load_segment_information) {
    rd_file_view_type *rst_view = rd_file_get_active_view(rst_file);
    add_wells(rst_view, report_nr, load_segment_information);
}

void WellInfo::add_wells(rd_file_view_type *rst_view, int report_nr,
                         bool load_segment_information) {
    close_guard close_stream_guard(rst_view);
    auto global_header = RSTHead::read(rst_view, report_nr);
    for (int well_nr = 0; well_nr < global_header.nwells; well_nr++) {
        auto well_state = WellState::read_wells_in_restart(
            rst_view, grid, report_nr, well_nr, load_segment_information);
        if (well_state) {
            std::string well_name = well_state->get_name();
            if (!has_well(well_name)) {
                wells[well_name] = std::make_shared<WellTimeLine>(well_name);
                well_names.push_back(well_name);
            }
            get_ts(well_name)->add_well(well_state);
        }
    }
}

void WellInfo::add_UNRST_wells(rd_file_type *rst_file,
                               bool load_segment_information) {
    rd_file_view_type *rst_view = rd_file_get_global_view(rst_file);
    int num_blocks = rd_file_view_get_num_named_kw(rst_view, SEQNUM_KW);
    for (int block_nr = 0; block_nr < num_blocks; block_nr++) {

        rd_file_view_type *step_view =
            rd_file_view_add_restart_view(rst_view, block_nr, -1, -1, -1);
        const rd_kw_type *seqnum_kw =
            rd_file_view_iget_named_kw(step_view, SEQNUM_KW, 0);
        int report_nr = rd_kw_iget_int(seqnum_kw, 0);

        transaction_guard transaction(rst_view);
        add_wells(step_view, report_nr, load_segment_information);
    }
}

void WellInfo::load_rstfile(const std::string &filename,
                            bool load_segment_information) {
    rd_file_ptr rd_file(rd_file_open(filename.c_str(), 0), rd_file_close);
    load_rstfile(rd_file.get(), load_segment_information);
}

void WellInfo::load_rstfile(rd_file_type *rd_file,
                            bool load_segment_information) {
    int report_nr;
    const char *filename = rd_file_get_src_file(rd_file);
    rd_file_enum file_type = rd_get_file_type(filename, nullptr, &report_nr);
    if ((file_type == RD_RESTART_FILE) ||
        (file_type == RD_UNIFIED_RESTART_FILE)) {
        if (file_type == RD_RESTART_FILE)
            add_wells(rd_file, report_nr, load_segment_information);
        else
            add_UNRST_wells(rd_file, load_segment_information);

    } else
        throw std::invalid_argument(fmt::format(
            "invalid file type: {} - must be a restart file", filename));
}
