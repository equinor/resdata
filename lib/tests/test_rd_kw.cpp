#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <new>
#include <sstream>
#include <stdexcept>
#include <string>

#include <ert/util/int_vector.hpp>

#include <resdata/FortIO.hpp>
#include <resdata/rd_file_kw.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/rd_type.hpp>

#include "tmpdir.hpp"

using Catch::Matchers::ContainsSubstring;

namespace {

/* Small RAII helper so the test bodies stay leak-free regardless of the
   assertion outcome. */
rd_kw_ptr make_int_kw(const char *name, int size) {
    auto kw = make_rd_kw(name, size, RD_INT);
    for (int i = 0; i < size; i++)
        rd_kw_iset_int(kw.get(), i, i);
    return kw;
}

} // namespace

TEST_CASE("rd_kw_alloc rejects negative size", "[rd_kw]") {
    REQUIRE_THROWS_WITH(rd_kw_alloc("KW", -1, RD_INT),
                        ContainsSubstring("rd_kw size was negative: -1"));
}

TEST_CASE("rd_kw_alloc_new rejects negative size", "[rd_kw]") {
    int data[1] = {0};
    REQUIRE_THROWS_AS(rd_kw_alloc_new("KW", -1, RD_INT, data),
                      std::invalid_argument);
}

TEST_CASE("rd_kw_resize rejects negative size", "[rd_kw]") {
    auto kw = make_int_kw("KW", 3);
    REQUIRE_THROWS_WITH(rd_kw_resize(kw.get(), -1),
                        ContainsSubstring("new_size was negative"));
}

TEST_CASE("typed accessors validate the index", "[rd_kw]") {
    auto kw = make_int_kw("KW", 3);
    SECTION("index too large") {
        REQUIRE_THROWS_WITH(rd_kw_iget_int(kw.get(), 5),
                            ContainsSubstring("Invalid index lookup"));
    }
    SECTION("negative index") {
        REQUIRE_THROWS_WITH(rd_kw_iset_int(kw.get(), -1, 0),
                            ContainsSubstring("Invalid index lookup"));
    }
}

TEST_CASE("typed accessors validate the type", "[rd_kw]") {
    auto float_kw = make_rd_kw("KW", 3, RD_FLOAT);

    SECTION("iget on wrong type") {
        REQUIRE_THROWS_WITH(rd_kw_iget_int(float_kw.get(), 0),
                            ContainsSubstring("wrong type"));
    }
    SECTION("iset on wrong type") {
        REQUIRE_THROWS_WITH(rd_kw_iset_int(float_kw.get(), 0, 1),
                            ContainsSubstring("wrong type"));
    }
    SECTION("iget_as_double on non numeric type") {
        auto bool_kw = make_rd_kw("KW", 3, RD_BOOL);
        REQUIRE_THROWS_WITH(
            rd_kw_iget_as_double(bool_kw.get(), 0),
            ContainsSubstring("can not be converted to double"));
    }
}

TEST_CASE("char/string accessors validate the type", "[rd_kw]") {
    auto int_kw = make_int_kw("KW", 3);
    SECTION("iget_char_ptr on non char type") {
        REQUIRE_THROWS_WITH(rd_kw_iget_char_ptr(int_kw.get(), 0),
                            ContainsSubstring("wrong type"));
    }
    SECTION("iget_string_ptr on non string type") {
        REQUIRE_THROWS_WITH(rd_kw_iget_string_ptr(int_kw.get(), 0),
                            ContainsSubstring("wrong type"));
    }
    SECTION("icmp_string on non char type") {
        REQUIRE_THROWS_WITH(rd_kw_icmp_string(int_kw.get(), 0, "X"),
                            ContainsSubstring("wrong type"));
    }
}

