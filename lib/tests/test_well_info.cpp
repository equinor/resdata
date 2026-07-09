#include <cstddef>
#include <filesystem>
#include <ios>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <resdata/FortIO.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/rd_file_kw.hpp>
#include <resdata/rd_file_view.hpp>
#include <resdata/rd_grid.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/rd_kw_magic.hpp>
#include <resdata/rd_type.hpp>
#include <resdata/well/well_info.hpp>

#include "grid_fixtures.hpp"
#include "tmpdir.hpp"

using Catch::Matchers::ContainsSubstring;

namespace {

void write_int_kw(ERT::FortIO &fortio, const char *name,
                  const std::vector<int> &data) {
    auto kw = make_rd_kw(name, static_cast<int>(data.size()), RD_INT);
    for (size_t i = 0; i < data.size(); ++i)
        rd_kw_iset_int(kw.get(), static_cast<int>(i), data[i]);
    rd_kw_fwrite(kw.get(), fortio);
}

void write_double_kw(ERT::FortIO &fortio, const char *name,
                     const std::vector<double> &data) {
    auto kw = make_rd_kw(name, static_cast<int>(data.size()), RD_DOUBLE);
    for (size_t i = 0; i < data.size(); ++i)
        rd_kw_iset_double(kw.get(), static_cast<int>(i), data[i]);
    rd_kw_fwrite(kw.get(), fortio);
}

struct RestartLayout {
    bool unified = false;
    bool with_iwel = true;
    bool with_zwel = true;
    int nx = 3;
    int ny = 3;
    int nz = 3;
    int nwells = 1;
    int niwelz = 8;
    int nzwelz = 3;
    int report_nr = 0;
};

void write_restart_file(const std::string &path, const RestartLayout &layout) {
    ERT::FortIO fortio(path, std::ios_base::out);

    if (layout.unified)
        write_int_kw(fortio, SEQNUM_KW, {layout.report_nr});

    std::vector<int> intehead(412, 0);
    intehead[INTEHEAD_NX_INDEX] = layout.nx;
    intehead[INTEHEAD_NY_INDEX] = layout.ny;
    intehead[INTEHEAD_NZ_INDEX] = layout.nz;
    intehead[INTEHEAD_NACTIVE_INDEX] = layout.nx * layout.ny * layout.nz;
    intehead[INTEHEAD_NWELLS_INDEX] = layout.nwells;
    intehead[INTEHEAD_NCWMAX_INDEX] = 1;
    intehead[INTEHEAD_NIWELZ_INDEX] = layout.niwelz;
    intehead[INTEHEAD_NZWELZ_INDEX] = layout.nzwelz;
    write_int_kw(fortio, INTEHEAD_KW, intehead);

    write_double_kw(fortio, DOUBHEAD_KW, {0.0});

    if (layout.with_iwel)
        write_int_kw(
            fortio, IWEL_KW,
            std::vector<int>(static_cast<size_t>(layout.niwelz) * layout.nwells,
                             0));

    if (layout.with_zwel) {
        auto zwel = make_rd_kw(ZWEL_KW, layout.nzwelz * layout.nwells, RD_CHAR);
        rd_kw_iset_string_ptr(zwel.get(), 0, "WELL-1");
        rd_kw_fwrite(zwel.get(), fortio);
    }

    fortio.fflush();
}

} // namespace

TEST_CASE_METHOD(Tmpdir,
                 "restart file without IWEL loads zero wells (non-unified)",
                 "[well][well_info]") {
    auto grid = make_rectangular_grid(3, 3, 3, 1.0, 1.0, 1.0, nullptr);
    auto path = (dirname / "CASE.X0000").string();

    RestartLayout layout;
    layout.with_iwel = false;
    layout.with_zwel = false;
    write_restart_file(path, layout);

    WellInfo well_info(grid.get());
    REQUIRE_NOTHROW(well_info.load_rstfile(path.c_str(), false));
    REQUIRE(well_info.num_wells() == 0);
}

