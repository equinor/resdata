#include <cstdint>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

#include <catch/catch.hpp>

#include <ecl/fortio.h>

#include "matchers.hpp"

namespace {

struct fcloser {
    void operator()( std::FILE* fp ) { fclose( fp ); }
};

using ufile = std::unique_ptr< std::FILE, fcloser >;

/*
 * Until the scope of network <-> host transformation is known, just copy the
 * implementation into the test program to avoid hassle with linking winsock
 */
#ifdef HOST_BIG_ENDIAN

template< typename T > constexpr T hton( T value ) noexcept { return value; }
template< typename T > constexpr T ntoh( T value ) noexcept { return value; }

#else

template< typename T >
constexpr typename std::enable_if< sizeof(T) == 1, T >::type
hton( T value ) noexcept {
    return value;
}

template< typename T >
constexpr typename std::enable_if< sizeof(T) == 2, T >::type
hton( T value ) noexcept {
   return  ((value & 0x00FF) << 8)
         | ((value & 0xFF00) >> 8)
         ;
}

template< typename T >
constexpr typename std::enable_if< sizeof(T) == 4, T >::type
hton( T value ) noexcept {
   return  ((value & 0x000000FF) << 24)
         | ((value & 0x0000FF00) <<  8)
         | ((value & 0x00FF0000) >>  8)
         | ((value & 0xFF000000) >> 24)
         ;
}

template< typename T >
constexpr typename std::enable_if< sizeof(T) == 8, T >::type
hton( T value ) noexcept {
   return  ((value & 0xFF00000000000000ull) >> 56)
         | ((value & 0x00FF000000000000ull) >> 40)
         | ((value & 0x0000FF0000000000ull) >> 24)
         | ((value & 0x000000FF00000000ull) >>  8)
         | ((value & 0x00000000FF000000ull) <<  8)
         | ((value & 0x0000000000FF0000ull) << 24)
         | ((value & 0x000000000000FF00ull) << 40)
         | ((value & 0x00000000000000FFull) << 56)
         ;
}

// preserve the ntoh name for symmetry
template< typename T >
constexpr T ntoh( T value ) noexcept {
    return hton( value );
}

#endif

}

using Catch::Matchers::Equals;

