#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>

#include <stdexcept>
#include <system_error>
#include <filesystem>
#include <random>

#include <ert/util/ert_api_config.hpp>

#ifdef ERT_HAVE_GETUID
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <paths.h>
#endif

#include <ert/util/util.hpp>
#include <ert/util/test_work_area.hpp>

#include "detail/util/path.hpp"

/*
  This file implements a small work area implementation to be used for
  tests which write to disk. The implementation works by creating a
  work area in /tmp and then call chdir() change the current working
  directory.

  An important aspect of this implementation is that test output from
  different users should not come in conflict with e.g. permission
  problems, to achieve this the directories created will be per user.

  When creating the work area you pass in a boolean flag whether you
  want the area to be stored when the destructor is called. After the
  the work_area is destroyed the cwd is changed back to the value it
  had before the area was created.

  The functions test_work_area_install_file(),
  test_work_area_copy_directory() and
  test_work_area_copy_directory_content() can be used to populate the
  work area with files and directories needed for the test
  execution. These functions have some intelligence when it comes to
  interpreting relative paths; a relative path input argument is
  interpreted relative to the original cwd.

  Basic usage example:
  --------------------


  -- Create directory /tmp/$USER/ert-test/my/example/test and call
  -- chdir() to the newly created directory.
  test_work_area_type * work_area = test_work_area_alloc("my/example/test" , true);

  -- Make files available from the test directory.
  test_work_area_install_file(work_area , "/home/user/build/test-data/file1");
  test_work_area_install_file(work_area , "relative/path/file2");

  -- Recursively copy directory and directory content into test area:
  test_work_area_copy_directory(work_area , "/home/user/build/test-data/case1");

  ...
  -- Do normal test operations
  ...

  -- Destroy test_work_area structure; since the work_area is created
  -- with input flag @store set to true the are on disk will not be
  -- cleared. After the test_work_area_free( ) function has been
  -- called the original cwd will be restored.

  test_work_area_free( work_area );
*/

#define DEFAULT_STORE false

#define TEST_PATH_FMT                                                          \
    "%s/test/%s/%08d"         /* username/test/test_name/random-integer */
#define FULL_PATH_FMT "%s/%s" /* prefix/test-path */

#define TEST_WORK_AREA_TYPE_ID 1107355

static std::string test_work_area_prefix() {
#ifdef HAVE_WINDOWS_GET_TEMP_PATH

    char tmp_path[MAX_PATH];
    GetTempPath(MAX_PATH, tmp_path);
    return tmp_path;

#else

    const char *prefix_path = getenv("TMPDIR");

#ifdef P_tmpdir
    if (!prefix_path)
        prefix_path = P_tmpdir;
#endif

    if (!prefix_path)
        prefix_path = _PATH_TMP;

    return std::filesystem::canonical(prefix_path).string();

#endif
}

