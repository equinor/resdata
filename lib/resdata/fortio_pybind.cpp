#include <cstdint>

#include <optional>
#include <string>
#include <ios>

#include <pybind11/detail/common.h>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>
#include <pybind11/stl.h>
#include <fmt/format.h>

#include <resdata/FortIO.hpp>

#include "ert/util/util.hpp"

#include <detail/resdata/cwrap_pybind.hpp>

namespace py = pybind11;

enum class Mode {
    READ_MODE = 1,
    WRITE_MODE = 2,
    READ_AND_WRITE_MODE = 3,
    APPEND_MODE = 4,
};

namespace {
PYBIND11_MODULE(fortio, m) {
    register_exceptions(m);
    m.doc() =
        "Module to support transparent binary IO of Fortran created files.\n"
        "\n"
        "In Fortran, when writing binary blobs of data to file the Fortran "
        "runtime will\n"
        "add a header and footer around the data. The Fortran code:\n"
        "\n"
        "   integer array(100)\n"
        "   write(unit) array\n"
        "\n"
        "writes a head and tail in addition to the actual data. The header and "
        "tail are\n"
        "4-byte integers whose value is the number of bytes in the immediately "
        "following\n"
        "record. I.e. what is actually found on disk after the Fortran code "
        "above is:\n"
        "\n"
        "  | 400 | array ...... | 400 |\n"
        "\n"
        "The FortIO.cpp file implements the fortio_type struct which can be "
        "used to read\n"
        "and write these. The current Python module is a minimal wrapping of "
        "this data\n"
        "structure; mainly to support passing FortIO handles to the underlying "
        "cpp\n"
        "functions.\n";

    py::enum_<Mode>(m, "Mode")
        .value("READ_MODE", Mode::READ_MODE)
        .value("WRITE_MODE", Mode::WRITE_MODE)
        .value("READ_AND_WRITE_MODE", Mode::READ_AND_WRITE_MODE)
        .value("APPEND_MODE", Mode::APPEND_MODE)
        .export_values();

    py::class_<ERT::FortIO> cls(
        m, "FortIO",
        "Open a FortIO handle for the given file_name.\n"
        "\n"
        "The newly created FortIO handle will open the underlying FILE*\n"
        "for reading, but if you pass the flag mode=FortIO.WRITE_MODE\n"
        "the file will be opened for writing.\n"
        "\n"
        "Observe that the flag @endian_flip_header will only affect the\n"
        "interpretation of the block size markers in the file, endian\n"
        "flipping of the actual data blocks must be handled at a higher\n"
        "level.\n"
        "\n"
        "When you are finished working with the FortIO instance you can "
        "manually\n"
        "close it with the close() method. Alternatively, the FortIO instance\n"
        "will close automatically when it goes out of scope.\n"
        "\n"
        "Small example script opening a restart file, and then writing\n"
        "all the pressure keywords to another file:\n"
        "\n"
        "   import sys\n"
        "   from resdata.resfile import FortIO, ResdataFile\n"
        "\n"
        "   rst_file = ResdataFile(sys.argv[1])\n"
        "   fortio = FortIO('PRESSURE', mode=FortIO.WRITE_MODE)\n"
        "\n"
        "   for kw in rst_file:\n"
        "       if kw.name() == 'PRESSURE':\n"
        "          kw.fwrite(fortio)\n"
        "\n"
        "   fortio.close()\n"
        "\n"
        "See the documentation of openFortIO() for an alternative\n"
        "method based on a context manager and the with statement.\n");

    cls.def(py::init([](std::string &file_name, Mode mode, bool fmt_file,
                        bool endian_flip_header) {
                auto iosmode = std::ios_base::out;
                if (mode == Mode::READ_MODE)
                    iosmode = std::ios_base::in;
                else if (mode == Mode::WRITE_MODE)
                    iosmode = std::ios_base::out;
                else if (mode == Mode::READ_AND_WRITE_MODE)
                    iosmode = std::ios_base::in | std::ios_base::out;
                else if (mode == Mode::APPEND_MODE)
                    iosmode = std::ios_base::app;
                else
                    throw py::value_error(fmt::format("Unknown mode {}", mode));
                return new ERT::FortIO(file_name, iosmode, fmt_file,
                                       endian_flip_header);
            }),
            py::arg("file_name"), py::arg("mode") = Mode::READ_MODE,
            py::arg("fmt_file") = false, py::arg("endian_flip_header") = true);

    cls.attr("READ_MODE") = Mode::READ_MODE;
    cls.attr("WRITE_MODE") = Mode::WRITE_MODE;
    cls.attr("READ_AND_WRITE_MODE") = Mode::READ_AND_WRITE_MODE;
    cls.attr("APPEND_MODE") = Mode::APPEND_MODE;

    cls.def("close", &ERT::FortIO::close);
    cls.def("get_position", &ERT::FortIO::ftell);
    cls.def(
        "truncate",
        [](ERT::FortIO &self, std::optional<offset_type> size) {
            if (!size)
                size = self.ftell();
            if (!self.ftruncate(*size)) {
                std::string msg = "Truncate of fortran filehandle:" +
                                  std::string(self.filename_ref()) + " failed";
                PyErr_SetString(PyExc_OSError, msg.c_str());
                throw py::error_already_set();
            }
        },
        py::arg("size") = std::nullopt,
        "Will truncate the file to new size.\n"
        "\n"
        "If the method is called without a size argument the stream\n"
        "will be truncated to the current position.\n");

    cls.def("filename",
            [](ERT::FortIO &self) { return std::string(self.filename_ref()); });

    cls.def(
        "seek",
        [](ERT::FortIO &self, offset_type position, int whence = 0) {
            return self.fseek(position, whence);
        },
        py::arg("position"), py::arg("whence") = 0);

    cls.def_static(
        "is_fortran_file",
        [](std::string filename, bool endian_flip = true) {
            return ERT::FortIO::looks_like_fortran_file(filename.c_str(),
                                                        endian_flip);
        },
        py::arg("filename"), py::arg("endian_flip") = true,
        "Will use heuristics to try to guess if @filename is a binary\n"
        "file written in fortran style. ASCII files will return false,\n"
        "even if they are structured as ECLIPSE keywords.\n");
}
} // namespace