TEST_CASE("rd_kw_iset_string_ptr validates type and length", "[rd_kw]") {
    SECTION("non alphabetic type") {
        auto int_kw = make_int_kw("KW", 1);
        REQUIRE_THROWS_WITH(rd_kw_iset_string_ptr(int_kw.get(), 0, "x"),
                            ContainsSubstring("Expected alphabetic data type"));
    }
    SECTION("string too long") {
        auto str_kw = make_rd_kw("KW", 1, RD_STRING(8));
        REQUIRE_THROWS_WITH(
            rd_kw_iset_string_ptr(str_kw.get(), 0, "123456789"),
            ContainsSubstring("cannot hold input string of length 9"));
    }
}

TEST_CASE("rd_kw_iget_stripped_string handles width edge cases", "[rd_kw]") {
    SECTION("RD_CHAR values can fill the full field width") {
        auto char_kw = make_rd_kw("KW", 2, RD_CHAR);

        rd_kw_iset_char_ptr(char_kw.get(), 0, "FOPRTEST");
        rd_kw_iset_char_ptr(char_kw.get(), 1, "BPR");

        REQUIRE(rd_kw_iget_stripped_string(char_kw.get(), 0) == "FOPRTEST");
        REQUIRE(rd_kw_iget_stripped_string(char_kw.get(), 1) == "BPR");
    }

    SECTION("RD_STRING values can fill the declared field width") {
        auto string_kw = make_rd_kw("KW", 1, RD_STRING(12));

        rd_kw_iset_string_ptr(string_kw.get(), 0, "0123456789AB");

        REQUIRE(rd_kw_iget_stripped_string(string_kw.get(), 0) ==
                "0123456789AB");
    }

    SECTION("embedded NUL stops the extracted string before field width") {
        auto string_kw = make_rd_kw("KW", 1, RD_STRING(12));
        char *raw = static_cast<char *>(rd_kw_iget_ptr(string_kw.get(), 0));

        std::memcpy(raw, "ABCD\0EFGHIJK", 12);
        raw[12] = '\0';

        REQUIRE(rd_kw_iget_stripped_string(string_kw.get(), 0) == "ABCD");
    }
}

TEST_CASE("scalar_set/scale/shift validate the type", "[rd_kw]") {
    auto float_kw = make_rd_kw("KW", 3, RD_FLOAT);
    REQUIRE_THROWS_WITH(rd_kw_scalar_set_int(float_kw.get(), 1),
                        ContainsSubstring("wrong type"));
    REQUIRE_THROWS_WITH(rd_kw_scale_int(float_kw.get(), 1),
                        ContainsSubstring("wrong type"));
    REQUIRE_THROWS_WITH(rd_kw_shift_int(float_kw.get(), 1),
                        ContainsSubstring("wrong type"));

    auto int_kw = make_int_kw("KW", 3);
    REQUIRE_THROWS_WITH(rd_kw_scalar_set_float_or_double(int_kw.get(), 1.0),
                        ContainsSubstring("wrong type"));
    REQUIRE_THROWS_WITH(rd_kw_scale_float_or_double(int_kw.get(), 1.0),
                        ContainsSubstring("wrong type"));
    REQUIRE_THROWS_WITH(rd_kw_shift_float_or_double(int_kw.get(), 1.0),
                        ContainsSubstring("wrong type"));
}

TEST_CASE("rd_kw_alloc_slice_copy validates range and stride", "[rd_kw]") {
    auto src = make_int_kw("KW", 4);
    SECTION("index1 beyond size") {
        REQUIRE_THROWS_WITH(rd_kw_alloc_slice_copy(src.get(), 10, 20, 1),
                            ContainsSubstring("> size"));
    }
    SECTION("non positive stride") {
        REQUIRE_THROWS_WITH(rd_kw_alloc_slice_copy(src.get(), 0, 4, 0),
                            ContainsSubstring("completely broken"));
    }
}

TEST_CASE("rd_kw_alloc_sub_copy validates offset and count", "[rd_kw]") {
    auto src = make_int_kw("KW", 4);
    SECTION("invalid offset") {
        REQUIRE_THROWS_WITH(rd_kw_alloc_sub_copy(src.get(), "NEW", -1, 1),
                            ContainsSubstring("invalid offset"));
    }
    SECTION("invalid count") {
        REQUIRE_THROWS_WITH(rd_kw_alloc_sub_copy(src.get(), "NEW", 0, 100),
                            ContainsSubstring("invalid count value"));
    }
}

