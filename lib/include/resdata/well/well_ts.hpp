#pragma once
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include <resdata/well/well_state.hpp>

class WellTimeLine {
    std::string well_name;
    std::vector<std::shared_ptr<WellState>> ts;

public:
    explicit WellTimeLine(std::string well_name) : well_name(well_name) {};

    void add_well(const std::shared_ptr<WellState> &well_state) {
        ts.push_back(well_state);
    }
    [[nodiscard]] std::string name() const { return well_name; }
    [[nodiscard]] std::shared_ptr<WellState> at(size_t n) const {
        return ts.at(n);
    }
    [[nodiscard]] size_t size() const { return ts.size(); }
};
