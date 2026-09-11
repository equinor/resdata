#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <memory>
#include <new>
#include <sstream>
#include <stdexcept>
#include <string>

#include <resdata/FortIO.hpp>
#include <resdata/rd_file_kw.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/rd_type.hpp>
#include <utility>
#include <vector>

#include "resdata/rd_util.hpp"
#include "tmpdir.hpp"

using Catch::Matchers::ContainsSubstring;

namespace {

std::unique_ptr<rd::KW> make_int_kw(const std::string &name, size_t size) {
    std::vector<int> data(size);
    for (size_t i = 0; i < size; i++)
        data[i] = i;
    return std::make_unique<rd::KW>(name, std::move(data));
}

} // namespace

TEST_CASE("rd_kw_alloc rejects negative size", "[rd_kw]") {
    REQUIRE_THROWS_WITH(rd::KW("KW", -1, RD_INT),
                        ContainsSubstring("rd_kw size was negative: -1"));
}

TEST_CASE("rd::KW constructor rejects negative size", "[rd_kw]") {
    REQUIRE_THROWS_AS(rd::KW("KW", -1, RD_INT), std::invalid_argument);
}

TEST_CASE("typed accessors validate the index", "[rd_kw]") {
    auto kw = make_int_kw("KW", 3);
    SECTION("index too large") {
        REQUIRE_THROWS_WITH(kw->at<int>(5),
                            ContainsSubstring("Invalid index lookup"));
    }
    SECTION("negative index") {
        REQUIRE_THROWS_WITH(kw->at<int>(-1) = 0,
                            ContainsSubstring("Invalid index lookup"));
    }
}

TEST_CASE("typed accessors validate the type", "[rd_kw]") {
    rd::KW float_kw{"KW", 3, RD_FLOAT};

    SECTION("iget on wrong type") {
        REQUIRE_THROWS_WITH(float_kw.at<int>(0),
                            ContainsSubstring("wrong type"));
    }
    SECTION("iset on wrong type") {
        REQUIRE_THROWS_WITH(float_kw.at<int>(0) = 1,
                            ContainsSubstring("wrong type"));
    }
    SECTION("iget_as_double on non numeric type") {
        rd::KW bool_kw{"KW", 3, RD_BOOL};
        REQUIRE_THROWS_WITH(bool_kw.as_double(0),
                            ContainsSubstring("cannot be converted to double"));
    }
}

TEST_CASE("char/string accessors validate the type", "[rd_kw]") {
    auto int_kw = make_int_kw("KW", 3);
    SECTION("iget_char_ptr on non char type") {
        REQUIRE_THROWS_WITH(int_kw->at<std::string>(0),
                            ContainsSubstring("wrong type"));
    }
    SECTION("iget_string_ptr on non string type") {
        REQUIRE_THROWS_WITH(int_kw->at<std::string>(0),
                            ContainsSubstring("wrong type"));
    }
}

TEST_CASE("rd::pad_spaces fits strings to the requested width", "[rd_kw]") {
    SECTION("shorter strings are padded with trailing spaces") {
        REQUIRE(rd::pad_spaces("x", 8) == "x       ");
    }
    SECTION("longer strings are truncated") {
        REQUIRE(rd::pad_spaces("123456789", 8) == "12345678");
    }
}

TEST_CASE("rd::strip_spaces handles width edge cases", "[rd_kw]") {
    SECTION("RD_CHAR values can fill the full field width") {
        rd::KW char_kw{"KW", {"FOPRTEST", "BPR"}};

        REQUIRE(rd::strip_spaces(char_kw.at<std::string>(0)) == "FOPRTEST");
        REQUIRE(rd::strip_spaces(char_kw.at<std::string>(1)) == "BPR");
    }

    SECTION("RD_STRING values can fill the declared field width") {
        rd::KW string_kw{"KW", 1, RD_STRING(12)};

        string_kw.at<std::string>(0) = "0123456789AB";

        REQUIRE(rd::strip_spaces(string_kw.at<std::string>(0)) ==
                "0123456789AB");
    }
}

TEST_CASE("slice copy validates range and stride", "[rd_kw]") {
    auto src = make_int_kw("KW", 4);
    SECTION("index1 beyond size") {
        REQUIRE_THROWS_WITH(rd::KW(*src.get(), 10, 20, 1),
                            ContainsSubstring("> size"));
    }
    SECTION("non positive stride") {
        REQUIRE_THROWS_WITH(rd::KW(*src.get(), 0, 4, 0),
                            ContainsSubstring("must be positive"));
    }
}

