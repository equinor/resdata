#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <ert/util/util.hpp>

#include <dirent.h>
#include <sys/types.h>

static void util_copy_directory__(const char *src_path, const char *target_path,
                                  int buffer_size, void *buffer) {
    if (!util_is_directory(src_path))
        util_abort("%s: %s is not a directory \n", __func__, src_path);

    util_make_path(target_path);
    {
        DIR *dirH = opendir(src_path);
        if (dirH == NULL)
            util_abort("%s: failed to open directory:%s / %s \n", __func__,
                       src_path, strerror(errno));

        {
            struct dirent *dp;
            do {
                dp = readdir(dirH);
                if (dp != NULL) {
                    if (dp->d_name[0] != '.') {
                        char *full_src_path =
                            util_alloc_filename(src_path, dp->d_name, NULL);
                        char *full_target_path =
                            util_alloc_filename(target_path, dp->d_name, NULL);
                        if (util_is_file(full_src_path)) {
                            util_copy_file__(full_src_path, full_target_path,
                                             buffer_size, buffer, true);
                        } else {
                            if (util_is_directory(full_src_path) &&
                                !util_is_link(full_src_path))
                                util_copy_directory__(full_src_path,
                                                      full_target_path,
                                                      buffer_size, buffer);
                        }

                        free(full_src_path);
                        free(full_target_path);
                    }
                }
            } while (dp != NULL);
        }
        closedir(dirH);
    }
}

/*  Does not handle symlinks. */
void util_copy_directory_content(const char *src_path,
                                 const char *target_path) {
    int buffer_size = 16 * 1024 * 1024; /* 16 MB */
    void *buffer = util_malloc(buffer_size);

    util_copy_directory__(src_path, target_path, buffer_size, buffer);
    free(buffer);
}

/**
    Equivalent to shell command cp -r src_path target_path
*/

/*  Does not handle symlinks. */

void util_copy_directory(const char *src_path, const char *__target_path) {
    int num_components;
    char **path_parts;
    char *path_tail;
    char *target_path;

    util_path_split(src_path, &num_components, &path_parts);
    path_tail = path_parts[num_components - 1];
    target_path = util_alloc_filename(__target_path, path_tail, NULL);

    util_copy_directory_content(src_path, target_path);

    free(target_path);
    util_free_stringlist(path_parts, num_components);
}
