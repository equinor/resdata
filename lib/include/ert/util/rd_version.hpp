#ifndef RD_VERSION
#define RD_VERSION

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

const char *rd_version_get_git_commit();
const char *rd_version_get_git_commit_short();
const char *rd_version_get_build_time();
int rd_version_get_major_version();
int rd_version_get_minor_version();
const char *rd_version_get_micro_version();
bool rd_version_is_ert_devel_version();

#ifdef __cplusplus
}
#endif

#endif
