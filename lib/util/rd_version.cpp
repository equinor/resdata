#include <ert/util/util.hpp>
#include <ert/util/rd_version.hpp>

#define xstr(s) #s
#define str(s) xstr(s)

const char *rd_version_get_git_commit() {
#ifdef GIT_COMMIT
    return str(GIT_COMMIT);
#else
    return "Unknown git commit hash";
#endif
}

const char *rd_version_get_build_time() {
#ifdef COMPILE_TIME_STAMP
    return COMPILE_TIME_STAMP;
#else
    return "Unknown build time";
#endif
}

int rd_version_get_major_version() { return RD_VERSION_MAJOR; }

int rd_version_get_minor_version() { return RD_VERSION_MINOR; }

const char *rd_version_get_micro_version() { return str(RD_VERSION_MICRO); }

bool rd_version_is_devel_version() {
    return util_sscanf_int(str(RD_VERSION_MICRO), NULL);
}
