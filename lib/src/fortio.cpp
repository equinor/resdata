#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include <stdexcept>

#include <ecl/fortio.h>

#ifdef HAVE_NETINET_IN_H
    #include <netinet/in.h>
#elif HAVE_ARPA_INET_H
    #include <arpa/inet.h>
#elif HAVE_WINSOCK2_H
    #include <winsock2.h>
#endif

namespace {

/*
 * Functions in fortio will try and roll back as much state as possible in case
 * of errors, to provide some exception safety. Since these error paths are
 * more cleanly handled with returned, the rollback is tied to the fguard's
 * destructor.
 *
 * However, if the rollback itself fails, the file is left in an unspecific
 * state, but we have no immediate way to report that, as the destructor does
 * not return a value. Instead, an exception is thrown, and all function use
 * function-try-blocks. While it's generally bad practice to throw from
 * destructors, the eclfio functions are extern C'd, and no exceptions leak.
 * Since there are no other destructors run from these functions than that of
 * fguard, the destructor-exception is ok, since it never happens during stack
 * unwinding.
 *
 * A string-less exception type is used, and std::exception() is noexcept in
 * C++11.
 */
struct fguard {
    FILE* fp;
    std::fpos_t pos;

    fguard( FILE* x ) : fp( x ) {
        const auto err = std::fgetpos( fp, &this->pos );

        if( err ) {
            /*
             * when fgetpos fails, presumably everything is broken and we
             * didn't even record our previous position. no point even trying
             * to roll back from this, so set fp to nullptr and bail
             */
            this->fp = nullptr;
            throw std::exception();
        }
    }

    ~fguard() noexcept( false ) {
        if( !this->fp ) return;

        const auto err = std::fsetpos( fp, &this->pos );
        if( err ) throw std::exception();
    }
};

struct options {
    bool bigendian  = true;
    bool size_limit = true;
    int elemsize    = sizeof( std::int32_t );

    // transform by default unless record is nullptr and this record should be
    // skipped
    bool transform    = true;
    bool force_notail = false;
    bool allow_notail = false;
};

options parse_opts( const char* opts ) noexcept {
    options o;

    for( const char* op = opts; *op != '\0'; ++op ) {
        switch( *op ) {
            case 'c': o.elemsize = sizeof( char );         break;
            case 'i': o.elemsize = sizeof( std::int32_t ); break;
            case 'f': o.elemsize = sizeof( float );        break;
            case 'd': o.elemsize = sizeof( double );       break;

            case 'E': o.bigendian = true;                  break;
            case 'e': o.bigendian = false;                 break;

            case 't': o.transform = true;                  break;
            case 'T': o.transform = false;                 break;

            case '~': o.force_notail = true;               break;
            case '$': o.allow_notail = true;               break;

            case '#': o.size_limit = false;                break;
            default: break;
        }
    }

    return o;
}

}

int eclfio_sizeof( std::FILE* fp, const char* opts, int32_t* out ) try {
    const auto o = parse_opts( opts );

    fguard guard( fp );

    std::int32_t size;
    const auto read = std::fread( &size, sizeof( size ), 1, fp );
    if( read != 1 ) return ECL_ERR_READ;

    if( o.bigendian )
        size = ntohl( size );

    guard.fp = nullptr;

    *out = size;
    return ECL_OK;

} catch( std::exception& ) {
    return ECL_ERR_SEEK;
}

int eclfio_skip( FILE* fp, const char* opts, int n ) try {
    fguard guard( fp );

    // TODO: support backwards skips
    if( n < 0 ) return ECL_EINVAL;

    for( int i = 0; i < n; ++i ) {
        int err = eclfio_get( fp, opts, nullptr, nullptr );
        if( err ) return err;
    }

    guard.fp = nullptr;
    return ECL_OK;
} catch( std::exception& ) {
    return ECL_ERR_SEEK;
}

