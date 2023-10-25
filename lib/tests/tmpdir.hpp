#ifndef LIB_TESTS_TMPDIR_HPP
#define LIB_TESTS_TMPDIR_HPP
#include <catch2/catch.hpp>
#include <iostream>
#include <string>

#ifdef FS_EXPERIMENTAL
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#include <filesystem>
namespace fs = std::filesystem;
#endif

class Tmpdir {
public:
    static bool delete_temporary_files;
    fs::path dirname;
    Tmpdir() {
        auto parentdir = fs::temp_directory_path() / "rd_test";

        do {
            dirname = parentdir / std::to_string(rand());
        } while (fs::exists(dirname));

        fs::create_directories(dirname);
        UNSCOPED_INFO("Writing files to " << dirname << "\n");
    }
    ~Tmpdir() {
        if (delete_temporary_files) {
            fs::remove_all(dirname);
        }
    }
};
#endif
