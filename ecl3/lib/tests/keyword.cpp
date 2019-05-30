#include <cstdio>

#include <catch2/catch.hpp>

#include <ecl3/keyword.h>

TEST_CASE("'INTE' gets correct type") {
    int type;
    const auto err = ecl3_keyword_type("INTE", &type);
    CHECK(err == ECL3_OK);
    CHECK(type == ECL3_INTE);
}

TEST_CASE("'REAL' gets correct type") {
    int type;
    const auto err = ecl3_keyword_type("REAL", &type);
    CHECK(err == ECL3_OK);
    CHECK(type == ECL3_REAL);
}

TEST_CASE("'DOUB' gets correct type") {
    int type;
    const auto err = ecl3_keyword_type("DOUB", &type);
    CHECK(err == ECL3_OK);
    CHECK(type == ECL3_DOUB);
}

TEST_CASE("'CHAR' gets correct type") {
    int type;
    const auto err = ecl3_keyword_type("CHAR", &type);
    CHECK(err == ECL3_OK);
    CHECK(type == ECL3_CHAR);
}

TEST_CASE("'MESS' gets correct type") {
    int type;
    const auto err = ecl3_keyword_type("MESS", &type);
    CHECK(err == ECL3_OK);
    CHECK(type == ECL3_MESS);
}

TEST_CASE("'LOGI' gets correct type") {
    int type;
    const auto err = ecl3_keyword_type("LOGI", &type);
    CHECK(err == ECL3_OK);
    CHECK(type == ECL3_LOGI);
}

TEST_CASE("'X231' gets correct type") {
    int type;
    const auto err = ecl3_keyword_type("X231", &type);
    CHECK(err == ECL3_OK);
    CHECK(type == ECL3_X231);
}

using namespace Catch::Generators;

static const ecl3_keyword_types C0NNs[] = {
    ECL3_C001,
    ECL3_C002,
    ECL3_C003,
    ECL3_C004,
    ECL3_C005,
    ECL3_C006,
    ECL3_C007,
    ECL3_C008,
    ECL3_C009,
    ECL3_C010,
    ECL3_C011,
    ECL3_C012,
    ECL3_C013,
    ECL3_C014,
    ECL3_C015,
    ECL3_C016,
    ECL3_C017,
    ECL3_C018,
    ECL3_C019,
    ECL3_C020,
    ECL3_C021,
    ECL3_C022,
    ECL3_C023,
    ECL3_C024,
    ECL3_C025,
    ECL3_C026,
    ECL3_C027,
    ECL3_C028,
    ECL3_C029,
    ECL3_C030,
    ECL3_C031,
    ECL3_C032,
    ECL3_C033,
    ECL3_C034,
    ECL3_C035,
    ECL3_C036,
    ECL3_C037,
    ECL3_C038,
    ECL3_C039,
    ECL3_C040,
    ECL3_C041,
    ECL3_C042,
    ECL3_C043,
    ECL3_C044,
    ECL3_C045,
    ECL3_C046,
    ECL3_C047,
    ECL3_C048,
    ECL3_C049,
    ECL3_C050,
    ECL3_C051,
    ECL3_C052,
    ECL3_C053,
    ECL3_C054,
    ECL3_C055,
    ECL3_C056,
    ECL3_C057,
    ECL3_C058,
    ECL3_C059,
    ECL3_C060,
    ECL3_C061,
    ECL3_C062,
    ECL3_C063,
    ECL3_C064,
    ECL3_C065,
    ECL3_C066,
    ECL3_C067,
    ECL3_C068,
    ECL3_C069,
    ECL3_C070,
    ECL3_C071,
    ECL3_C072,
    ECL3_C073,
    ECL3_C074,
    ECL3_C075,
    ECL3_C076,
    ECL3_C077,
    ECL3_C078,
    ECL3_C079,
    ECL3_C080,
    ECL3_C081,
    ECL3_C082,
    ECL3_C083,
    ECL3_C084,
    ECL3_C085,
    ECL3_C086,
    ECL3_C087,
    ECL3_C088,
    ECL3_C089,
    ECL3_C090,
    ECL3_C091,
    ECL3_C092,
    ECL3_C093,
    ECL3_C094,
    ECL3_C095,
    ECL3_C096,
    ECL3_C097,
    ECL3_C098,
    ECL3_C099,
};

TEST_CASE("'C0NN' gets correct type") {
    static_assert(
        sizeof(C0NNs) == sizeof(ecl3_keyword_types) * 99,
        "'C0NNs' should contain 99 elements"
    );

    for (auto nn = 1; nn < 100; ++nn) {
        int type;
        char str[5] = {};
        std::sprintf(str, "C0%.2d", nn);
        const auto err = ecl3_keyword_type(str, &type);
        INFO("C0NN = " << str);
        CHECK(err == ECL3_OK);
        CHECK(type == C0NNs[nn - 1]);
    }
}

TEST_CASE("'C000' fails") {
    int type;
    const auto err = ecl3_keyword_type("C000", &type);
    CHECK(err == ECL3_INVALID_ARGS);
}

TEST_CASE("Invalid keyword type fails") {
    int type;
    const auto err = ecl3_keyword_type("FAIL", &type);
    CHECK(err == ECL3_INVALID_ARGS);
}
