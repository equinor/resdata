#include <cstring>

#include <fmt/format.h>
#include <memory>

#include <resdata/rd_kw.hpp>
#include <resdata/rd_rsthead.hpp>
#include <resdata/rd_units.hpp>

#include <resdata/well/well_const.hpp>
#include <resdata/well/well_conn.hpp>

/*
  Observe that when the (ijk) values are initialized they are
  shifted to zero offset values, to be aligned with the rest of the
  ert libraries.
*/

static void well_conn_assert_direction(WellConnDir dir,
                                       bool matrix_connection) {
    if ((dir == WellConnDir::fracX || dir == WellConnDir::fracY) &&
        matrix_connection)
        throw InvalidDirection(
            fmt::format("Connection contained invalid direction, dir:{}  "
                        "matrix_connection:{}",
                        dir, matrix_connection));
}

WellConnection::WellConnection(int i, int j, int k, double connection_factor,
                               WellConnDir dir, bool open, int segment_id,
                               bool matrix_connection,
                               ert_rd_unit_enum unit_system)
    : i(i), j(j), k(k), dir(dir), open(open),
      matrix_connection(matrix_connection),
      connection_factor(connection_factor), oil_rate(0.0), gas_rate(0.0),
      water_rate(0.0), volume_rate(0.0), unit_system(unit_system) {

    well_conn_assert_direction(dir, matrix_connection);
    if (segment_id == CONN_NORMAL_WELL_SEGMENT_VALUE)
        this->segment_id = WELL_CONN_NORMAL_WELL_SEGMENT_ID;
    else
        this->segment_id = segment_id;
}

/*
  Observe that the (ijk) and branch values are shifted to zero offset to be
  aligned with the rest of the ert libraries.
*/
std::shared_ptr<WellConnection>
WellConnection::from_keywords(const rd_kw_type *icon_kw,
                              const rd_kw_type *scon_kw,
                              const rd_kw_type *xcon_kw, const RSTHead &header,
                              int well_nr, int conn_nr) {

    const int icon_offset = header.niconz * (header.ncwmax * well_nr + conn_nr);
    int IC = rd_kw_iget_int(icon_kw, icon_offset + ICON_IC_INDEX);
    if (IC <= 0)
        throw InvalidConnection("IC <= 0: Connection not in current LGR");

    /*
    Out in the wild we have encountered files where the integer value used to
    indicate direction has had an invalid value for some connections.
    */
    int int_direction =
        rd_kw_iget_int(icon_kw, icon_offset + ICON_DIRECTION_INDEX);
    if ((int_direction < 0) || (int_direction > ICON_FRACY))
        throw InvalidConnection(fmt::format(
            "Invalid direction value:{} encountered for well", int_direction));

    int i = rd_kw_iget_int(icon_kw, icon_offset + ICON_I_INDEX) - 1;
    int j = rd_kw_iget_int(icon_kw, icon_offset + ICON_J_INDEX) - 1;
    int k = rd_kw_iget_int(icon_kw, icon_offset + ICON_K_INDEX) - 1;
    double connection_factor = -1;
    bool matrix_connection = true;
    bool is_open =
        (rd_kw_iget_int(icon_kw, icon_offset + ICON_STATUS_INDEX) > 0);
    auto dir = WellConnDir::fracX;

    /* Set the K value and fracture flag. */
    {
        if (header.dualp) {
            int geometric_nz = header.nz / 2;
            if (k >= geometric_nz) {
                k -= geometric_nz;
                matrix_connection = false;
            }
        }
    }

    /* Set the direction flag */
    if (int_direction == ICON_DEFAULT_DIR_VALUE)
        int_direction = ICON_DEFAULT_DIR_TARGET;

    switch (int_direction) {
    case (ICON_DIRX):
        dir = WellConnDir::X;
        break;
    case (ICON_DIRY):
        dir = WellConnDir::Y;
        break;
    case (ICON_DIRZ):
        dir = WellConnDir::Z;
        break;
    case (ICON_FRACX):
        dir = WellConnDir::fracX;
        break;
    case (ICON_FRACY):
        dir = WellConnDir::fracY;
        break;
    }

    if (scon_kw) {
        const int scon_offset =
            header.nsconz * (header.ncwmax * well_nr + conn_nr);
        connection_factor =
            rd_kw_iget_as_double(scon_kw, scon_offset + SCON_CF_INDEX);
    }

    {
        int segment_id =
            rd_kw_iget_int(icon_kw, icon_offset + ICON_SEGMENT_INDEX) -
            ECLIPSE_WELL_SEGMENT_OFFSET + WELL_SEGMENT_OFFSET;
        auto conn = std::make_shared<WellConnection>(
            i, j, k, connection_factor, dir, is_open, segment_id,
            matrix_connection, header.unit_system);

        if (xcon_kw) {
            const int xcon_offset =
                header.nxconz * (header.ncwmax * well_nr + conn_nr);

            conn->water_rate =
                rd_kw_iget_as_double(xcon_kw, xcon_offset + XCON_WRAT_INDEX);
            conn->gas_rate =
                rd_kw_iget_as_double(xcon_kw, xcon_offset + XCON_GRAT_INDEX);
            conn->oil_rate =
                rd_kw_iget_as_double(xcon_kw, xcon_offset + XCON_ORAT_INDEX);
            conn->volume_rate =
                rd_kw_iget_double(xcon_kw, xcon_offset + XCON_QR_INDEX);
        }

        /**
       For multisegmented wells ONLY the global part of the restart
       file has segment information, i.e. the ?SEG
       keywords. Consequently iseg_kw will be nullptr for the part of a
       MSW + LGR well.
    */

        return conn;
    }
}

std::shared_ptr<WellConnection>
WellConnection::read_wellhead(const rd_kw_type *iwel_kw, const RSTHead &header,
                              int well_nr) {
    const int iwel_offset = header.niwelz * well_nr;
    int conn_i = rd_kw_iget_int(iwel_kw, iwel_offset + IWEL_HEADI_INDEX) - 1;

    if (conn_i >= 0) {
        int conn_j =
            rd_kw_iget_int(iwel_kw, iwel_offset + IWEL_HEADJ_INDEX) - 1;
        int conn_k =
            rd_kw_iget_int(iwel_kw, iwel_offset + IWEL_HEADK_INDEX) - 1;
        bool matrix_connection = true;
        bool open = true;
        double connection_factor = -1;

        if (header.dualp) {
            int geometric_nz = header.nz / 2;
            if (conn_k >= geometric_nz) {
                conn_k -= geometric_nz;
                matrix_connection = false;
            }
        }

        if (matrix_connection)
            return std::make_shared<WellConnection>(
                conn_i, conn_j, conn_k, connection_factor, WellConnDir::Z, open,
                WELL_CONN_NORMAL_WELL_SEGMENT_ID, true, header.unit_system);
        else
            return std::make_shared<WellConnection>(
                conn_i, conn_j, conn_k, connection_factor, WellConnDir::Z, open,
                WELL_CONN_NORMAL_WELL_SEGMENT_ID, false, header.unit_system);
    } else
        // The well is completed in this LGR - however the wellhead is in another LGR.
        return nullptr;
}