TEST_CASE("records with broken tail can be read", "[fortio][f77]") {
    ufile handle( std::tmpfile() );
    REQUIRE( handle );
    auto* fp = handle.get();

    std::vector< std::int32_t > src( 10, 0 );
    const std::int32_t head = sizeof( std::int32_t ) * src.size();

    for( int i = 0; i < int(src.size()); ++i )
        src[ i ] = i;

    auto elems = std::fwrite( &head, sizeof( head ), 1, fp );
    REQUIRE_THAT( elems, FwriteCount( 1 ) );

    elems = std::fwrite( src.data(), sizeof( head ), src.size(), fp );
    REQUIRE_THAT( elems, FwriteCount( src.size() ) );

    std::int32_t size = src.size();

    SECTION( "missing tail" ) {

        SECTION("querying size is not affected") {
            std::rewind( fp );

            Err err = eclfio_sizeof( fp, "e", &size );
            CHECK( err == Err::ok() );
            CHECK( size == 10 );
        }

        SECTION("failure with strict read") {
            std::rewind( fp );
            auto out = src;
            const auto pos = std::ftell( fp );

            Err err = eclfio_get( fp, "e", &size, out.data() );
            CHECK( err == Err::unexpected_eof() );
            CHECK( size == 10 );
            CHECK( pos == std::ftell( fp ) );
        }

        SECTION("success with allow-notail ($)") {
            std::rewind( fp );
            auto out = src;
            const auto pos = std::ftell( fp );

            Err err = eclfio_get( fp, "e$", &size, out.data() );
            CHECK( err == Err::ok() );
            CHECK( size == 10 );
            CHECK( pos < std::ftell( fp ) );
            CHECK_THAT( out, Equals( src ) );
        }

        SECTION("success with force-notail (~)") {
            std::rewind( fp );
            auto out = src;
            const auto pos = std::ftell( fp );

            Err err = eclfio_get( fp, "e~", &size, out.data() );
            CHECK( err == Err::ok() );
            CHECK( size == 10 );
            CHECK( pos < std::ftell( fp ) );
            CHECK_THAT( out, Equals( src ) );
        }
    }

    SECTION( "mismatching between head and tail" ) {
        auto tail = head + 1;
        auto elems = std::fwrite( &tail, sizeof( head ), 1, fp );
        REQUIRE_THAT( elems, FwriteCount( 1 ) );

        SECTION("querying size is not affected") {
            std::rewind( fp );

            Err err = eclfio_sizeof( fp, "e", &size );
            CHECK( err == Err::ok() );
            CHECK( size == 10 );
        }

        SECTION("failure with strict read") {
            std::rewind( fp );
            auto out = src;
            const auto pos = std::ftell( fp );

            Err err = eclfio_get( fp, "e", &size, out.data() );
            CHECK( err == Err::invalid_record() );
            CHECK( size == 10 );
            CHECK( pos == std::ftell( fp ) );
        }

        SECTION("success with allow-notail ($)") {
            std::rewind( fp );
            auto out = src;
            const auto pos = std::ftell( fp );

            Err err = eclfio_get( fp, "e$", &size, out.data() );
            CHECK( err == Err::ok() );
            CHECK( size == 10 );
            CHECK( pos < std::ftell( fp ) );
            CHECK_THAT( out, Equals( src ) );
        }

        SECTION("success with force-notail (~)") {
            std::rewind( fp );
            auto out = src;
            const auto pos = std::ftell( fp );

            Err err = eclfio_get( fp, "e~", &size, out.data() );
            CHECK( err == Err::ok() );
            CHECK( size == 10 );
            CHECK( pos < std::ftell( fp ) );
            CHECK_THAT( out, Equals( src ) );
        }
    }
}

TEST_CASE("record with valid, but too small body", "[fortio][f77]") {
    ufile handle( std::tmpfile() );
    REQUIRE( handle );
    auto* fp = handle.get();

    std::vector< std::int32_t > src( 10, 0 );
    const std::int32_t head = sizeof(std::int32_t) * (src.size() + 2) ;

    auto elems = std::fwrite( &head, sizeof( head ), 1, fp );
    REQUIRE_THAT( elems, FwriteCount( 1 ) );

    elems = std::fwrite( src.data(), sizeof( head ), src.size(), fp );
    REQUIRE_THAT( elems, FwriteCount( src.size() ) );

    std::rewind( fp );
    const auto pos = std::ftell( fp );
    std::vector< std::int32_t > out( head, 0 );
    std::int32_t size = out.size();
    Err err = eclfio_get( fp, "e", &size, out.data() );

    CHECK( err == Err::unexpected_eof() );
    CHECK( pos == std::ftell( fp ) );
    CHECK( size == out.size() );
}

TEST_CASE("record with invalid head", "[fortio][f77]") {
    ufile handle( std::tmpfile() );
    REQUIRE( handle );
    auto* fp = handle.get();

    std::vector< std::int32_t > src( 10, 0 );

    for( std::int32_t head : { -4, 11 } ) SECTION(
          head < 0
        ? "negative length"
        : "length not multiple of element size" ) {

        auto elems = std::fwrite( &head, sizeof( head ), 1, fp );
        REQUIRE_THAT( elems, FwriteCount( 1 ) );

        const std::vector< std::int32_t > src( 10, 0 );
        elems = std::fwrite( src.data(), sizeof( head ), src.size(), fp );
        REQUIRE_THAT( elems, FwriteCount( src.size() ) );

        std::rewind( fp );
        const auto pos = std::ftell( fp );
        std::int32_t size = src.size();
        Err err = eclfio_get( fp, "e", nullptr, nullptr );

        CHECK( err == Err::invalid_record() );
        CHECK( pos == std::ftell( fp ) );
        CHECK( size == src.size() );
    }
}

