#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

#include <ert/util/util.hpp>

namespace {

/* Number of elements that keeps the buffer comfortably inside L1/L2, so that
   the measurement reflects the flipping itself rather than memory bandwidth. */
constexpr size_t CACHE_RESIDENT = 8192;

/* Large enough to stream from memory, which is the case when a full keyword
   data section is read from a restart file. */
constexpr size_t STREAMING = 1 << 20;

/** Storage that can hand out both an 8 byte aligned and a deliberately
    4-byte-but-not-8-byte aligned view of the same buffer. */
class FlipBuffer {
    std::vector<uint64_t> storage;

public:
    explicit FlipBuffer(size_t elements, size_t element_size)
        : storage(elements * element_size / sizeof(uint64_t) + 2,
                  0x0123456789abcdefULL) {}

    void *aligned() { return storage.data(); }
    void *offset_by_4() { return reinterpret_cast<char *>(storage.data()) + 4; }
};

std::string label(const char *what, size_t elements) {
    return std::string(what) + ", " + std::to_string(elements) + " elements";
}

} // namespace

TEST_CASE("util_endian_flip_vector baseline", "[.][endian_benchmark]") {
    FlipBuffer big(STREAMING, sizeof(uint64_t));
    FlipBuffer small(CACHE_RESIDENT, sizeof(uint64_t));

    BENCHMARK(label("2 byte, aligned, cache resident", CACHE_RESIDENT)) {
        util_endian_flip_vector(small.aligned(), 2, CACHE_RESIDENT);
    };

    BENCHMARK(label("4 byte, aligned, cache resident", CACHE_RESIDENT)) {
        util_endian_flip_vector(small.aligned(), 4, CACHE_RESIDENT);
    };

    BENCHMARK(label("4 byte, offset by 4, cache resident", CACHE_RESIDENT)) {
        util_endian_flip_vector(small.offset_by_4(), 4, CACHE_RESIDENT);
    };

    BENCHMARK(label("8 byte, aligned, cache resident", CACHE_RESIDENT)) {
        util_endian_flip_vector(small.aligned(), 8, CACHE_RESIDENT);
    };

    BENCHMARK(label("4 byte, aligned, streaming", STREAMING)) {
        util_endian_flip_vector(big.aligned(), 4, STREAMING);
    };

    BENCHMARK(label("4 byte, offset by 4, streaming", STREAMING)) {
        util_endian_flip_vector(big.offset_by_4(), 4, STREAMING);
    };

    BENCHMARK(label("8 byte, aligned, streaming", STREAMING)) {
        util_endian_flip_vector(big.aligned(), 8, STREAMING);
    };
}

TEST_CASE("util_endian_flip_vector baseline for indexed reads",
          "[.][endian_benchmark]") {
    /* The element counts an indexed read produces: a single summary value, the
       three PARAMS entries making up a date, and the four RSEG values. */
    FlipBuffer buffer(8, sizeof(uint64_t));

    BENCHMARK("4 byte, aligned, 1 element") {
        util_endian_flip_vector(buffer.aligned(), 4, 1);
    };

    BENCHMARK("4 byte, aligned, 3 elements") {
        util_endian_flip_vector(buffer.aligned(), 4, 3);
    };

    BENCHMARK("4 byte, offset by 4, 3 elements") {
        util_endian_flip_vector(buffer.offset_by_4(), 4, 3);
    };

    BENCHMARK("8 byte, aligned, 4 elements") {
        util_endian_flip_vector(buffer.aligned(), 8, 4);
    };
}
