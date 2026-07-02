#include <catch2/catch_test_macros.hpp>

#include <fstream>
#include <ios>
#include <memory>
#include <stdexcept>

#include <resdata/FortIO.hpp>
#include <resdata/rd_file_kw.hpp>
#include <resdata/rd_util.hpp>

#include "tmpdir.hpp"

TEST_CASE_METHOD(Tmpdir,
                 "rd_file_kw_inplace_fwrite rejects an unloaded keyword") {
    std::unique_ptr<rd_file_kw_type, decltype(&rd_file_kw_free)> file_kw(
        rd_file_kw_alloc0("TEST_KW", RD_INT, 10, 0), &rd_file_kw_free);

    auto filename = (dirname / "dummy").string();
    {
        std::ofstream ofs(filename);
        REQUIRE(ofs);
    }

    ERT::FortIO fortio(filename, std::ios_base::in, false, true);

    REQUIRE_THROWS_AS(rd_file_kw_inplace_fwrite(file_kw.get(), fortio),
                      std::invalid_argument);
}
