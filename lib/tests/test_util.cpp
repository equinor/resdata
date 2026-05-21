#include <catch2/catch_test_macros.hpp>
#include <cstdlib>
#include <ert/util/util.hpp>
#include <filesystem>

#include "tmpdir.hpp"

TEST_CASE_METHOD(Tmpdir, "Test getcwd after unlink cwd", "[unittest]") {
    GIVEN("Test directory") {
        auto subdir = dirname / "unlink";
        auto previous_cwd = fs::current_path();
        fs::create_directory(subdir);
        fs::current_path(subdir);

        THEN("Use normally") {
            auto path = util_alloc_cwd();
            std::free(path);
        }

        THEN("Unlink") {
            fs::remove(subdir);
            CHECK_THROWS(util_alloc_cwd());
        }

        fs::current_path(previous_cwd);
    }
}
