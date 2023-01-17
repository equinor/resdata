#include <string>
#include <stdarg.h>

namespace ecl {
namespace util {

std::string string_format(const char *fmt, ...) {
    int length;
    std::string s;
    {
        va_list va;
        va_start(va, fmt);
        length = vsnprintf(NULL, 0, fmt, va);
        va_end(va);
    }
    s.resize(length + 1);
    {
        va_list va;
        va_start(va, fmt);
        vsprintf((char *)s.data(), fmt, va);
        va_end(va);
    }
    return s;
}
} // namespace util
} // namespace ecl
