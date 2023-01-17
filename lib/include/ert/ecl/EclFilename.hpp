#ifndef ERT_ECL_FILEMAME_HPP
#define ERT_ECL_FILEMAME_HPP
#include <string>

#include <ert/ecl/ecl_util.hpp>

namespace ERT {
std::string EclFilename(const std::string &base, ecl_file_enum file_type,
                        int report_step, bool fmt_file = false);
std::string EclFilename(const std::string &base, ecl_file_enum file_type,
                        bool fmt_file = false);

std::string EclFilename(const std::string &path, const std::string &base,
                        ecl_file_enum file_type, int report_step,
                        bool fmt_file = false);
std::string EclFilename(const std::string &path, const std::string &base,
                        ecl_file_enum file_type, bool fmt_file = false);

ecl_file_enum EclFiletype(const std::string &filename);
} // namespace ERT
#endif