TEST_CASE_METHOD(Tmpdir, "restart file without IWEL loads zero wells (unified)",
                 "[well][well_info]") {
    auto grid = make_rectangular_grid(3, 3, 3, 1.0, 1.0, 1.0, nullptr);
    auto path = (dirname / "CASE.UNRST").string();

    RestartLayout layout;
    layout.unified = true;
    layout.with_iwel = false;
    layout.with_zwel = false;
    write_restart_file(path, layout);

    WellInfo well_info(grid.get());
    REQUIRE_NOTHROW(well_info.load_rstfile(path, false));
    REQUIRE(well_info.num_wells() == 0);
}

TEST_CASE_METHOD(Tmpdir,
                 "restart file with IWEL but without ZWEL throws (non-unified)",
                 "[well][well_info]") {
    auto grid = make_rectangular_grid(3, 3, 3, 1.0, 1.0, 1.0, nullptr);
    auto path = (dirname / "CASE.X0000").string();

    RestartLayout layout;
    layout.with_iwel = true;
    layout.with_zwel = false;
    write_restart_file(path, layout);

    WellInfo well_info(grid.get());
    REQUIRE_THROWS_AS(well_info.load_rstfile(path.c_str(), false),
                      std::out_of_range);
}

TEST_CASE_METHOD(Tmpdir,
                 "restart file with IWEL but without ZWEL throws (unified)",
                 "[well][well_info]") {
    auto grid = make_rectangular_grid(3, 3, 3, 1.0, 1.0, 1.0, nullptr);
    auto path = (dirname / "CASE.UNRST").string();

    RestartLayout layout;
    layout.unified = true;
    layout.with_iwel = true;
    layout.with_zwel = false;
    write_restart_file(path, layout);

    WellInfo well_info(grid.get());
    REQUIRE_THROWS_AS(well_info.load_rstfile(path, false), std::out_of_range);
}

TEST_CASE_METHOD(Tmpdir, "loading an unknown file type throws",
                 "[well][well_info]") {
    auto grid = make_rectangular_grid(3, 3, 3, 1.0, 1.0, 1.0, nullptr);
    auto path = (dirname / "CASE.TXT").string();
    {
        ERT::FortIO fortio(path, std::ios_base::out);
        write_int_kw(fortio, INTEHEAD_KW, std::vector<int>(412, 0));
        fortio.fflush();
    }

    WellInfo well_info(grid.get());
    REQUIRE_THROWS_AS(well_info.load_rstfile(path, false),
                      std::invalid_argument);
}

namespace {

std::string write_three_kw_file(const fs::path &dirname) {
    auto path = (dirname / "data_file").string();
    ERT::FortIO fortio(path, std::ios_base::out);
    write_int_kw(fortio, "TEST1_KW", {537, 538, 539, 540, 541});
    write_int_kw(fortio, "TEST2_KW", {0, 1, 2});
    write_int_kw(fortio, "TEST3_KW", {7, 8});
    fortio.fflush();
    return path;
}

bool is_loaded(std::shared_ptr<FileKW> file_kw) {
    return file_kw->get_kw_ptr() != nullptr;
}

} // namespace

TEST_CASE_METHOD(Tmpdir, "keywords are lazily loaded", "[well][transaction]") {
    auto path = write_three_kw_file(dirname);
    rd_file_ptr file = open_rd_file(path);
    auto *view = rd_file_get_global_view(file.get());

    auto fk0 = rd_file_view_iget_file_kw(view, 0);
    auto fk1 = rd_file_view_iget_file_kw(view, 1);
    auto fk2 = rd_file_view_iget_file_kw(view, 2);

    REQUIRE_FALSE(is_loaded(fk0));
    REQUIRE_FALSE(is_loaded(fk1));
    REQUIRE_FALSE(is_loaded(fk2));

    rd_file_view_iget_kw(view, 0);

    REQUIRE(is_loaded(fk0));
    REQUIRE_FALSE(is_loaded(fk1));
    REQUIRE_FALSE(is_loaded(fk2));
}
