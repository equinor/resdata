#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <vector>

#include <catch/catch.hpp>
#include <rapidcheck.h>

#include <ecl/fortio.h>

struct closer {
   void operator()( std::FILE* f ) { if( f ) fclose( f ); }
};

using ufp = std::unique_ptr< std::FILE, closer >;

TEST_CASE("Option-less record write", "[fortio][f77][fuzz]") {
    auto ok = rc::check("Option-less record write",
        []( const std::vector< std::int32_t >& l0 ) -> void {
            ufp fp( std::tmpfile() );
            RC_ASSERT( fp );

            auto err = eclfio_put( fp.get(), "", l0.size(), l0.data() );
            RC_ASSERT( err == 0 );

            std::rewind( fp.get() );

            std::int32_t size = l0.size();
            std::vector< std::int32_t > l1( size );

            err = eclfio_get( fp.get(), "", &size, l1.data() );
            RC_ASSERT( err == 0 );
            RC_ASSERT( std::size_t(size) == l0.size() );
            RC_ASSERT( l0 == l1 );
        }
    );

    CHECK( ok );
}

TEST_CASE("Symmetric options does not change get", "[fortio][f77][fuzz]") {
    auto ok = rc::check("opts-dont-matter",
        [] ( const std::string& opts, const std::vector< int >& l0 ) {
            ufp fp( std::tmpfile() );
            RC_ASSERT( fp );

            auto err = eclfio_put( fp.get(),
                                   opts.c_str(),
                                   l0.size(),
                                   l0.data() );
            RC_ASSERT( err == 0 );

            std::rewind( fp.get() );
            std::int32_t size = -1;

            /*
             * don't actually read any data, because this test doesn't scan the
             * opts arg to figure out the *size* of the input type, which
             * changes get and put behaviour (but should work nicely if
             * symmetric.
             *
             * Checking for this is certainly possible, but is of somewhat
             * little value since this test largely checks the robustness of
             * the parsing functionality, and dealing with arbitrary data at
             * that point is a hassle and would obscure.
             *
             * TODO: add a similar test that fixes the type to be chars/bytes
             */
            err = eclfio_get( fp.get(), opts.c_str(), &size, nullptr );

            RC_ASSERT( err == 0 );
            RC_ASSERT( std::size_t(size) == l0.size() );
        }
    );

    CHECK( ok );
}


TEST_CASE("sizeof yields same size as get", "[fortio][f77][fuzz]") {
    auto ok = rc::check("sizeof-get-same-size",
        []( const std::vector< std::int32_t >& l0 ) -> void {

            ufp fp( std::tmpfile() );
            RC_ASSERT( fp );

            auto err = eclfio_put( fp.get(),
                                "",
                                l0.size(),
                                l0.data() );
            RC_ASSERT( err == 0 );
            std::rewind( fp.get() );

            std::int32_t sizeof_size;
            err = eclfio_sizeof( fp.get(), "", &sizeof_size );

            RC_ASSERT( err == 0 );
            RC_ASSERT( std::ftell( fp.get() ) == 0 );

            std::int32_t size;
            err = eclfio_get( fp.get(), "", &size, nullptr );

            RC_ASSERT( err == 0 );
            RC_ASSERT( std::ftell( fp.get() ) > 0 );
            RC_ASSERT( sizeof_size == size );
        }
    );

    CHECK( ok );
}

TEST_CASE("sizeof always rewinds", "[fortio][f77][fuzz]") {
    auto ok = rc::check("sizeof-always-rewinds",
        []( std::vector< int > l0, const std::string& opts ) {

            ufp fp( std::tmpfile() );
            RC_ASSERT( fp );

            auto c = std::fwrite( l0.data(),
                                  sizeof( int ),
                                  l0.size(),
                                  fp.get() );
            RC_ASSERT( c == l0.size() );
            std::rewind( fp.get() );

            std::int32_t size = -1;
            eclfio_sizeof( fp.get(), opts.c_str(), &size );
            RC_ASSERT( std::ftell( fp.get() ) == 0 );
        }
    );

    CHECK( ok );
}
