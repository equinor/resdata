#include <algorithm>
#include <cstddef>
#include <ios>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_range.hpp>

#include <resdata/FortIO.hpp>
#include <resdata/rd_grid.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/rd_kw_magic.hpp>
#include <resdata/rd_type.hpp>
#include <resdata/well/well_const.hpp>
#include <resdata/well/well_info.hpp>
#include <resdata/well/well_state.hpp>

#include "grid_fixtures.hpp"
#include "tmpdir.hpp"

namespace {

/*
 * Dimensions of the synthetic restart file. The values are the
 * record strides of the corresponding keywords and are chosen large enough to
 * contain every index the loader reads.
 */
struct Dims {
    size_t nx = 3;
    size_t ny = 3;
    size_t nz = 3;
    size_t nactive = 27;
    size_t nwells = 1;
    size_t ncwmax = 1;
    size_t niwelz = 72;
    size_t nzwelz = 3;
    size_t nxwelz = 8;
    size_t niconz = 16;
    size_t nsconz = 2;
    size_t nxconz = 52;
    size_t nisegz = 8;
    size_t nsegmx = 1;
    size_t nrsegz = 12;
    size_t nlbrmx = 1;
    size_t nilbrz = 2;
};

std::unique_ptr<rd::KW> build_intehead(const Dims &d) {
    std::vector<int> data(412, 0);
    data[INTEHEAD_UNIT_INDEX] = INTEHEAD_METRIC_VALUE;
    data[INTEHEAD_NX_INDEX] = static_cast<int>(d.nx);
    data[INTEHEAD_NY_INDEX] = static_cast<int>(d.ny);
    data[INTEHEAD_NZ_INDEX] = static_cast<int>(d.nz);
    data[INTEHEAD_NACTIVE_INDEX] = static_cast<int>(d.nactive);
    data[INTEHEAD_NWELLS_INDEX] = static_cast<int>(d.nwells);
    data[INTEHEAD_NCWMAX_INDEX] = static_cast<int>(d.ncwmax);
    data[INTEHEAD_NIWELZ_INDEX] = static_cast<int>(d.niwelz);
    data[INTEHEAD_NXWELZ_INDEX] = static_cast<int>(d.nxwelz);
    data[INTEHEAD_NZWELZ_INDEX] = static_cast<int>(d.nzwelz);
    data[INTEHEAD_NICONZ_INDEX] = static_cast<int>(d.niconz);
    data[INTEHEAD_NSCONZ_INDEX] = static_cast<int>(d.nsconz);
    data[INTEHEAD_NXCONZ_INDEX] = static_cast<int>(d.nxconz);
    data[INTEHEAD_NISEGZ_INDEX] = static_cast<int>(d.nisegz);
    data[INTEHEAD_NSEGMX_INDEX] = static_cast<int>(d.nsegmx);
    data[INTEHEAD_NSWLMX_INDEX] = 1;
    data[INTEHEAD_NLBRMX_INDEX] = static_cast<int>(d.nlbrmx);
    data[INTEHEAD_NILBRZ_INDEX] = static_cast<int>(d.nilbrz);
    data[INTEHEAD_NRSEGZ_INDEX] = static_cast<int>(d.nrsegz);
    return std::make_unique<rd::KW>(INTEHEAD_KW, std::move(data));
}

std::unique_ptr<rd::KW> build_logihead() {
    return std::make_unique<rd::KW>(LOGIHEAD_KW, LOGIHEAD_RESTART_SIZE,
                                    RD_BOOL);
}

std::unique_ptr<rd::KW> build_doubhead() {
    return std::make_unique<rd::KW>(DOUBHEAD_KW, std::vector<double>{0.0});
}

std::unique_ptr<rd::KW> build_iwel(const Dims &d) {
    std::vector<int> data(d.niwelz * d.nwells, 0);
    data[IWEL_HEADI_INDEX] = 1;
    data[IWEL_HEADJ_INDEX] = 1;
    data[IWEL_HEADK_INDEX] = 1;
    data[IWEL_CONNECTIONS_INDEX] = 1;
    data[IWEL_TYPE_INDEX] = IWEL_PRODUCER;
    data[IWEL_STATUS_INDEX] = 1;
    data[IWEL_SEGMENTED_WELL_NR_INDEX] = 1;
    return std::make_unique<rd::KW>(IWEL_KW, std::move(data));
}

std::unique_ptr<rd::KW> build_zwel(const Dims &d) {
    auto kw = std::make_unique<rd::KW>(ZWEL_KW, d.nzwelz * d.nwells, RD_CHAR);
    kw->set_padded(0, "WELL-1");
    return kw;
}

std::unique_ptr<rd::KW> build_icon(const Dims &d) {
    std::vector<int> data(d.niconz * d.ncwmax * d.nwells, 0);
    data[ICON_IC_INDEX] = 1;
    data[ICON_I_INDEX] = 1;
    data[ICON_J_INDEX] = 1;
    data[ICON_K_INDEX] = 1;
    data[ICON_STATUS_INDEX] = 1;
    data[ICON_DIRECTION_INDEX] = ICON_DIRZ;
    data[ICON_SEGMENT_INDEX] = 0;
    return std::make_unique<rd::KW>(ICON_KW, std::move(data));
}

std::unique_ptr<rd::KW> build_scon(const Dims &d) {
    std::vector<double> data(d.nsconz * d.ncwmax * d.nwells, 0.0);
    data[SCON_CF_INDEX] = 1.0;
    return std::make_unique<rd::KW>(SCON_KW, std::move(data));
}

std::unique_ptr<rd::KW> build_xcon(const Dims &d) {
    auto kw = std::make_unique<rd::KW>(XCON_KW, d.nxconz * d.ncwmax * d.nwells,
                                       RD_DOUBLE);
    kw->scalar_set<double>(0.0);
    return kw;
}

std::unique_ptr<rd::KW> build_iseg(const Dims &d) {
    std::vector<int> data(d.nisegz * d.nsegmx, 0);
    // Make the single segment inactive (branch -> INACTIVE)
    data[ISEG_OUTLET_INDEX] = 0;
    data[ISEG_BRANCH_INDEX] = -1;
    return std::make_unique<rd::KW>(ISEG_KW, std::move(data));
}

std::unique_ptr<rd::KW> build_rseg(const Dims &d) {
    return std::make_unique<rd::KW>(
        RSEG_KW, std::vector<double>(d.nrsegz * d.nsegmx, 0.0));
}

std::unique_ptr<rd::KW> build_xwel(const Dims &d) {
    return std::make_unique<rd::KW>(
        XWEL_KW, std::vector<double>(d.nxwelz * d.nwells, 0.0));
}

struct NamedKw {
    std::string name;
    std::unique_ptr<rd::KW> kw;
};

std::vector<NamedKw> build_all(const Dims &d) {
    std::vector<NamedKw> kws;
    kws.push_back({INTEHEAD_KW, build_intehead(d)});
    kws.push_back({LOGIHEAD_KW, build_logihead()});
    kws.push_back({DOUBHEAD_KW, build_doubhead()});
    kws.push_back({IWEL_KW, build_iwel(d)});
    kws.push_back({ZWEL_KW, build_zwel(d)});
    kws.push_back({ICON_KW, build_icon(d)});
    kws.push_back({SCON_KW, build_scon(d)});
    kws.push_back({XCON_KW, build_xcon(d)});
    kws.push_back({ISEG_KW, build_iseg(d)});
    kws.push_back({RSEG_KW, build_rseg(d)});
    kws.push_back({XWEL_KW, build_xwel(d)});
    return kws;
}

void write_file(const std::string &path, const std::vector<NamedKw> &kws) {
    ERT::FortIO fortio(path, std::ios_base::out);
    for (const auto &nk : kws) {
        if (nk.kw)
            nk.kw->fwrite(fortio);
    }
    fortio.fflush();
}

} // namespace

