#include <cctype>
#include <cstring>
#include <limits>
#include <vector>

#include <catch2/catch.hpp>
#include <endianness/endianness.h>

#include <ecl3/f77.h>

TEST_CASE("elems-in-block reads correct big-endian integral", "[f77]") {
    const char head[] = { 0x00, 0x00, 0x00, 0x10 };

    std::size_t elems;
    auto err = ecl3_elems_in_block(head, 'I', &elems);
    CHECK(err == ECL3_OK);
    CHECK(elems == 4);
}

TEST_CASE("elems-in-block reads correct little-endian integral", "[f77]") {
    const char head[] = { 0x10, 0x00, 0x00, 0x00 };

    std::size_t elems;
    auto err = ecl3_elems_in_block(head, 'i', &elems);
    CHECK(err == ECL3_OK);
    CHECK(elems == 4);
}

TEST_CASE("elems-in-block reads correct big-endian bytes", "[f77]") {
    const char head[] = { 0x00, 0x00, 0x00, 0x10 };

    std::size_t elems;
    auto err = ecl3_elems_in_block(head, 'C', &elems);
    CHECK(err == ECL3_OK);
    CHECK(elems == 16);
}

TEST_CASE("elems-in-block with wrong format returns error", "[f77]") {
    const char head[] = { 0x00, 0x00, 0x00, 0x10 };
    std::size_t elems;
    auto err = ecl3_elems_in_block(head, 'a', &elems);
    CHECK(err == ECL3_INVALID_ARGS);
}

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

    static std::vector< T > to_le(std::vector< T > xs) {
        auto fn = [](T x) {
            using sized = uint_size< sizeof(T) >;
            typename sized::type tmp;
            std::memcpy(&tmp, &x, sizeof(tmp));
            tmp = sized::htole(tmp);
            std::memcpy(&x, &tmp, sizeof(tmp));
            return x;
        };

        std::transform(xs.begin(), xs.end(), xs.begin(), fn);
        return xs;
    }
};

template <> int type< std::int32_t >::fmt() { return GENERATE('i', 'I'); }
template <> int type< float >       ::fmt() { return GENERATE('f', 'F'); }
template <> int type< double >      ::fmt() { return GENERATE('d', 'D'); }

bool not_valid_format(int x) {
    switch (x) {
        case 'c':
        case 'C':
        case 'i':
        case 'f':
        case 'd':
        case 'I':
        case 'F':
        case 'D':
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
    const auto converted = std::isupper(fmt)
                         ? type< T >::to_be(source)
                         : type< T >::to_le(source)
                         ;

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
