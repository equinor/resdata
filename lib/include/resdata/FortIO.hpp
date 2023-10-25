#ifndef OPM_ERT_FORTIO_KW
#define OPM_ERT_FORTIO_KW

#include <fstream>
#include <string>
#include <memory>

#include <resdata/fortio.h>
#include <resdata/rd_endian_flip.hpp>

#include <ert/util/ert_unique_ptr.hpp>

namespace ERT {
class FortIO {
public:
    FortIO();
    FortIO(const std::string &filename, std::ios_base::openmode mode,
           bool fmt_file = false, bool endian_flip_header = RD_ENDIAN_FLIP);
    void open(const std::string &filename, std::ios_base::openmode mode,
              bool fmt_file = false, bool endian_flip_header = RD_ENDIAN_FLIP);
    void fflush() const;
    bool ftruncate(offset_type new_size);

    fortio_type *get() const;
    void close();

private:
    ert_unique_ptr<fortio_type, fortio_fclose> m_fortio;
};
} // namespace ERT

#endif
