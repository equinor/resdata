#pragma once

#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>

#include <resdata/rd_kw.hpp>
#include <resdata/rd_util.hpp>
#include <resdata/rd_rsthead.hpp>
#include <resdata/rd_units.hpp>

#define WELL_CONN_NORMAL_WELL_SEGMENT_ID -999
enum class WellConnDir { X = 1, Y = 2, Z = 3, fracX = 4, fracY = 5 };

class InvalidDirection : public std::invalid_argument {
public:
    explicit InvalidDirection(const std::string &message)
        : std::invalid_argument(message) {}
};

class InvalidConnection : public std::invalid_argument {
public:
    explicit InvalidConnection(const std::string &message)
        : std::invalid_argument(message) {}
};

class WellConnection {
    int i;
    int j;
    int k;
    WellConnDir dir;
    bool open;
    int segment_id;         // -1: Ordinary well
    bool matrix_connection; // k >= nz => fracture (and k -= nz )
    double connection_factor;
    double oil_rate;
    double gas_rate;
    double water_rate;
    double volume_rate;
    ert_rd_unit_enum unit_system;

public:
    WellConnection(int i, int j, int k, double connection_factor,
                   WellConnDir dir, bool open,
                   int segment_id = WELL_CONN_NORMAL_WELL_SEGMENT_ID,
                   bool matrix_connection = true,
                   ert_rd_unit_enum unit_system = RD_METRIC_UNITS);

    [[nodiscard]] bool operator==(const WellConnection &other) const {
        return std::tie(i, j, k, dir, open, segment_id, matrix_connection,
                        connection_factor, oil_rate, gas_rate, water_rate,
                        volume_rate, unit_system) ==
               std::tie(other.i, other.j, other.k, other.dir, other.open,
                        other.segment_id, other.matrix_connection,
                        other.connection_factor, other.oil_rate, other.gas_rate,
                        other.water_rate, other.volume_rate, other.unit_system);
    }
    [[nodiscard]] double get_connection_factor() const {
        return connection_factor;
    }

    static std::shared_ptr<WellConnection>
    from_keywords(const rd_kw_type *icon_kw, const rd_kw_type *scon_kw,
                  const rd_kw_type *xcon_kw, const RSTHead &header, int well_nr,
                  int conn_nr);
    static std::shared_ptr<WellConnection>
    read_wellhead(const rd_kw_type *iwel_kw, const RSTHead &header,
                  int well_nr);
    [[nodiscard]] bool is_MSW() const {
        return segment_id != WELL_CONN_NORMAL_WELL_SEGMENT_ID;
    }
    [[nodiscard]] int get_i() const { return i; }
    [[nodiscard]] int get_j() const { return j; }
    [[nodiscard]] int get_k() const { return k; }
    [[nodiscard]] WellConnDir get_dir() const { return dir; }
    [[nodiscard]] bool is_open() const { return open; }
    [[nodiscard]] int get_segment_id() const { return segment_id; }
    [[nodiscard]] bool is_fracture_connection() const {
        return !matrix_connection;
    }
    [[nodiscard]] bool is_matrix_connection() const {
        return matrix_connection;
    }
    [[nodiscard]] double get_oil_rate() const { return oil_rate; }
    [[nodiscard]] double get_gas_rate() const { return gas_rate; }
    [[nodiscard]] double get_water_rate() const { return water_rate; }
    [[nodiscard]] double get_volume_rate() const { return volume_rate; }
    [[nodiscard]] double get_oil_rate_si() const {
        return oil_rate * liquid_conversion_factor(unit_system);
    }
    [[nodiscard]] double get_gas_rate_si() const {
        return gas_rate * gas_conversion_factor(unit_system);
    }
    [[nodiscard]] double get_water_rate_si() const {
        return water_rate * liquid_conversion_factor(unit_system);
    }
    [[nodiscard]] double get_volume_rate_si() const {
        return volume_rate * liquid_conversion_factor(unit_system);
    }
};
