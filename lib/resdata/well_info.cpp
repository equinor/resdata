#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <fmt/format.h>

#include <resdata/rd_rsthead.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/rd_file_view.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/rd_kw_magic.hpp>
#include <resdata/rd_util.hpp>
#include <resdata/well/well_const.hpp>
#include <resdata/well/well_state.hpp>
#include <resdata/well/well_info.hpp>
#include <resdata/rd_file_flag.hpp>
#include <resdata/well/well_ts.hpp>

namespace {
struct close_guard {
    explicit close_guard(rd::FileView *file_view)
        : file_view(file_view),
          was_set(file_view->drop_flags(FileMode::CLOSE_STREAM)) {}

    ~close_guard() {
        if (was_set)
            file_view->add_flag(FileMode::CLOSE_STREAM);
    }
    close_guard(const close_guard &) = delete;
    close_guard &operator=(const close_guard &) = delete;

    rd::FileView *file_view;
    bool was_set;
};

struct clear_guard {
    explicit clear_guard(std::shared_ptr<rd::FileView> file_view)
        : file_view(file_view) {}

    ~clear_guard() { file_view->clear(); }
    clear_guard(const clear_guard &) = delete;
    clear_guard &operator=(const clear_guard &) = delete;

    std::shared_ptr<rd::FileView> file_view;
};
} // namespace

void WellInfo::add_wells(rd_file_type *rst_file, int report_nr,
                         bool load_segment_information) {
    auto rst_view = rd_file_get_global_view(rst_file);
    add_wells(rst_view.get(), report_nr, load_segment_information);
}

void WellInfo::add_wells(rd::FileView *rst_view, int report_nr,
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
    auto rst_view = rd_file_get_global_view(rst_file);
    size_t num_blocks = rst_view->num_named_kw(SEQNUM_KW);
    for (size_t block_nr = 0; block_nr < num_blocks; block_nr++) {

        auto step_view = rst_view->restart_view_from_seqnum_index(block_nr);
        if (!step_view)
            throw std::runtime_error(
                fmt::format("Could not find restart step: {}", block_nr));

        const rd_kw_type *seqnum_kw = step_view->get_kw(SEQNUM_KW, 0);
        int report_nr = rd_kw_iget_int(seqnum_kw, 0);

        clear_guard clear(rst_view);
        add_wells(step_view.get(), report_nr, load_segment_information);
    }
}

void WellInfo::load_rstfile(const std::string &filename,
                            bool load_segment_information) {
    rd_file_ptr rd_file = rd::File::open(filename);
    load_rstfile(rd_file.get(), load_segment_information);
}

void WellInfo::load_rstfile(rd_file_type *rd_file,
                            bool load_segment_information) {
    int report_nr;
    std::string filename = rd_file->filename();
    rd_file_enum file_type =
        rd_get_file_type(filename.c_str(), nullptr, &report_nr);
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