TEST_CASE("sub copy constructor validates offset and count", "[rd_kw]") {
    auto src = make_int_kw("KW", 4);
    SECTION("invalid offset") {
        REQUIRE_THROWS_WITH(rd::KW(*src.get(), "NEW", 100, 1),
                            ContainsSubstring("invalid offset"));
    }
    SECTION("invalid count") {
        REQUIRE_THROWS_WITH(rd::KW(*src.get(), "NEW", 0, 100),
                            ContainsSubstring("invalid count value"));
    }
}

TEST_CASE("inplace binary ops validate size and type", "[rd_kw]") {
    auto a = make_int_kw("A", 3);
    auto b = make_int_kw("B", 4);
    rd::KW a_char{"A", 3, RD_CHAR};
    rd::KW b_char{"B", 3, RD_CHAR};

    SECTION("size mismatch") {
        REQUIRE_THROWS_WITH(*a.get() -= *b.get(),
                            ContainsSubstring("type/size"));
    }

    SECTION("type not implemented") {
        REQUIRE_THROWS_WITH(a_char -= b_char,
                            ContainsSubstring("not implemented for type"));
    }
}

TEST_CASE("rd_kw_first_different validates offset and size", "[rd_kw]") {
    auto a = make_int_kw("A", 3);
    auto b = make_int_kw("B", 4);
    auto c = make_int_kw("C", 3);

    SECTION("size mismatch") {
        REQUIRE_THROWS_WITH(a->first_different(b.get(), 0, 0, 0),
                            ContainsSubstring("sorry invalid comparison"));
    }
    SECTION("invalid offset") {
        REQUIRE_THROWS_WITH(
            a->first_different(c.get(), 5, 0, 0),
            ContainsSubstring(
                "offset value in first_difference exceeded size: 5"));
    }
}

TEST_CASE_METHOD(Tmpdir, "fread_alloc throws on corrupt data", "[rd_kw]") {
    auto good = (dirname / "GOOD.txt").string();
    {
        auto kw = make_int_kw("INTKW", 4);
        ERT::FortIO fortio(good, std::ios_base::out, /*fmt_file=*/true);
        kw->fwrite(fortio);
    }

    std::string contents;
    {
        std::ifstream in(good);
        std::stringstream ss;
        ss << in.rdbuf();
        contents = ss.str();
    }

    SECTION("non numeric data") {
        auto bad = (dirname / "BAD.txt").string();
        {
            std::string corrupt = contents;
            corrupt.replace(corrupt.find('0'), 1, "XYZ");
            std::ofstream out(bad);
            out << corrupt;
        }
        ERT::FortIO fortio(bad, std::ios_base::in, /*fmt_file=*/true);
        REQUIRE_THROWS_WITH(rd::KW::fread(fortio),
                            ContainsSubstring("reading of keyword:INTKW"));
    }
}

TEST_CASE_METHOD(Tmpdir, "fread_alloc rejects bad logical value", "[rd_kw]") {
    auto good = (dirname / "GOOD.txt").string();
    {
        rd::KW kw{"BKW", 3, RD_BOOL};
        for (size_t i = 0; i < 3; i++)
            kw.at<bool>(i) = true;
        ERT::FortIO fortio(good, std::ios_base::out, /*fmt_file=*/true);
        kw.fwrite(fortio);
    }

    std::string contents;
    {
        std::ifstream in(good);
        std::stringstream ss;
        ss << in.rdbuf();
        contents = ss.str();
    }
    auto bad = (dirname / "BAD.txt").string();
    {
        for (auto &ch : contents)
            if (ch == 'T')
                ch = 'Q';
        std::ofstream out(bad);
        out << contents;
    }

    ERT::FortIO fortio(bad, std::ios_base::in, /*fmt_file=*/true);
    REQUIRE_THROWS_WITH(rd::KW::fread(fortio),
                        ContainsSubstring("Logical value: [Q] not recogniced"));
}

TEST_CASE_METHOD(Tmpdir, "FileKW::read guards against buffer_size overflow",
                 "[rd_kw]") {
    auto path = (dirname / "FILE").string();
    {
        std::ofstream create(path, std::ios_base::binary);
        REQUIRE(create.good());
    }
    std::ifstream stream(path, std::ios_base::binary);
    REQUIRE(stream.good());
    REQUIRE_THROWS_AS(FileKW::read(stream, SIZE_MAX), std::length_error);
}
