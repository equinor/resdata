#include <algorithm>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
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
    int nx = 3;
    int ny = 3;
    int nz = 3;
    int nactive = 27;
    int nwells = 1;
    int ncwmax = 1;
    int niwelz = 72;
    int nzwelz = 3;
    int nxwelz = 8;
    int niconz = 16;
    int nsconz = 2;
    int nxconz = 52;
    int nisegz = 8;
    int nsegmx = 1;
    int nrsegz = 12;
    int nlbrmx = 1;
    int nilbrz = 2;
};

rd_kw_ptr build_intehead(const Dims &d) {
    auto kw = make_rd_kw(INTEHEAD_KW, 412, RD_INT);
    rd_kw_scalar_set_int(kw.get(), 0);
    rd_kw_iset_int(kw.get(), INTEHEAD_UNIT_INDEX, INTEHEAD_METRIC_VALUE);
    rd_kw_iset_int(kw.get(), INTEHEAD_NX_INDEX, d.nx);
    rd_kw_iset_int(kw.get(), INTEHEAD_NY_INDEX, d.ny);
    rd_kw_iset_int(kw.get(), INTEHEAD_NZ_INDEX, d.nz);
    rd_kw_iset_int(kw.get(), INTEHEAD_NACTIVE_INDEX, d.nactive);
    rd_kw_iset_int(kw.get(), INTEHEAD_NWELLS_INDEX, d.nwells);
    rd_kw_iset_int(kw.get(), INTEHEAD_NCWMAX_INDEX, d.ncwmax);
    rd_kw_iset_int(kw.get(), INTEHEAD_NIWELZ_INDEX, d.niwelz);
    rd_kw_iset_int(kw.get(), INTEHEAD_NXWELZ_INDEX, d.nxwelz);
    rd_kw_iset_int(kw.get(), INTEHEAD_NZWELZ_INDEX, d.nzwelz);
    rd_kw_iset_int(kw.get(), INTEHEAD_NICONZ_INDEX, d.niconz);
    rd_kw_iset_int(kw.get(), INTEHEAD_NSCONZ_INDEX, d.nsconz);
    rd_kw_iset_int(kw.get(), INTEHEAD_NXCONZ_INDEX, d.nxconz);
    rd_kw_iset_int(kw.get(), INTEHEAD_NISEGZ_INDEX, d.nisegz);
    rd_kw_iset_int(kw.get(), INTEHEAD_NSEGMX_INDEX, d.nsegmx);
    rd_kw_iset_int(kw.get(), INTEHEAD_NSWLMX_INDEX, 1);
    rd_kw_iset_int(kw.get(), INTEHEAD_NLBRMX_INDEX, d.nlbrmx);
    rd_kw_iset_int(kw.get(), INTEHEAD_NILBRZ_INDEX, d.nilbrz);
    rd_kw_iset_int(kw.get(), INTEHEAD_NRSEGZ_INDEX, d.nrsegz);
    return kw;
}

rd_kw_ptr build_logihead() {
    auto kw = make_rd_kw(LOGIHEAD_KW, LOGIHEAD_RESTART_SIZE, RD_BOOL);
    for (int i = 0; i < LOGIHEAD_RESTART_SIZE; ++i)
        rd_kw_iset_bool(kw.get(), i, false);
    return kw;
}

rd_kw_ptr build_doubhead() {
    auto kw = make_rd_kw(DOUBHEAD_KW, 1, RD_DOUBLE);
    rd_kw_iset_double(kw.get(), DOUBHEAD_DAYS_INDEX, 0.0);
    return kw;
}

rd_kw_ptr build_iwel(const Dims &d) {
    auto kw = make_rd_kw(IWEL_KW, d.niwelz * d.nwells, RD_INT);
    rd_kw_scalar_set_int(kw.get(), 0);
    rd_kw_iset_int(kw.get(), IWEL_HEADI_INDEX, 1);
    rd_kw_iset_int(kw.get(), IWEL_HEADJ_INDEX, 1);
    rd_kw_iset_int(kw.get(), IWEL_HEADK_INDEX, 1);
    rd_kw_iset_int(kw.get(), IWEL_CONNECTIONS_INDEX, 1);
    rd_kw_iset_int(kw.get(), IWEL_TYPE_INDEX, IWEL_PRODUCER);
    rd_kw_iset_int(kw.get(), IWEL_STATUS_INDEX, 1);
    rd_kw_iset_int(kw.get(), IWEL_SEGMENTED_WELL_NR_INDEX, 1);
    return kw;
}

rd_kw_ptr build_zwel(const Dims &d) {
    auto kw = make_rd_kw(ZWEL_KW, d.nzwelz * d.nwells, RD_CHAR);
    rd_kw_iset_string_ptr(kw.get(), 0, "WELL-1");
    return kw;
}