struct Spec {
    rd_data_type type;
    rd_data_type wrong;
};

const std::map<std::string, Spec> &kw_specs() {
    static const std::map<std::string, Spec> specs = {
        {INTEHEAD_KW, {RD_INT, RD_FLOAT}},  {LOGIHEAD_KW, {RD_BOOL, RD_INT}},
        {DOUBHEAD_KW, {RD_DOUBLE, RD_INT}}, {IWEL_KW, {RD_INT, RD_FLOAT}},
        {ZWEL_KW, {RD_CHAR, RD_INT}},       {ICON_KW, {RD_INT, RD_FLOAT}},
        {SCON_KW, {RD_DOUBLE, RD_CHAR}},    {XCON_KW, {RD_DOUBLE, RD_CHAR}},
        {ISEG_KW, {RD_INT, RD_FLOAT}},      {RSEG_KW, {RD_DOUBLE, RD_CHAR}},
        {XWEL_KW, {RD_DOUBLE, RD_INT}}};
    return specs;
}

enum class Mode { MISSING, WRONG_TYPE, EMPTY, SHORT };

void mutate(std::vector<NamedKw> &kws, const std::string &name, Mode mode) {
    auto it = std::find_if(kws.begin(), kws.end(),
                           [&](const NamedKw &nk) { return nk.name == name; });
    REQUIRE(it != kws.end());
    size_t size = it->kw->size();
    const Spec &spec = kw_specs().at(name);
    switch (mode) {
    case Mode::MISSING:
        it->kw.reset();
        break;
    case Mode::WRONG_TYPE:
        it->kw = std::make_unique<rd::KW>(name, size, spec.wrong);
        break;
    case Mode::EMPTY:
        it->kw = std::make_unique<rd::KW>(name, 0, spec.type);
        break;
    case Mode::SHORT:
        it->kw = std::make_unique<rd::KW>(name, 1, spec.type);
        break;
    }
}

