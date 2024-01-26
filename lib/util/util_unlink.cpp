#include "ert/util/build_config.hpp"

#include <ert/util/util.hpp>

#if defined(HAVE_WINDOWS_UNLINK)

#include <io.h>

int util_unlink(const char *filename) { return _unlink(filename); }

#elif defined(HAVE_POSIX_UNLINK)

#include <unistd.h>

int util_unlink(const char *filename) { return unlink(filename); }

#else

#error "No unlinke implementation on this platform"

#endif
