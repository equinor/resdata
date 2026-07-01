
#include <map>
#include <memory>
#include <stdexcept>
#include <string>

#include <resdata/rd_kw.hpp>
#include <resdata/rd_rsthead.hpp>

#include <resdata/well/well_rseg_loader.hpp>
#include <resdata/well/well_const.hpp>
#include <resdata/well/well_conn.hpp>
#include <resdata/well/well_segment.hpp>

std::shared_ptr<WellSegment> WellSegment::from_kw(
    const rd_kw_type *iseg_kw, const well_rseg_loader_type *rseg_loader,
    const RSTHead &header, int well_nr, int segment_index, int segment_id) {
    if (!rseg_loader) {
        throw std::invalid_argument(
            "fatal internal error - tried to create well_segment "
            "instance without RSEG keyword");
    } else {
        const int iseg_offset =
            header.nisegz * (header.nsegmx * well_nr + segment_index);
        const int rseg_offset =
            header.nrsegz * (header.nsegmx * well_nr + segment_index);
        int outlet_segment_id =
            rd_kw_iget_int(iseg_kw, iseg_offset + ISEG_OUTLET_INDEX) -
            ECLIPSE_WELL_SEGMENT_OFFSET + WELL_SEGMENT_OFFSET; // -1
        int branch_id =
            rd_kw_iget_int(iseg_kw, iseg_offset + ISEG_BRANCH_INDEX) -
            ECLIPSE_WELL_BRANCH_OFFSET + WELL_BRANCH_OFFSET; // -1
        const double *rseg_data =
            well_rseg_loader_load_values(rseg_loader, rseg_offset);

        auto segment = std::make_shared<WellSegment>(
            segment_id, outlet_segment_id, branch_id);
        segment->depth = rseg_data[0];
        segment->length = rseg_data[1];
        segment->total_length = rseg_data[2];
        segment->diameter = rseg_data[3];
        return segment;
    }
}

bool WellSegment::link(WellSegment *outlet_segment) {
    if (this->outlet_segment_id == outlet_segment->segment_id) {
        this->outlet_segment = outlet_segment;
        if (outlet_segment->branch_id == this->branch_id) {
            outlet_segment->link_count++;
        }
        return true;
    } else
        /* This is a quite fatal topological error - and aborting is probaly the wisest
           thing to do. */
        return false;
}

bool WellSegment::add_connection(const std::string &grid_name,
                                 std::shared_ptr<WellConnection> conn) {
    int conn_segment_id = conn->get_segment_id();
    if (conn_segment_id == this->segment_id) {
        connections[grid_name].push_back(conn);
        return true;
    } else
        return false;
}

bool well_segment_well_is_MSW(int well_nr, const rd_kw_type *iwel_kw,
                              const RSTHead &rst_head) {
    int iwel_offset = rst_head.niwelz * well_nr;
    int segment_well_nr =
        rd_kw_iget_int(iwel_kw, iwel_offset + IWEL_SEGMENTED_WELL_NR_INDEX) - 1;

    if (segment_well_nr == IWEL_SEGMENTED_WELL_NR_NORMAL_VALUE)
        return false;
    else
        return true;
}