int eclfio_get( std::FILE* fp,
                const char* opts,
                int32_t* recordsize,
                void* record ) try {

    auto o = parse_opts( opts );

    // if this is a skip, opt out of transform altogether
    o.transform = o.transform and bool(record);
    o.size_limit = o.size_limit and record and recordsize;

    fguard guard( fp );

    std::int32_t head;
    auto read = std::fread( &head, sizeof( head ), 1, fp );
    if( read != 1 ) return ECL_ERR_READ;

    if( o.bigendian )
        head = (std::int32_t)ntohl( (std::uint32_t)head );

    if( head < 0 ) return ECL_INVALID_RECORD;

    if( o.size_limit and head > *recordsize * o.elemsize )
        return ECL_EINVAL;

    if( record ) {
        read = std::fread( record, head, 1, fp );
        if( read != 1 ) return ECL_ERR_READ;
    } else {
        /* read-buffer is zero, so skip this record instead of reading it */
        const auto err = std::fseek( fp, head, SEEK_CUR );
        if( err ) return ECL_ERR_READ;
    }

    const auto elems = head / o.elemsize;

    if( o.transform and o.bigendian and o.elemsize > 1 ) {
        char* ptr = static_cast< char* >( record );
        if( o.elemsize == sizeof( std::int16_t ) ) {
            for( int i = 0; i < head / o.elemsize; ++i ) {
                std::uint16_t x;
                std::memcpy( &x, ptr, sizeof( x ) );
                x = ntohs( x );
                std::memcpy( ptr, &x, sizeof( x ) );
                ptr += 2;
            }
        } else {
            for( int i = 0; i < head / o.elemsize; ++i ) {
                std::uint16_t x;
                std::memcpy( &x, ptr, sizeof( x ) );
                x = ntohl( x );
                std::memcpy( ptr, &x, sizeof( x ) );
                ptr += 4;
            }
        }
    }

    if( o.force_notail ) {
        if( recordsize ) *recordsize = elems;
        guard.fp = nullptr;
        return ECL_OK;
    }

    fguard tailguard( fp );
    std::int32_t tail;
    read = std::fread( &tail, sizeof( tail ), 1, fp );

    if( read == 1 and o.bigendian )
        tail = (std::int32_t)ntohl( (std::uint32_t)tail );

    /*
     * success; record has a tail, and it matched our head. All is good -
     * return and don't rewind. This is the most common case
     */
    if( read == 1 and head == tail ) {
        if( recordsize ) *recordsize = elems;
        tailguard.fp = guard.fp = nullptr;
        return ECL_OK;
    }

    /*
     * read was sucessful, but the tail didn't match. However, missing tails
     * are ok, so rewind the tail, and preserve position at end-of-record
     */
    if( read == 1 and head != tail and o.allow_notail ) {
        if( recordsize ) *recordsize = elems;
        guard.fp = nullptr;
        return ECL_OK;
    }

    /*
     * last record, and ok to not have tail. don't rewind as this is a success
     */
    if( read != 1 and feof( fp ) and o.allow_notail ) {
        if( recordsize ) *recordsize = elems;
        guard.fp = tailguard.fp = nullptr;
        return ECL_OK;
    }

    /*
     * read was succesful, but the tail didn't match, so flag the record as
     * invalid and rewind
     */
    if( read == 1 and head != tail )
        return ECL_INVALID_RECORD;

    /* read failed, but not at end-of-file */
    if( read != 1 and ferror( fp ) )
        return ECL_ERR_READ;

    /*
     * The read failed at end-of-file, and we expected a tail. Flag the record
     * as invalid and roll back the file pointer
     */
    if( read != 1 and feof( fp ) and not o.allow_notail )
        return ECL_INVALID_RECORD;

    return ECL_ERR_UNKNOWN;
} catch( std::exception& ) {
    return ECL_ERR_SEEK;
}

int eclfio_put( std::FILE* fp,
                const char* opts,
                int nmemb,
                const void* record ) try {

    const auto o = parse_opts( opts );

    if(   std::numeric_limits< std::int32_t >::max()
        < std::int64_t(nmemb) * std::int64_t(o.elemsize) ) {
        /*
         * a huge record (>2G) is being requested, but the 4-byte headers don't
         * allow that. consider this an invalid argument and bail
         */
        return ECL_EINVAL;
    }

    fguard guard( fp );

    std::int32_t head = o.elemsize * nmemb;

    if( o.bigendian )
        head = (std::int32_t)htonl( (std::uint32_t)head );

    auto write = std::fwrite( &head, sizeof( head ), 1, fp );
    if( write != 1 ) return ECL_ERR_WRITE;

    const auto size = o.elemsize;
    int bytes_read = 0;

    char buffer[ 4096 ];
    while( nmemb > 0 ) {
        const auto items = std::min( nmemb, 4096 / size );
        const auto bytes = items * size;
        memcpy( buffer, (char*)record + bytes_read, bytes );

        if( o.transform and o.bigendian and o.elemsize > 1 ) {
            char* ptr = buffer;
            if( o.elemsize == sizeof( std::int16_t ) ) {
                for( int i = 0; i < items; ++i ) {
                    std::uint16_t x;
                    std::memcpy( &x, ptr, sizeof( x ) );
                    x = htons( x );
                    std::memcpy( ptr, &x, sizeof( x ) );
                    ptr += 2;
                }
            } else {
                for( int i = 0; i < items; ++i ) {
                    std::uint32_t x;
                    std::memcpy( &x, ptr, sizeof( x ) );
                    x = htonl( x );
                    std::memcpy( ptr, &x, sizeof( x ) );
                    ptr += 4;
                }
            }
        }

        write = fwrite( record, size, items, fp );
        if( int( write ) != items ) return ECL_ERR_WRITE;

        nmemb -= items;
        bytes_read += bytes;
    }


    if( o.force_notail ) {
        guard.fp = nullptr;
        return ECL_OK;
    }

    write = std::fwrite( &head, sizeof( head ), 1, fp );
    if( write != 1 ) return ECL_ERR_WRITE;

    guard.fp = nullptr;
    return ECL_OK;

} catch( std::exception& ) {
    return ECL_ERR_SEEK;
}
