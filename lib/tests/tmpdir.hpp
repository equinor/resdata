#pragma once
#include <catch2/catch_message.hpp>
#include <cstdlib>
#include <stdexcept>
#include <string>

#include <filesystem>
#include <system_error>

namespace fs = std::filesystem;

// We do not expect more than single-digit subdirs,
// so we set a maximum number to guarantee a bounded
// loop as the fixture creates directories in a loop.
// We set it to the max subdirs in ext3 which is the
// most limited among reasonably new filesystems.
constexpr int MAX_SUBDIRS = 31998;

/**
 * A tmpdir fixture that creates a temporary directory
 * dirname. Optionally deletes the file in ~Tmpdir if
 * delete_temporary_files is set to true.
 */
struct Tmpdir {
    static bool delete_temporary_files;
    fs::path dirname;
    Tmpdir() {
        // All tmpdirs are created in parendir. We
        // create it if it doesn't exist already
        auto parentdir = fs::temp_directory_path() / "rdtest";
        fs::create_directory(parentdir);
        bool created = false;

        // The instance of Tmpdir that has fs::create_directory
        // return true takes ownership of it.
        for (int i = 0; i < MAX_SUBDIRS &&
                        !(created = fs::create_directory(
                              dirname = (parentdir / std::to_string(i))));
             ++i)
            ;

        if (!created)
            throw std::runtime_error(
                "Tmpdir ran out of available subdir names");
    }
    ~Tmpdir() {
        if (delete_temporary_files) {
            // Rename to a garbage name first, so the original
            // name slot is freed immediately and the slow
            // recursive delete happens on the garbage name.
            // rename is atomic on same-filesystem renames
            // (POSIX) and same-directory renames (NTFS).
            auto garbage = dirname.string() + ".garbage";

            std::error_code ec;
            fs::rename(dirname, garbage, ec);

            if (!ec) {
                fs::remove_all(garbage);
            } else {
                // Rename failed (shouldn't happen on same filesystem)
                // falls back to direct removal
                fs::remove_all(dirname);
            }
        }
    }
};
