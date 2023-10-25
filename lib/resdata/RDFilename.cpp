#include <string>
#include <stdexcept>

#include <resdata/RDFilename.hpp>

namespace ERT {

std::string RDFilename(const std::string &path, const std::string &base,
                       rd_file_enum file_type, int report_step, bool fmt_file) {
    char *tmp = rd_alloc_filename(path.c_str(), base.c_str(), file_type,
                                  fmt_file, report_step);
    std::string retval = tmp;
    free(tmp);
    return retval;
}

std::string RDFilename(const std::string &base, rd_file_enum file_type,
                       int report_step, bool fmt_file) {
    char *tmp = rd_alloc_filename(nullptr, base.c_str(), file_type, fmt_file,
                                  report_step);
    std::string retval = tmp;
    free(tmp);
    return retval;
}

namespace {
bool require_report_step(rd_file_enum file_type) {
    if ((file_type == RD_RESTART_FILE) || (file_type == RD_SUMMARY_FILE))
        return true;
    else
        return false;
}
} // namespace

std::string RDFilename(const std::string &path, const std::string &base,
                       rd_file_enum file_type, bool fmt_file) {
    if (require_report_step(file_type))
        throw std::runtime_error(
            "Must use overload with report step for this file type");
    else {
        char *tmp = rd_alloc_filename(path.c_str(), base.c_str(), file_type,
                                      fmt_file, -1);
        std::string retval = tmp;
        free(tmp);
        return retval;
    }
}

std::string RDFilename(const std::string &base, rd_file_enum file_type,
                       bool fmt_file) {
    if (require_report_step(file_type))
        throw std::runtime_error(
            "Must use overload with report step for this file type");
    else {
        char *tmp =
            rd_alloc_filename(nullptr, base.c_str(), file_type, fmt_file, -1);
        std::string retval = tmp;
        free(tmp);
        return retval;
    }
}

rd_file_enum RDFiletype(const std::string &filename) {
    return rd_get_file_type(filename.c_str(), nullptr, nullptr);
}

} // namespace ERT
