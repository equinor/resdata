#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>

#include <catch2/catch.hpp>
#include <endianness/endianness.h>

#include <ecl3/keyword.h>

namespace {

template < std::size_t > struct uint_size {};
template < > struct uint_size< 4 > {
    using type = std::uint32_t;
    static type htobe(type x) { return htobe32(x); }
    static type htole(type x) { return htole32(x); }
};
template < > struct uint_size< 8 > {
    using type = std::uint64_t;
    static type htobe(type x) { return htobe64(x); }
    static type htole(type x) { return htole64(x); }
};

template < typename T >
struct type {
    static T min() {
        return std::numeric_limits< T >::min();
    }

    static T max() {
        return std::numeric_limits< T >::max();
    }

    static int fmt();

    static std::vector< char > buffer(std::size_t size) {
        return std::vector< char >(size * sizeof(T));
    }

    static std::vector< T > to_be(std::vector< T > xs) {
        auto fn = [](T x) {
            using sized = uint_size< sizeof(T) >;
            typename sized::type tmp;
            std::memcpy(&tmp, &x, sizeof(tmp));
            tmp = sized::htobe(tmp);
            std::memcpy(&x, &tmp, sizeof(tmp));
            return x;
        };

        std::transform(xs.begin(), xs.end(), xs.begin(), fn);
        return xs;
    }
};

template <> int type< std::int32_t >::fmt() { return ECL3_INTE; }
template <> int type< float >       ::fmt() { return ECL3_REAL; }
template <> int type< double >      ::fmt() { return ECL3_DOUB; }

const ecl3_keyword_types C0NNs[] = {
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

bool not_valid_format(int x) {
    switch (x) {
        case ECL3_INTE:
        case ECL3_REAL:
        case ECL3_DOUB:
        case ECL3_CHAR:
        case ECL3_MESS:
        case ECL3_LOGI:
        case ECL3_X231:

        case ECL3_C001:
        case ECL3_C002:
        case ECL3_C003:
        case ECL3_C004:
        case ECL3_C005:
        case ECL3_C006:
        case ECL3_C007:
        case ECL3_C008:
        case ECL3_C009:
        case ECL3_C010:
        case ECL3_C011:
        case ECL3_C012:
        case ECL3_C013:
        case ECL3_C014:
        case ECL3_C015:
        case ECL3_C016:
        case ECL3_C017:
        case ECL3_C018:
        case ECL3_C019:
        case ECL3_C020:
        case ECL3_C021:
        case ECL3_C022:
        case ECL3_C023:
        case ECL3_C024:
        case ECL3_C025:
        case ECL3_C026:
        case ECL3_C027:
        case ECL3_C028:
        case ECL3_C029:
        case ECL3_C030:
        case ECL3_C031:
        case ECL3_C032:
        case ECL3_C033:
        case ECL3_C034:
        case ECL3_C035:
        case ECL3_C036:
        case ECL3_C037:
        case ECL3_C038:
        case ECL3_C039:
        case ECL3_C040:
        case ECL3_C041:
        case ECL3_C042:
        case ECL3_C043:
        case ECL3_C044:
        case ECL3_C045:
        case ECL3_C046:
        case ECL3_C047:
        case ECL3_C048:
        case ECL3_C049:
        case ECL3_C050:
        case ECL3_C051:
        case ECL3_C052:
        case ECL3_C053:
        case ECL3_C054:
        case ECL3_C055:
        case ECL3_C056:
        case ECL3_C057:
        case ECL3_C058:
        case ECL3_C059:
        case ECL3_C060:
        case ECL3_C061:
        case ECL3_C062:
        case ECL3_C063:
        case ECL3_C064:
        case ECL3_C065:
        case ECL3_C066:
        case ECL3_C067:
        case ECL3_C068:
        case ECL3_C069:
        case ECL3_C070:
        case ECL3_C071:
        case ECL3_C072:
        case ECL3_C073:
        case ECL3_C074:
        case ECL3_C075:
        case ECL3_C076:
        case ECL3_C077:
        case ECL3_C078:
        case ECL3_C079:
        case ECL3_C080:
        case ECL3_C081:
        case ECL3_C082:
        case ECL3_C083:
        case ECL3_C084:
        case ECL3_C085:
        case ECL3_C086:
        case ECL3_C087:
        case ECL3_C088:
        case ECL3_C089:
        case ECL3_C090:
        case ECL3_C091:
        case ECL3_C092:
        case ECL3_C093:
        case ECL3_C094:
        case ECL3_C095:
        case ECL3_C096:
        case ECL3_C097:
        case ECL3_C098:
        case ECL3_C099:
            return false;
    }

    return true;
}

}

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

using namespace Catch::Matchers;
using namespace Catch::Generators;

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

TEST_CASE("ecl3_keyword_size for ECL3_INTE is correct") {
    int size;
    const auto err = ecl3_keyword_size(ECL3_INTE, &size);
    CHECK(err == ECL3_OK);
    CHECK(size == sizeof(std::int32_t));
}

TEST_CASE("ecl3_keyword_size for ECL3_REAL is correct") {
    int size;
    const auto err = ecl3_keyword_size(ECL3_REAL, &size);
    CHECK(err == ECL3_OK);
    CHECK(size == sizeof(float));
    static_assert(
        sizeof(float) == 4,
        "ecl3_keyword_size assumes sizeof(float) == 4"
    );
}

TEST_CASE("ecl3_keyword_size for ECL3_DOUB is correct") {
    int size;
    const auto err = ecl3_keyword_size(ECL3_DOUB, &size);
    CHECK(err == ECL3_OK);
    CHECK(size == sizeof(double));
    static_assert(
        sizeof(double) == 8,
        "ecl3_keyword_size assumes sizeof(double) == 8"
    );
}

TEST_CASE("ecl3_keyword_size for ECL3_CHAR is correct") {
    int size;
    const auto err = ecl3_keyword_size(ECL3_CHAR, &size);
    CHECK(err == ECL3_OK);
    CHECK(size == 8);
}

TEST_CASE("ecl3_keyword_size for ECL3_MESS is correct") {
    int size;
    const auto err = ecl3_keyword_size(ECL3_MESS, &size);
    CHECK(err == ECL3_OK);
    CHECK(size == 0);
}

TEST_CASE("ecl3_keyword_size for ECL3_LOGI is correct") {
    int size;
    const auto err = ecl3_keyword_size(ECL3_LOGI, &size);
    CHECK(err == ECL3_OK);
    CHECK(size == 4);
}

TEST_CASE("ecl3_keyword_size for ECL3_X231 is correct") {
    int size;
    const auto err = ecl3_keyword_size(ECL3_X231, &size);
    CHECK(err == ECL3_UNSUPPORTED);
}

TEST_CASE("ecl3_keyword_size for ECL3_C0NN is correct") {
    for (auto nn = 1; nn < 100; ++nn) {
        int size;
        auto C0NN = C0NNs[nn - 1];
        const auto err = ecl3_keyword_size(C0NN, &size);
        CHECK(err == ECL3_OK);
        CHECK(size == nn);
    }
}

TEST_CASE("ecl3_keyword_size detects wrong argument") {
    int size;
    const auto err = ecl3_keyword_size(7132, &size);
    CHECK(err == ECL3_INVALID_ARGS);
}

TEST_CASE("ecl3_block_size for ECL3_INTE is correct") {
    int size;
    const auto err = ecl3_block_size(ECL3_INTE, &size);
    CHECK(err == ECL3_OK);
    CHECK(size == 1000);
}

TEST_CASE("ecl3_block_size for ECL3_REAL is correct") {
    int size;
    const auto err = ecl3_block_size(ECL3_REAL, &size);
    CHECK(err == ECL3_OK);
    CHECK(size == 1000);
}

TEST_CASE("ecl3_block_size for ECL3_DOUB is correct") {
    int size;
    const auto err = ecl3_block_size(ECL3_DOUB, &size);
    CHECK(err == ECL3_OK);
    CHECK(size == 1000);
}

TEST_CASE("ecl3_block_size for ECL3_CHAR is correct") {
    int size;
    const auto err = ecl3_block_size(ECL3_CHAR, &size);
    CHECK(err == ECL3_OK);
    CHECK(size == 105);
}

TEST_CASE("ecl3_block_size for ECL3_MESS is correct") {
    int size;
    const auto err = ecl3_block_size(ECL3_MESS, &size);
    CHECK(err == ECL3_OK);
    CHECK(size == 1000);
}

TEST_CASE("ecl3_block_size for ECL3_LOGI is correct") {
    int size;
    const auto err = ecl3_block_size(ECL3_LOGI, &size);
    CHECK(err == ECL3_OK);
    CHECK(size == 1000);
}

TEST_CASE("ecl3_block_size for ECL3_X231 is correct") {
    int size;
    const auto err = ecl3_block_size(ECL3_X231, &size);
    CHECK(size == 1000);
    CHECK(err == ECL3_OK);
}

TEST_CASE("ecl3_block_size for ECL3_C0NN is correct") {
    for (auto nn = 1; nn < 100; ++nn) {
        int size;
        auto C0NN = C0NNs[nn - 1];
        const auto err = ecl3_block_size(C0NN, &size);
        CHECK(err == ECL3_OK);
        CHECK(size == 105);
    }
}

template < typename T >
void roundtrip() {
    const auto fmt = type< T >::fmt();
    const auto source = GENERATE(
        take(10, chunk(100, random(type< T >::min(), type< T >::max())))
    );
    const auto size = source.size();
    auto buffer = type< T >::buffer(size);
    auto result = std::vector< T >(size);

    ecl3_put_native(buffer.data(), source.data(), fmt, size);
    ecl3_get_native(result.data(), buffer.data(), fmt, size);
    INFO("fmt = " << fmt);
    CHECK_THAT(result, Equals(source));
}

template < typename T >
void read_formatted() {
    const auto fmt = type< T >::fmt();
    const auto source = GENERATE(
        take(10, chunk(100, random(type< T >::min(), type< T >::max())))
    );
    const auto size = source.size();
    const auto converted = type< T >::to_be(source);

    auto result = std::vector< T >(size);
    ecl3_get_native(result.data(), converted.data(), fmt, size);
    INFO("fmt = " << fmt);
    CHECK_THAT(result, Equals(source));
}

TEST_CASE("get-put int roundtrip maintains equality") {
    roundtrip< std::int32_t >();
}

TEST_CASE("get-put float roundtrip maintains equality") {
    roundtrip< float >();
}

TEST_CASE("get-put double roundtrip maintains equality") {
    roundtrip< double >();
}

TEST_CASE("reading formatted integers") {
    read_formatted< std::int32_t >();
}

TEST_CASE("reading formatted floats") {
    read_formatted< float >();
}

TEST_CASE("reading formatted doubles") {
    read_formatted< double >();
}

TEST_CASE("invalid format-argument to get_native fails") {
    const auto fmt = GENERATE(
        take(100, filter(not_valid_format, random(-10000, 1000000)))
    );
    INFO("fmt = " << fmt);
    const auto err = ecl3_get_native(nullptr, nullptr, fmt, 0);
    CHECK(err == ECL3_INVALID_ARGS);
}

TEST_CASE("invalid format-argument to put_native fails") {
    const auto fmt = GENERATE(
        take(100, filter(not_valid_format, random(-10000, 1000000)))
    );
    INFO("fmt = " << fmt);
    const auto err = ecl3_put_native(nullptr, nullptr, fmt, 0);
    CHECK(err == ECL3_INVALID_ARGS);
}

namespace {

template < typename T >
void read_blocked_numeric_array() {
    // TODO: figure out a good way of testing C0NN and CHAR
    static const auto minv = type< T >::min();
    static const auto maxv = type< T >::max();
    const auto chunks = 5;

    const auto source = GENERATE(
        take(10, chunk(4800, random(minv, maxv)))
    );

    // TODO: share the generate-bunch-of-vectors code with f77
    auto buffer = type< T >::buffer(source.size());
    const auto fmt = type< T >::fmt();
    auto err = ecl3_put_native(buffer.data(),
                               source.data(),
                               fmt,
                               source.size());

    REQUIRE(err == ECL3_OK);

    /*
     * Ensure that the ecl3_array_body pauses reading enough times, and that
     * the destination is filled completely
     */
    auto result = std::vector< T >(source.size());
    auto* dst = result.data();
    auto* src = buffer.data();
    int remaining = source.size();

    int block_size;
    err = ecl3_block_size(fmt, &block_size);
    REQUIRE(err == ECL3_OK);

    int iterations = 0;
    while (remaining > 0) {
        int read;
        const auto err = ecl3_array_body(dst,
                                         src,
                                         fmt,
                                         remaining,
                                         block_size,
                                         &read);
        REQUIRE(err == ECL3_OK);

        CHECK(((remaining >= block_size and read == block_size)
            or (remaining <  block_size and read == remaining)
        ));

        /*
         * Users too must manually advance the dst/src pointers, according to
         * how they're stored. Although it's omitted here, when read from disk
         * the chunks will be interspersed with block heads/tails.
         */
        remaining -= read;
        dst += read;
        src += read * sizeof(T);
        ++iterations;
    }

    CHECK(remaining == 0);
    CHECK(iterations == chunks);
    CHECK_THAT(result, Equals(source));
}

}

TEST_CASE("Read blocked array of INTE") {
    read_blocked_numeric_array< std::int32_t >();
}

TEST_CASE("Read blocked array of REAL") {
    read_blocked_numeric_array< float >();
}

TEST_CASE("Read blocked array of DOUB") {
    read_blocked_numeric_array< double >();
}

// TODO: add CHAR
