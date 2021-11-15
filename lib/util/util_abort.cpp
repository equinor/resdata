#include <atomic>
#include <csetjmp>
#include <cstdarg>
#include <cstdio>
#include <sstream>
#include <string>

#ifndef WIN32
#include <pwd.h>
#endif

#include <fmt/format.h>
#include <backward.hpp>

#include <ert/util/util.h>

using fmt::format;
using fmt::print;

namespace {

util_abort_handler_t *abort_handler;

/* Used in tests */
jmp_buf test_jmp_buf{};
std::string test_intercept_function;

#ifdef WIN32
#pragma warning("Disabling dumping stack-trace to file on Windows.")
#pragma warning("Please provide a good location for it")
#else
std::string make_dump_path() {
    time_t timestamp = time(NULL);
    char day[32];
    strftime(day, 32, "%Y%m%d-%H%M%S", localtime(&timestamp));

    const auto uid = getuid();
    const auto pwd = getpwuid(uid);

    return fmt::format("/tmp/ert_abort_dump.{}.{}.log",
                       pwd ? pwd->pw_name : std::to_string(uid), day);
}
#endif

} // namespace

extern "C" void util_set_abort_handler(util_abort_handler_t *handler) {
    abort_handler = handler;
}

extern "C" jmp_buf *util_abort_test_jump_buffer() { return &test_jmp_buf; }

extern "C" void util_abort_test_set_intercept_function(const char *function) {
    test_intercept_function = function == nullptr ? "" : function;
}

extern "C" void util_abort__(const char *file, const char *function, int line,
                             const char *fmt, ...) {
    /* Make sure that util_abort__ is entered exactly once */
    static std::atomic_bool abort_flag;
    if (abort_flag.exchange(true))
        std::abort();

    /* Get trace */
    std::string trace;
    {
        backward::StackTrace stacktrace;
        stacktrace.load_here();

        std::ostringstream pretty_trace;
        backward::Printer printer;
        printer.print(stacktrace, pretty_trace);
        trace = pretty_trace.str();
    }

    /* Get formatted error message */
    va_list ap;
    char message[4096];
    va_start(ap, fmt);
    vsnprintf(message, sizeof(message), fmt, ap);
    va_end(ap);

    /* User-defined abort handler */
    if (abort_handler)
        abort_handler(file, line, function, message, trace.c_str());

    /* Exit out of util_abort without aborting. Used in tests. */
    if (test_intercept_function == function) {
        abort_flag = false; // reset abort condition
        longjmp(test_jmp_buf, 0);
    }

    FILE *abort_dump = nullptr;
#ifndef WIN32
    std::string abort_dump_path;
    if (!getenv("ERT_SHOW_BACKTRACE")) {
        abort_dump_path = make_dump_path();
        abort_dump = fopen(abort_dump_path.c_str(), "w");
    }
#endif
    if (abort_dump == nullptr)
        abort_dump = stderr;

    print(abort_dump,
          "\n\nAbort called from: {} ({}:{})\n\nError message: {}\n\n",
          function, file, line, message);
    print(abort_dump, "{}\n", std::string(80, '-'));
    fputs(trace.c_str(), abort_dump);
    print(abort_dump, "{}\n\n", std::string(80, '-'));

#ifndef WIN32
    if (abort_dump != stderr) {
        print(stderr,
              "\nError message: {}\nSee file: {} for more details of the "
              "crash.\nSetting the environment variable \"ERT_SHOW_BACKTRACE\" "
              "will show the backtrace on stderr.\n",
              message, abort_dump_path);
    }
#endif

    std::abort();
}
