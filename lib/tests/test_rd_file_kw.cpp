#include <catch2/catch_test_macros.hpp>

#include <fstream>
#include <ios>
#include <memory>
#include <stdexcept>

#include <resdata/FortIO.hpp>
#include <resdata/rd_file_kw.hpp>
#include <resdata/rd_util.hpp>

#include "resdata/rd_type.hpp"
#include "tmpdir.hpp"

TEST_CASE_METHOD(Tmpdir,
                 "rd_file_kw_inplace_fwrite rejects an unloaded keyword") {
    FileKW file_kw(0, RD_INT, 10, "TEST_KW");

    auto filename = (dirname / "dummy").string();
    {
        std::ofstream ofs(filename);
        REQUIRE(ofs);
    }

    ERT::FortIO fortio(filename, std::ios_base::in, false, true);

    REQUIRE_THROWS_AS(file_kw.inplace_write(fortio), std::runtime_error);
}
