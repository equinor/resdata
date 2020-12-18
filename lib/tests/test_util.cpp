#include <catch2/catch.hpp>
#include <ert/util/util.hpp>

#include "tmpdir.hpp"

using namespace Catch;
using namespace Matchers;

TEST_CASE_METHOD(Tmpdir, "Test getcwd after unlink cwd", "[unittest]") {
    GIVEN("Test directory") {
        auto subdir = dirname / "unlink";
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

        fs::current_path(dirname);
    }
}
