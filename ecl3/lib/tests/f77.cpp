#include <cctype>
#include <cstring>
#include <limits>
#include <vector>

#include <catch2/catch.hpp>
#include <endianness/endianness.h>

#include <ecl3/f77.h>
#include <ecl3/keyword.h>

using namespace Catch::Generators;
using namespace Catch::Matchers;

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
        return 0;
        return std::numeric_limits< T >::min();
    }

    static T max() {
        return 1;
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