TEST_CASE("rd_kw_alloc_scatter_copy rejects unsupported type", "[rd_kw]") {
    auto src = make_rd_kw("KW", 1, RD_MESS);
    int mapping[1] = {0};
    REQUIRE_THROWS_WITH(
        rd_kw_alloc_scatter_copy(src.get(), 1, mapping, nullptr),
        ContainsSubstring("unsupported type"));
}

TEST_CASE("inplace binary ops validate size and type", "[rd_kw]") {
    auto a = make_int_kw("A", 3);
    auto b = make_int_kw("B", 4);
    auto a_char = make_rd_kw("A", 3, RD_CHAR);
    auto b_char = make_rd_kw("B", 3, RD_CHAR);

    SECTION("size mismatch") {
        REQUIRE_THROWS_WITH(rd_kw_inplace_add(a.get(), b.get()),
                            ContainsSubstring("type/size"));
        REQUIRE_THROWS_WITH(rd_kw_inplace_sub(a.get(), b.get()),
                            ContainsSubstring("type/size"));
        REQUIRE_THROWS_WITH(rd_kw_inplace_mul(a.get(), b.get()),
                            ContainsSubstring("type/size"));
        REQUIRE_THROWS_WITH(rd_kw_inplace_div(a.get(), b.get()),
                            ContainsSubstring("type/size"));
        REQUIRE_THROWS_WITH(rd_kw_inplace_add_squared(a.get(), b.get()),
                            ContainsSubstring("type/size"));
    }

    SECTION("type not implemented") {
        REQUIRE_THROWS_WITH(rd_kw_inplace_add(a_char.get(), b_char.get()),
                            ContainsSubstring("not implemented for type"));
        REQUIRE_THROWS_WITH(rd_kw_inplace_sub(a_char.get(), b_char.get()),
                            ContainsSubstring("not implemented for type"));
        REQUIRE_THROWS_WITH(rd_kw_inplace_mul(a_char.get(), b_char.get()),
                            ContainsSubstring("not implemented for type"));
        REQUIRE_THROWS_WITH(rd_kw_inplace_div(a_char.get(), b_char.get()),
                            ContainsSubstring("not implemented for type"));
    }
}

TEST_CASE("inplace unary ops validate type", "[rd_kw]") {
    auto char_kw = make_rd_kw("KW", 3, RD_CHAR);
    REQUIRE_THROWS_WITH(rd_kw_inplace_abs(char_kw.get()),
                        ContainsSubstring("inplace abs not implemented"));
    REQUIRE_THROWS_WITH(rd_kw_inplace_sqrt(char_kw.get()),
                        ContainsSubstring("inplace sqrt not implemented"));
}

TEST_CASE("indexed inplace/copy ops validate size and type", "[rd_kw]") {
    int_vector_type *index_set = int_vector_alloc(0, 0);
    int_vector_append(index_set, 0);

    auto a = make_int_kw("A", 3);
    auto b = make_int_kw("B", 4);

    REQUIRE_THROWS_WITH(rd_kw_copy_indexed(a.get(), index_set, b.get()),
                        ContainsSubstring("type/size"));
    REQUIRE_THROWS_WITH(rd_kw_inplace_add_indexed(a.get(), index_set, b.get()),
                        ContainsSubstring("type/size"));
    REQUIRE_THROWS_WITH(rd_kw_inplace_sub_indexed(a.get(), index_set, b.get()),
                        ContainsSubstring("type/size"));
    REQUIRE_THROWS_WITH(rd_kw_inplace_mul_indexed(a.get(), index_set, b.get()),
                        ContainsSubstring("type/size"));
    REQUIRE_THROWS_WITH(rd_kw_inplace_div_indexed(a.get(), index_set, b.get()),
                        ContainsSubstring("type/size"));

    int_vector_free(index_set);
}

