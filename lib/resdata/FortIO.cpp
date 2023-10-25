#include <stdexcept>

#include <ert/util/util.h>

#include <resdata/FortIO.hpp>

namespace ERT {

FortIO::FortIO() {}

FortIO::FortIO(const std::string &filename, std::ios_base::openmode mode,
               bool fmt_file, bool endian_flip_header) {
    open(filename, mode, fmt_file, endian_flip_header);
}

void FortIO::open(const std::string &filename, std::ios_base::openmode mode,
                  bool fmt_file, bool endian_flip_header) {
    if (mode == std::ios_base::in) {
        if (util_file_exists(filename.c_str())) {
            fortio_type *c_ptr = fortio_open_reader(filename.c_str(), fmt_file,
                                                    endian_flip_header);
            m_fortio.reset(c_ptr);
        } else
            throw std::invalid_argument("File " + filename + " does not exist");
    } else if (mode == std::ios_base::app) {
        fortio_type *c_ptr =
            fortio_open_append(filename.c_str(), fmt_file, endian_flip_header);
        m_fortio.reset(c_ptr);
    } else {
        fortio_type *c_ptr =
            fortio_open_writer(filename.c_str(), fmt_file, endian_flip_header);
        m_fortio.reset(c_ptr);
    }
}

void FortIO::close() {
    if (m_fortio)
        m_fortio.reset();
}

fortio_type *FortIO::get() const { return m_fortio.get(); }

void FortIO::fflush() const { fortio_fflush(m_fortio.get()); }

bool FortIO::ftruncate(offset_type new_size) {
    return fortio_ftruncate(m_fortio.get(), new_size);
}
} // namespace ERT