const char *mode_name(Mode m) {
    switch (m) {
    case Mode::MISSING:
        return "MISSING";
    case Mode::WRONG_TYPE:
        return "WRONG_TYPE";
    case Mode::EMPTY:
        return "EMPTY";
    case Mode::SHORT:
        return "SHORT";
    }
    return "?";
}

struct Case {
    const char *kw;
    Mode mode;
    bool throws;
    size_t wells;
};

std::vector<Case> all_cases() {
    return {
        {INTEHEAD_KW, Mode::MISSING, true, 0},
        {INTEHEAD_KW, Mode::WRONG_TYPE, true, 0},
        {INTEHEAD_KW, Mode::EMPTY, false, 0},
        {INTEHEAD_KW, Mode::SHORT, false, 0},

        {DOUBHEAD_KW, Mode::MISSING, true, 0},
        {DOUBHEAD_KW, Mode::WRONG_TYPE, true, 0},
        {DOUBHEAD_KW, Mode::EMPTY, true, 0},
        {DOUBHEAD_KW, Mode::SHORT, false, 1},

        {LOGIHEAD_KW, Mode::MISSING, false, 1},
        {LOGIHEAD_KW, Mode::WRONG_TYPE, true, 0},
        {LOGIHEAD_KW, Mode::EMPTY, true, 0},
        {LOGIHEAD_KW, Mode::SHORT, true, 0},

        {IWEL_KW, Mode::MISSING, false, 0},
        {IWEL_KW, Mode::WRONG_TYPE, true, 0},
        {IWEL_KW, Mode::EMPTY, true, 0},
        {IWEL_KW, Mode::SHORT, true, 0},

        {ZWEL_KW, Mode::MISSING, true, 0},
        {ZWEL_KW, Mode::WRONG_TYPE, true, 0},
        {ZWEL_KW, Mode::EMPTY, true, 0},
        {ZWEL_KW, Mode::SHORT, false, 1},

        {ICON_KW, Mode::MISSING, false, 1},
        {ICON_KW, Mode::WRONG_TYPE, true, 0},
        {ICON_KW, Mode::EMPTY, true, 0},
        {ICON_KW, Mode::SHORT, false, 1},

        {SCON_KW, Mode::MISSING, false, 1},
        {SCON_KW, Mode::WRONG_TYPE, true, 0},
        {SCON_KW, Mode::EMPTY, true, 0},
        {SCON_KW, Mode::SHORT, false, 1},

        {XCON_KW, Mode::MISSING, false, 1},
        {XCON_KW, Mode::WRONG_TYPE, true, 0},
        {XCON_KW, Mode::EMPTY, true, 0},
        {XCON_KW, Mode::SHORT, true, 0},

        {ISEG_KW, Mode::MISSING, false, 1},
        {ISEG_KW, Mode::WRONG_TYPE, true, 0},
        {ISEG_KW, Mode::EMPTY, true, 0},
        {ISEG_KW, Mode::SHORT, true, 0},

        {RSEG_KW, Mode::MISSING, false, 1},
        {RSEG_KW, Mode::WRONG_TYPE, true, 0},
        {RSEG_KW, Mode::EMPTY, true, 0},
        {RSEG_KW, Mode::SHORT, true, 0},

        {XWEL_KW, Mode::MISSING, false, 1},
        {XWEL_KW, Mode::WRONG_TYPE, true, 0},
        {XWEL_KW, Mode::EMPTY, true, 0},
        {XWEL_KW, Mode::SHORT, true, 0},
    };
}

TEST_CASE_METHOD(Tmpdir, "well keyword failure conditions", "[well][wellkw]") {
    auto c = GENERATE(from_range(all_cases()));
    CAPTURE(c.kw, mode_name(c.mode));

    Dims d;
    auto grid =
        make_rectangular_grid(static_cast<int>(d.nx), static_cast<int>(d.ny),
                              static_cast<int>(d.nz), 1.0, 1.0, 1.0, nullptr);
    auto kws = build_all(d);
    mutate(kws, c.kw, c.mode);

    auto path = (dirname / "CASE.X0000").string();
    write_file(path, kws);

    WellInfo wi(grid.get());

    if (c.throws) {
        REQUIRE_THROWS_AS(wi.load_rstfile(path, true), std::exception);
    } else {
        REQUIRE_NOTHROW(wi.load_rstfile(path, true));
        REQUIRE(wi.num_wells() == c.wells);
    }
}

TEST_CASE_METHOD(Tmpdir, "baseline restart file loads one well",
                 "[well][wellkw]") {
    Dims d;
    auto kws = build_all(d);
    auto path = (dirname / "CASE.X0000").string();
    write_file(path, kws);

    auto grid = make_rectangular_grid(d.nx, d.ny, d.nz, 1.0, 1.0, 1.0, nullptr);
    WellInfo well_info(grid.get());

    REQUIRE_NOTHROW(well_info.load_rstfile(path.c_str(), true));
    REQUIRE(well_info.num_wells() == 1);
}
