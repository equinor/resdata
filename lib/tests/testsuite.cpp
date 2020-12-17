#define CATCH_CONFIG_RUNNER
#include <catch2/catch.hpp>

#include "tmpdir.hpp"

bool Tmpdir::delete_temporary_files = true;

int main(int argc, char* const argv[]) {
    Catch::Session session;

    using namespace Catch::clara;

    auto cli = session.cli() |
               Opt(Tmpdir::delete_temporary_files)["--delete-temporary-files"](
                   "tests delete temporary files they create");

    session.cli(cli);
    int result = session.applyCommandLine(argc, argv);

    if (result != 0) return result;

    result = session.run();

    return result;
}
