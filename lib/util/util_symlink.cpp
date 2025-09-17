#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "ert/util/build_config.hpp"
#include <ert/util/ert_api_config.hpp>
#include <ert/util/util.hpp>

#ifndef ERT_HAVE_SYMLINK

bool util_is_link(const char *path) { return false; }

char *util_alloc_link_target(const char *link) {
    return util_alloc_string_copy(link);
}

#else

#include <dirent.h>
#include <unistd.h>

/**
  This function returns true if path is a symbolic link.
*/

bool util_is_link(const char *path) {
    stat_type stat_buffer;
    if (lstat(path, &stat_buffer) == 0)
        return S_ISLNK(stat_buffer.st_mode);
    else if (errno == ENOENT)
        /*Path does not exist at all. */
        return false;
    else {
        util_abort("%s: stat(%s) failed: %s \n", __func__, path,
                   strerror(errno));
        return false;
    }
}

#ifdef ERT_HAVE_READLINKAT

#if !defined(ERT_HAVE_READLINKAT_DECLARATION)
/*
  The manual page says that the readlinkat() function should be in the
  unistd.h header file, but not on RedHat5. On RedHat6 it is.

*/
extern ssize_t readlinkat(int __fd, __const char *__restrict __path,
                          char *__restrict __buf, size_t __len);
#endif /* !defined(ERT_HAVE_READLINKAT_DECLARATION) */

#endif

#endif // ERT_HAVE_SYMLINK