rd_kw_ptr build_icon(const Dims &d) {
    auto kw = make_rd_kw(ICON_KW, d.niconz * d.ncwmax * d.nwells, RD_INT);
    rd_kw_scalar_set_int(kw.get(), 0);
    rd_kw_iset_int(kw.get(), ICON_IC_INDEX, 1);
    rd_kw_iset_int(kw.get(), ICON_I_INDEX, 1);
    rd_kw_iset_int(kw.get(), ICON_J_INDEX, 1);
    rd_kw_iset_int(kw.get(), ICON_K_INDEX, 1);
    rd_kw_iset_int(kw.get(), ICON_STATUS_INDEX, 1);
    rd_kw_iset_int(kw.get(), ICON_DIRECTION_INDEX, ICON_DIRZ);
    rd_kw_iset_int(kw.get(), ICON_SEGMENT_INDEX, 0);
    return kw;
}

rd_kw_ptr build_scon(const Dims &d) {
    auto kw = make_rd_kw(SCON_KW, d.nsconz * d.ncwmax * d.nwells, RD_DOUBLE);
    rd_kw_scalar_set_double(kw.get(), 0.0);
    rd_kw_iset_double(kw.get(), SCON_CF_INDEX, 1.0);
    return kw;
}

rd_kw_ptr build_xcon(const Dims &d) {
    auto kw = make_rd_kw(XCON_KW, d.nxconz * d.ncwmax * d.nwells, RD_DOUBLE);
    rd_kw_scalar_set_double(kw.get(), 0.0);
    return kw;
}

rd_kw_ptr build_iseg(const Dims &d) {
    auto kw = make_rd_kw(ISEG_KW, d.nisegz * d.nsegmx, RD_INT);
    rd_kw_scalar_set_int(kw.get(), 0);
    // Make the single segment inactive (branch -> INACTIVE)
    rd_kw_iset_int(kw.get(), ISEG_OUTLET_INDEX, 0);
    rd_kw_iset_int(kw.get(), ISEG_BRANCH_INDEX, -1);
    return kw;
}

rd_kw_ptr build_rseg(const Dims &d) {
    auto kw = make_rd_kw(RSEG_KW, d.nrsegz * d.nsegmx, RD_DOUBLE);
    rd_kw_scalar_set_double(kw.get(), 0.0);
    return kw;
}

rd_kw_ptr build_xwel(const Dims &d) {
    auto kw = make_rd_kw(XWEL_KW, d.nxwelz * d.nwells, RD_DOUBLE);
    rd_kw_scalar_set_double(kw.get(), 0.0);
    return kw;
}

struct NamedKw {
    std::string name;
    rd_kw_ptr kw;
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
            rd_kw_fwrite(nk.kw.get(), fortio);
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
    int size = rd_kw_get_size(it->kw.get());
    const Spec &spec = kw_specs().at(name);
    switch (mode) {
    case Mode::MISSING:
        it->kw.reset();
        break;
    case Mode::WRONG_TYPE:
        it->kw = make_rd_kw(name.c_str(), size, spec.wrong);
        break;
    case Mode::EMPTY:
        it->kw = make_rd_kw(name.c_str(), 0, spec.type);
        break;
    case Mode::SHORT:
        it->kw = make_rd_kw(name.c_str(), 1, spec.type);
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
    int wells;
};

std::vector<Case> all_cases() {
    return {
        {INTEHEAD_KW, Mode::MISSING, true, 0},
        {INTEHEAD_KW, Mode::WRONG_TYPE, false, 0}, // Should throw but doesn't
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
        {ZWEL_KW, Mode::WRONG_TYPE, false, 1}, // Should throw but doesn't
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
        {RSEG_KW, Mode::WRONG_TYPE, false, 1}, // Should throw but doesn't
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
    auto grid = make_rectangular_grid(d.nx, d.ny, d.nz, 1.0, 1.0, 1.0, nullptr);
    auto kws = build_all(d);
    mutate(kws, c.kw, c.mode);

    auto path = (dirname / "CASE.X0000").string();
    write_file(path, kws);

    std::unique_ptr<well_info_type, decltype(&well_info_free)> wi(
        well_info_alloc(grid.get()), well_info_free);

    if (c.throws) {
        REQUIRE_THROWS_AS(well_info_load_rstfile(wi.get(), path.c_str(), true),
                          std::exception);
    } else {
        REQUIRE_NOTHROW(well_info_load_rstfile(wi.get(), path.c_str(), true));
        REQUIRE(well_info_get_num_wells(wi.get()) == c.wells);
    }
}

TEST_CASE_METHOD(Tmpdir, "baseline restart file loads one well",
                 "[well][wellkw]") {
    Dims d;
    auto kws = build_all(d);
    auto path = (dirname / "CASE.X0000").string();
    write_file(path, kws);

    auto grid = make_rectangular_grid(d.nx, d.ny, d.nz, 1.0, 1.0, 1.0, nullptr);
    std::unique_ptr<well_info_type, decltype(&well_info_free)> well_info(
        well_info_alloc(grid.get()), well_info_free);

    REQUIRE_NOTHROW(
        well_info_load_rstfile(well_info.get(), path.c_str(), true));
    REQUIRE(well_info_get_num_wells(well_info.get()) == 1);
}