TEST_CASE("rd_kw_max_min validates type", "[rd_kw]") {
    auto char_kw = make_rd_kw("KW", 3, RD_CHAR);
    char max[8];
    char min[8];
    REQUIRE_THROWS_WITH(rd_kw_max_min(char_kw.get(), max, min),
                        ContainsSubstring("invalid type for element sum"));
}

TEST_CASE("element sum validates type", "[rd_kw]") {
    auto char_kw = make_rd_kw("KW", 3, RD_CHAR);
    char sum[8];
    REQUIRE_THROWS_WITH(rd_kw_element_sum(char_kw.get(), sum),
                        ContainsSubstring("invalid type for element sum"));

    auto int_kw = make_int_kw("KW", 3);
    REQUIRE_THROWS_WITH(rd_kw_element_sum_float(int_kw.get()),
                        ContainsSubstring("invalid type"));

    int_vector_type *index_set = int_vector_alloc(0, 0);
    int_vector_append(index_set, 0);
    REQUIRE_THROWS_WITH(
        rd_kw_element_sum_indexed(char_kw.get(), index_set, sum),
        ContainsSubstring("invalid type for element sum"));
    int_vector_free(index_set);
}

TEST_CASE("rd_kw_first_different validates offset and size", "[rd_kw]") {
    auto a = make_int_kw("A", 3);
    auto b = make_int_kw("B", 4);
    auto c = make_int_kw("C", 3);

    SECTION("size mismatch") {
        REQUIRE_THROWS_WITH(rd_kw_first_different(a.get(), b.get(), 0, 0, 0),
                            ContainsSubstring("sorry invalid comparison"));
    }
    SECTION("invalid offset") {
        REQUIRE_THROWS_WITH(rd_kw_first_different(a.get(), c.get(), 5, 0, 0),
                            ContainsSubstring("invalid offset value"));
    }
}

TEST_CASE_METHOD(Tmpdir, "fread_alloc throws on corrupt data", "[rd_kw]") {
    auto good = (dirname / "GOOD.txt").string();
    {
        auto kw = make_int_kw("INTKW", 4);
        ERT::FortIO fortio(good, std::ios_base::out, /*fmt_file=*/true);
        rd_kw_fwrite(kw.get(), fortio);
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
        REQUIRE_THROWS_WITH(rd_kw_fread_alloc(fortio),
                            ContainsSubstring("reading of keyword:INTKW"));
    }
}

TEST_CASE_METHOD(Tmpdir, "fread_alloc rejects bad logical value", "[rd_kw]") {
    auto good = (dirname / "GOOD.txt").string();
    {
        auto kw = make_rd_kw("BKW", 3, RD_BOOL);
        for (int i = 0; i < 3; i++)
            rd_kw_iset_bool(kw.get(), i, true);
        ERT::FortIO fortio(good, std::ios_base::out, /*fmt_file=*/true);
        rd_kw_fwrite(kw.get(), fortio);
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
    REQUIRE_THROWS_WITH(rd_kw_fread_alloc(fortio),
                        ContainsSubstring("Logical value: [Q] not recogniced"));
}

TEST_CASE_METHOD(Tmpdir, "fseek_kw throws on missing keyword", "[rd_kw]") {
    auto path = (dirname / "FILE").string();
    {
        auto kw = make_int_kw("INTKW", 4);
        ERT::FortIO fortio(path, std::ios_base::out);
        rd_kw_fwrite(kw.get(), fortio);
    }

    ERT::FortIO fortio(path, std::ios_base::in);
    REQUIRE_THROWS_WITH(rd_kw_fseek_kw("MISSING", /*rewind=*/false,
                                       /*abort_on_error=*/true, fortio),
                        ContainsSubstring("failed to locate keyword:MISSING"));
}

TEST_CASE_METHOD(Tmpdir, "FileKW::read guards against buffer_size overflow",
                 "[rd_kw]") {
    auto path = (dirname / "FILE").string();
    std::FILE *stream = std::fopen(path.c_str(), "wb+");
    REQUIRE(stream != nullptr);
    REQUIRE_THROWS_AS(FileKW::read(stream, SIZE_MAX), std::bad_alloc);
    std::fclose(stream);
}
