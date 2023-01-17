#include <stdlib.h>

#include <string>

#include <ert/util/util.h>

#include "detail/util/path.hpp"

namespace ecl {

namespace util {

namespace path {

/*
        The bizarre dive down into c_str() is to avoid inadvertendtly
        introducing a symbol which is creates a ABI compatibility issue between
        the libstdc++ librararies. A bug in the devtoolset compiler?
      */

std::string dirname(const std::string &fname) {
    size_t last_slash = fname.rfind(UTIL_PATH_SEP_CHAR);
    if (last_slash == std::string::npos)
        return "";

    if (last_slash == 0)
        return fname.substr(0, 1);
    else
        return fname.substr(0, last_slash);
}

std::string basename(const std::string &fname) {
    size_t end_pos = fname.rfind('.');
    size_t offset = fname.rfind(UTIL_PATH_SEP_CHAR);
    if (offset == std::string::npos)
        offset = 0;
    else
        offset += 1;

    {
        const char *c_str = fname.c_str();
        const char *return_raw;
        if (end_pos == std::string::npos || end_pos < offset)
            return_raw = util_alloc_string_copy(&c_str[offset]);
        else
            return_raw =
                util_alloc_substring_copy(c_str, offset, end_pos - offset);
        std::string return_value = return_raw;
        free((void *)return_raw);
        return return_value;
    }
}

std::string extension(const std::string &fname) {
    size_t end_pos = fname.rfind('.');
    size_t last_slash = fname.rfind(UTIL_PATH_SEP_CHAR);
    if (end_pos == std::string::npos)
        return "";

    if (last_slash == std::string::npos || end_pos > last_slash) {
        const char *c_str = fname.c_str();

        const char *return_raw = util_alloc_substring_copy(
            c_str, end_pos + 1, fname.size() - end_pos - 1);
        std::string return_value = return_raw;
        free((void *)return_raw);
        return return_value;
    }

    return "";
}

} // namespace path
} // namespace util
} // namespace ecl
