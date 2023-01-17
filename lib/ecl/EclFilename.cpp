#include <string>
#include <stdexcept>

#include <ert/ecl/EclFilename.hpp>

namespace ERT {

std::string EclFilename(const std::string &path, const std::string &base,
                        ecl_file_enum file_type, int report_step,
                        bool fmt_file) {
    char *tmp = ecl_util_alloc_filename(path.c_str(), base.c_str(), file_type,
                                        fmt_file, report_step);
    std::string retval = tmp;
    free(tmp);
    return retval;
}

std::string EclFilename(const std::string &base, ecl_file_enum file_type,
                        int report_step, bool fmt_file) {
    char *tmp = ecl_util_alloc_filename(nullptr, base.c_str(), file_type,
                                        fmt_file, report_step);
    std::string retval = tmp;
    free(tmp);
    return retval;
}

namespace {
bool require_report_step(ecl_file_enum file_type) {
    if ((file_type == ECL_RESTART_FILE) || (file_type == ECL_SUMMARY_FILE))
        return true;
    else
        return false;
}
} // namespace

std::string EclFilename(const std::string &path, const std::string &base,
                        ecl_file_enum file_type, bool fmt_file) {
    if (require_report_step(file_type))
        throw std::runtime_error(
            "Must use overload with report step for this file type");
    else {
        char *tmp = ecl_util_alloc_filename(path.c_str(), base.c_str(),
                                            file_type, fmt_file, -1);
        std::string retval = tmp;
        free(tmp);
        return retval;
    }
}

std::string EclFilename(const std::string &base, ecl_file_enum file_type,
                        bool fmt_file) {
    if (require_report_step(file_type))
        throw std::runtime_error(
            "Must use overload with report step for this file type");
    else {
        char *tmp = ecl_util_alloc_filename(nullptr, base.c_str(), file_type,
                                            fmt_file, -1);
        std::string retval = tmp;
        free(tmp);
        return retval;
    }
}

ecl_file_enum EclFiletype(const std::string &filename) {
    return ecl_util_get_file_type(filename.c_str(), nullptr, nullptr);
}

} // namespace ERT
