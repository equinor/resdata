#include <ert/util/test_util.hpp>

#include "detail/util/path.hpp"

using namespace rd::util;

int main(int argc, char **argv) {

    test_assert_std_string_equal(std::string(""), path::dirname("entry"));
    test_assert_std_string_equal(std::string("entry"), path::basename("entry"));
    test_assert_std_string_equal(std::string(""), path::extension("entry"));

    test_assert_std_string_equal(std::string("path"),
                                 path::dirname("path/base.ext"));
    test_assert_std_string_equal(std::string("base"),
                                 path::basename("path/base.ext"));
    test_assert_std_string_equal(std::string("ext"),
                                 path::extension("path/base.ext"));

    test_assert_std_string_equal(std::string("/tmp"),
                                 path::dirname("/tmp/file"));
    test_assert_std_string_equal(std::string("file"),
                                 path::basename("/tmp/file"));
    test_assert_std_string_equal(std::string(""), path::extension("/tmp/file"));

    test_assert_std_string_equal(std::string("/"), path::dirname("/tmp"));
    test_assert_std_string_equal(std::string("tmp"), path::basename("/tmp"));
    test_assert_std_string_equal(std::string(""), path::extension("/tmp"));

    test_assert_std_string_equal(std::string("/tmp/user.ext"),
                                 path::dirname("/tmp/user.ext/file.ext"));
    test_assert_std_string_equal(std::string("file"),
                                 path::basename("/tmp/user.ext/file.ext"));
    test_assert_std_string_equal(std::string("ext"),
                                 path::extension("/tmp/user.ext/file.ext"));

    test_assert_std_string_equal(std::string("/tmp/user.ext"),
                                 path::dirname("/tmp/user.ext/"));
    test_assert_std_string_equal(std::string(""),
                                 path::basename("/tmp/user.ext/"));
    test_assert_std_string_equal(std::string(""),
                                 path::extension("/tmp/user.ext/"));
}
