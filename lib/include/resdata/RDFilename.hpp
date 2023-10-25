#ifndef ERT_RD_FILEMAME_HPP
#define ERT_RD_FILEMAME_HPP
#include <string>

#include <resdata/rd_util.hpp>

namespace ERT {
std::string RDFilename(const std::string &base, rd_file_enum file_type,
                       int report_step, bool fmt_file = false);
std::string RDFilename(const std::string &base, rd_file_enum file_type,
                       bool fmt_file = false);

std::string RDFilename(const std::string &path, const std::string &base,
                       rd_file_enum file_type, int report_step,
                       bool fmt_file = false);
std::string RDFilename(const std::string &path, const std::string &base,
                       rd_file_enum file_type, bool fmt_file = false);

rd_file_enum RDFiletype(const std::string &filename);
} // namespace ERT
#endif
