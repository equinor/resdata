#ifndef ECL_VERSION
#define ECL_VERSION

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

const char *ecl_version_get_git_commit();
const char *ecl_version_get_git_commit_short();
const char *ecl_version_get_build_time();
int ecl_version_get_major_version();
int ecl_version_get_minor_version();
const char *ecl_version_get_micro_version();
bool ecl_version_is_ert_devel_version();

#ifdef __cplusplus
}
#endif

#endif