TEST_CASE("requesting string does not consider endianness", "[fortio][f77]") {
    ufile handle( std::tmpfile() );
    REQUIRE( handle );
    auto* fp = handle.get();

    const std::string expected = "FOPT    MINISTEP";
    Err err = eclfio_put( fp, "b", 16, expected.data() );
    REQUIRE( err == Err::ok() );
    std::rewind( fp );

    for( const std::string opts : { "s", "st", "ts", "fst" } )
    SECTION( "with options: " + opts ) {
        char data[17]  = { 0 };
        std::int32_t size = 2;
        err = eclfio_get( fp, opts.c_str(), &size, data );

        CHECK( err == Err::ok() );
        CHECK( data == expected );
        CHECK( size == 2 );
    }
}

TEST_CASE("encountering EOF after valid block", "[fortio][f77]") {
    ufile handle( std::tmpfile() );
    REQUIRE( handle );
    auto* fp = handle.get();

    SECTION("in empty file") {
        std::int32_t size = 10;
        Err err = eclfio_get( fp, "", &size, nullptr );
        CHECK( err == Err::eof() );
    }

    SECTION("after single, empty block") {
        std::int32_t head = 0;
        std::fwrite( &head, sizeof( head ), 1, fp );
        std::fwrite( &head, sizeof( head ), 1, fp );
        std::rewind( fp );

        std::int32_t size = -1;
        Err err = eclfio_sizeof( fp, "", &size );
        CHECK( err == Err::ok() );
        CHECK( size == 0 );

        err = eclfio_get( fp, "", &size, nullptr );

        CHECK( err == Err::ok() );
        CHECK( size == 0 );

        err = eclfio_get( fp, "", &size, nullptr );
        CHECK( err == Err::eof() );
    }

    SECTION("after single, non-empty block") {
        std::int32_t head = 10;
        std::vector< std::int32_t > src( 10 );

        Err err = eclfio_put( fp, "", head, src.data() );
        REQUIRE( err == Err::ok() );
        std::rewind( fp );

        std::int32_t size = head;
        err = eclfio_get( fp, "", &size, nullptr );

        CHECK( err == Err::ok() );
        CHECK( size == 10 );

        err = eclfio_get( fp, "", &size, nullptr );
        CHECK( err == Err::eof() );
    }
}

TEST_CASE("unexpected EOF in block body", "[fortio][f77]") {
    ufile handle( std::tmpfile() );
    REQUIRE( handle );
    auto* fp = handle.get();

    /* allocate more room than we write (except the write is too short) */
    std::vector< std::int32_t > out( 4, 0 );

    std::int32_t head = hton( std::int32_t( 3 * sizeof( std::int32_t ) ) );
    std::fwrite( &head, sizeof( head ), 1, fp );
    std::fwrite( &head, sizeof( head ), 1, fp );
    std::fwrite( &head, sizeof( head ), 1, fp );

    std::rewind( fp );
    std::int32_t size = sizeof( std::int32_t ) * out.size();

    SECTION( "when reading body" ) {
        Err err = eclfio_get( fp, "", &size, out.data() );

        CHECK( err == Err::unexpected_eof() );
        /* expect size not to be modified after failed call */
        CHECK( size == 16 );
    }

    SECTION( "when skipping body" ) {
        /* skipping the record should raise the same error */
        Err err = eclfio_get( fp, "", &size, nullptr );
        CHECK( err == Err::unexpected_eof() );
        CHECK( size == 16 );
    }

    SECTION( "when reading the unbounded body" ) {
        /* reading without size hint should not affect unexpected EOFs */
        Err err = eclfio_get( fp, "", nullptr, out.data() );
        CHECK( err == Err::unexpected_eof() );
        CHECK( size == 16 );
    }

    SECTION( "when skipping the unbounded body" ) {
        /* skipping without size hint should not affect unexpected EOFs */
        Err err = eclfio_get( fp, "", nullptr, nullptr );
        CHECK( err == Err::unexpected_eof() );
        CHECK( size == 16 );
    }
}