namespace rd {
namespace util {

static bool test_work_area_copy_parent__(const TestArea *work_area,
                                         const std::string &input_path,
                                         bool copy_content) {
    std::filesystem::path full_path = input_path;
    if (full_path.is_relative())
        full_path = work_area->original_cwd() / full_path;

    std::error_code ec;
    if (!std::filesystem::exists(full_path, ec))
        return false;

    full_path = full_path.lexically_normal();
    if (!full_path.has_filename()) /* Trailing path separator. */
        full_path = full_path.parent_path();
    const std::string parent_path = full_path.parent_path().string();

    if (copy_content)
        work_area->copy_directory_content(parent_path);
    else
        work_area->copy_directory(parent_path);

    return true;
}
static std::mt19937 generator(std::random_device{}());

static std::string create_test_path(const std::string &test_name,
                                    const char *prefix, const char *user_name) {
    const int MAX_TRIES = 10;
    for (int try_count = 0; try_count < MAX_TRIES; try_count++) {
        unsigned int random_int = static_cast<unsigned int>(generator());
        random_int = random_int % 100000000;

        std::unique_ptr<char, decltype(&std::free)> test_path{
            util_alloc_sprintf(TEST_PATH_FMT, user_name, test_name.c_str(),
                               random_int),
            &std::free};

        std::unique_ptr<char, decltype(&std::free)> test_cwd{
            util_alloc_sprintf(FULL_PATH_FMT, prefix, test_path.get()),
            &std::free};

        if (util_mkdir_p(test_cwd.get()))
            return test_cwd.get();
    }

    throw std::runtime_error(
        std::string(__func__) + ": failed to make test directory after " +
        std::to_string(MAX_TRIES) + " tries: " + test_name);
}

TestArea::TestArea(const std::string &test_name, bool store_area)
    : store(store_area) {
    const std::string prefix = test_work_area_prefix();

#ifdef ERT_HAVE_GETUID
    uid_t uid = getuid();
    struct passwd *pw = getpwuid(uid);
    std::unique_ptr<char, decltype(&std::free)> user_name{
        util_alloc_string_copy(pw->pw_name), &std::free};
#else
    std::unique_ptr<char, decltype(&std::free)> user_name{
        util_alloc_sprintf("ert-test-%08u",
                           static_cast<unsigned int>(generator())),
        &std::free};
#endif

    std::string test_cwd =
        rd::util::create_test_path(test_name, prefix.c_str(), user_name.get());

    {
        std::unique_ptr<char, decltype(&std::free)> cwd_tmp{util_alloc_cwd(),
                                                            &std::free};
        this->org_cwd = cwd_tmp.get();
    }
    if (util_chdir(test_cwd.c_str()) != 0)
        throw std::runtime_error(
            std::string(__func__) +
            ": Failed to move into temporary directory: " + test_cwd);
    this->cwd = test_cwd;
}

TestArea::~TestArea() {
    util_chdir(this->org_cwd.c_str());
    if (!this->store) {
        std::error_code ec;
        std::filesystem::remove_all(this->cwd, ec);
        // silently fail for backwards compatibility
    }
}

const std::string &TestArea::test_cwd() const { return this->cwd; }

const std::string &TestArea::original_cwd() const { return this->org_cwd; }

std::string TestArea::original_path(const std::string &input_path) const {
    if (util_is_abs_path(input_path.c_str()))
        return std::string(input_path);
    else {
        std::unique_ptr<char, decltype(&std::free)> fname{
            util_alloc_filename(this->original_cwd().c_str(),
                                input_path.c_str(), NULL),
            &std::free};
        return std::string(fname.get());
    }
}

void TestArea::copy_file(const std::string &input_src_file) const {
    std::string src_file = this->original_path(input_src_file);

    if (util_file_exists(src_file.c_str())) {
        std::unique_ptr<char, decltype(&std::free)> target_name{
            util_split_alloc_filename(input_src_file.c_str()), &std::free};
        std::unique_ptr<char, decltype(&std::free)> target_file{
            util_alloc_filename(this->test_cwd().c_str(), target_name.get(),
                                NULL),
            &std::free};
        util_copy_file(src_file.c_str(), target_file.get());
    }
}

/* Recursively copies the content of src_path into target_path; hidden entries
   and symlinked directories are skipped. */
static void copy_directory_content__(const std::filesystem::path &src_path,
                                     const std::filesystem::path &target_path) {
    if (!std::filesystem::is_directory(src_path))
        throw std::invalid_argument(src_path.string() + " is not a directory");

    std::filesystem::create_directories(target_path);
    for (const auto &entry : std::filesystem::directory_iterator(src_path)) {
        const std::filesystem::path name = entry.path().filename();
        if (name.string()[0] == '.')
            continue;

        if (entry.is_regular_file())
            std::filesystem::copy_file(
                entry.path(), target_path / name,
                std::filesystem::copy_options::overwrite_existing);
        else if (entry.is_directory() && !entry.is_symlink())
            copy_directory_content__(entry.path(), target_path / name);
    }
}

void TestArea::copy_directory(const std::string input_directory) const {
    std::filesystem::path src_directory = this->original_path(input_directory);
    std::filesystem::path name = src_directory.filename();
    if (name.empty())
        name = src_directory.parent_path().filename();
    copy_directory_content__(src_directory, this->test_cwd() / name);
}

void TestArea::copy_directory_content(const std::string input_directory) const {
    copy_directory_content__(this->original_path(input_directory),
                             this->test_cwd());
}

bool TestArea::copy_parent(const std::string input_path) const {
    return test_work_area_copy_parent__(this, input_path, false);
}

bool TestArea::copy_parent_content(const std::string input_path) const {
    return test_work_area_copy_parent__(this, input_path, true);
}

} // namespace util
} // namespace rd

test_work_area_type *test_work_area_alloc__(const char *test_name,
                                            bool store_area) {
    if (test_name)
        return new rd::util::TestArea(test_name, store_area);
    else
        return NULL;
}

test_work_area_type *test_work_area_alloc(const char *test_name) {
    return test_work_area_alloc__(test_name, false);
}

void test_work_area_free(test_work_area_type *work_area) { delete work_area; }

const char *test_work_area_get_cwd(const test_work_area_type *work_area) {
    return work_area->test_cwd().c_str();
}

const char *
test_work_area_get_original_cwd(const test_work_area_type *work_area) {
    return work_area->original_cwd().c_str();
}

/**
   The point of this function is that the test code should be able to
   access the file @input_file independent of the fact that it has
   changed path. If @input_file is an absolute path the function will
   do nothing, if @input_file is a relative path the function will
   copy @input_file from the location relative to the original cwd to
   the corresponding location relative to the test cwd.
*/
void test_work_area_install_file(const test_work_area_type *work_area,
                                 const char *input_src_file) {
    if (util_is_abs_path(input_src_file))
        return;
    else {
        std::string src_file = work_area->original_path(input_src_file);
        std::string src_path = rd::util::path::dirname(input_src_file);

        if (!util_entry_exists(src_path.c_str()))
            util_make_path(src_path.c_str());

        if (util_file_exists(src_file.c_str())) {
            std::unique_ptr<char, decltype(&std::free)> target_file{
                util_alloc_filename(work_area->test_cwd().c_str(),
                                    input_src_file, NULL),
                &std::free};
            util_copy_file(src_file.c_str(), target_file.get());
        }
    }
}

void test_work_area_copy_file(const test_work_area_type *work_area,
                              const char *input_file) {
    if (input_file)
        work_area->copy_file(std::string(input_file));
}

void test_work_area_copy_directory(const test_work_area_type *work_area,
                                   const char *input_directory) {
    work_area->copy_directory(std::string(input_directory));
}

void test_work_area_copy_directory_content(const test_work_area_type *work_area,
                                           const char *input_directory) {
    work_area->copy_directory_content(std::string(input_directory));
}

bool test_work_area_copy_parent_directory(const test_work_area_type *work_area,
                                          const char *input_path) {
    return work_area->copy_parent(std::string(input_path));
}

bool test_work_area_copy_parent_content(const test_work_area_type *work_area,
                                        const char *input_path) {
    return work_area->copy_parent_content(std::string(input_path));
}
