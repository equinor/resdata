#include <stdlib.h>
#include <string.h>

#include <ert/util/util.hpp>

#include <dirent.h>
#include <sys/types.h>
#include <unistd.h>

/**
   Only removes the last component in path.
*/
void static util_clear_directory__(const char *path, bool strict_uid,
                                   bool unlink_root) {
    if (util_is_directory(path)) {
        DIR *dirH = opendir(path);

        if (dirH != NULL) {
            const uid_t uid = getuid();
            struct dirent *dentry;

            while ((dentry = readdir(dirH)) != NULL) {
                stat_type buffer;
                mode_t mode;
                const char *entry_name = dentry->d_name;
                if ((strcmp(entry_name, ".") != 0) &&
                    (strcmp(entry_name, "..") != 0)) {
                    char *full_path =
                        util_alloc_filename(path, entry_name, NULL);

                    if (lstat(full_path, &buffer) == 0) {
                        mode = buffer.st_mode;

                        if (S_ISDIR(mode))
                            /*
                Recursively descending into sub directory.
              */
                            util_clear_directory__(full_path, strict_uid, true);
                        else if (S_ISLNK(mode))
                            /*
                Symbolic links are unconditionally removed.
              */
                            unlink(full_path);
                        else if (S_ISREG(mode)) {
                            /*
                 It is a regular file - we remove it (if we own it!).
              */
                            if ((!strict_uid) || (buffer.st_uid == uid)) {
                                int unlink_return = unlink(full_path);
                                if (unlink_return != 0) {
                                    /* Unlink failed */
                                }
                            }
                        }
                    }
                    free(full_path);
                }
            }
        }
        closedir(dirH);

        /* Finish with clearing the root directory */
        if (unlink_root) {
            int rmdir_return = rmdir(path);
            if (rmdir_return != 0) {
                /* Unlink failed */
            }
        }
    }
}

/**
   This function will clear away all the contents (including
   subdirectories) in the directory @path.

   If the parameter @strict_uid is set to true, the function will only
   attempt to remove entries where the calling uid is also the owner
   of the entry.

   If the parameter @unlink_root is true the directory @path will also
   be removed, otherwise it will be left as an empty directory.

   The function will just go about deleting as much as it can; errors
   are not signalled in any way!

   The function is in util_getuid() because uid_t and getuid() are so
   important elements of the function.
*/

void util_clear_directory(const char *path, bool strict_uid, bool unlink_root) {
    util_clear_directory__(path, strict_uid, unlink_root);
}
