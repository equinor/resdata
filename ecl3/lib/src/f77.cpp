#include <cstdint>
#include <cstring>
#include <type_traits>

#include <endianness/endianness.h>

#include <ecl3/f77.h>

namespace {

void memcpy_bswap32(void* d, const void* s, std::size_t nmemb) noexcept (true) {
    std::uint32_t tmp;
    auto* dst = reinterpret_cast< char* >(d);
    auto* src = reinterpret_cast< const char* >(s);
    for (std::size_t i = 0; i < nmemb; ++i) {
        std::memcpy(&tmp, src, sizeof(tmp));
        tmp = bswap32(tmp);
        std::memcpy(dst, &tmp, sizeof(tmp));
        dst += sizeof(tmp);
        src += sizeof(tmp);
    }
}

void memcpy_bswap64(void* d, const void* s, std::size_t nmemb) noexcept (true) {
    std::uint64_t tmp;
    auto* dst = reinterpret_cast< char* >(d);
    auto* src = reinterpret_cast< const char* >(s);
    for (std::size_t i = 0; i < nmemb; ++i) {
        std::memcpy(&tmp, src, sizeof(tmp));
        tmp = bswap64(tmp);
        std::memcpy(dst, &tmp, sizeof(tmp));
        dst += sizeof(tmp);
        src += sizeof(tmp);
    }
}

#if defined(__BIG_ENDIAN__)
void memcpy_msb32(void* dst, const void* src, std::size_t nmemb) noexcept (true) {
    if (dst == src) return;
    std::memcpy(dst, src, nmemb * sizeof(std::uint32_t));
}

void memcpy_msb64(void* dst, const void* src, std::size_t nmemb) noexcept (true) {
    if (dst == src) return;
    std::memcpy(dst, src, nmemb * sizeof(std::uint64_t));
}

void memcpy_lsb32(void* dst, const void* src, std::size_t nmemb) noexcept (true) {
    memcpy_bswap32(dst, src, nmemb);
}

void memcpy_lsb64(void* dst, const void* src, std::size_t nmemb) noexcept (true) {
    memcpy_bswap64(dst, src, nmemb);
}
#elif defined(__LITTLE_ENDIAN__)
void memcpy_msb32(void* dst, const void* src, std::size_t nmemb) noexcept (true) {
    memcpy_bswap32(dst, src, nmemb);
}

void memcpy_msb64(void* dst, const void* src, std::size_t nmemb) noexcept (true) {
    memcpy_bswap64(dst, src, nmemb);
}

void memcpy_lsb32(void* dst, const void* src, std::size_t nmemb) noexcept (true) {
    if (dst == src) return;
    std::memcpy(dst, src, nmemb * sizeof(std::uint32_t));
}

void memcpy_lsb64(void* dst, const void* src, std::size_t nmemb) noexcept (true) {
    if (dst == src) return;
    std::memcpy(dst, src, nmemb * sizeof(std::uint64_t));
}

#else
    #error "__BIG_ENDIAN__ or __LITTLE_ENDIAN__ must be set"
#endif

}

int ecl3_elems_in_block(const char* xs, int fmt, std::size_t* elems) {
    std::uint32_t size;
    std::memcpy(&size, xs, sizeof(size));

    switch (fmt) {
        case 'c':
        case 'i':
        case 'f':
        case 'd':
            size = le32toh(size);
            break;

        case 'C':
        case 'I':
        case 'F':
        case 'D':
            size = be32toh(size);
            break;

        default:
            return ECL3_INVALID_ARGS;
    }

    switch (fmt) {
        case 'c':
        case 'C':
            break;

        case 'i':
        case 'f':
        case 'I':
        case 'F':
            static_assert(sizeof(float) == 4, "assuming 4-byte float");
            size /= 4;
            break;

        case 'd':
        case 'D':
            static_assert(sizeof(double) == 8, "assuming 8-byte double");
            size /= 8;
            break;

        default:
            return ECL3_INVALID_ARGS;
    }

    *elems = size;
    return ECL3_OK;
}

int ecl3_get_native(void* dst, const void* src, int fmt, std::size_t elems) {
    switch (fmt) {
        case 'c':
        case 'C':
            std::memcpy(dst, src, elems);
            return ECL3_OK;

        case 'i': memcpy_lsb32(dst, src, elems); return ECL3_OK;
        case 'f': memcpy_lsb32(dst, src, elems); return ECL3_OK;
        case 'd': memcpy_lsb64(dst, src, elems); return ECL3_OK;

        case 'I': memcpy_msb32(dst, src, elems); return ECL3_OK;
        case 'F': memcpy_msb32(dst, src, elems); return ECL3_OK;
        case 'D': memcpy_msb64(dst, src, elems); return ECL3_OK;

        default:
            return ECL3_INVALID_ARGS;
    }

    return ECL3_OK;
}

int ecl3_put_native(void* dst, const void* src, int fmt, std::size_t elems) {
    switch (fmt) {
        case 'c':
        case 'C':
            std::memcpy(dst, src, elems);
            return ECL3_OK;

        case 'i': memcpy_lsb32(dst, src, elems); return ECL3_OK;
        case 'f': memcpy_lsb32(dst, src, elems); return ECL3_OK;
        case 'd': memcpy_lsb64(dst, src, elems); return ECL3_OK;

        case 'I': memcpy_msb32(dst, src, elems); return ECL3_OK;
        case 'F': memcpy_msb32(dst, src, elems); return ECL3_OK;
        case 'D': memcpy_msb64(dst, src, elems); return ECL3_OK;

        default:
            return ECL3_INVALID_ARGS;
    }

    return ECL3_OK;
}
