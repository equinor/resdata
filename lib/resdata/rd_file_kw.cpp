#include <algorithm>
#include <array>
#include <cstddef>
#include <ios>
#include <istream>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <ert/util/util.hpp>

#include <resdata/rd_kw.hpp>
#include <resdata/rd_file_kw.hpp>
#include <resdata/FortIO.hpp>
#include <resdata/rd_type.hpp>

void FileKW::assert_kw() const {
    if (!kw)
        throw std::runtime_error("keyword could not be loaded from file "
                                 "(rd_kw_fread_alloc returned NULL)");

    if (!rd_type_is_equal(this->data_type, rd_kw_get_data_type(kw.get())))
        throw std::runtime_error(std::string(__func__) +
                                 ": type mismatch between header and file.");

    if (kw_size != rd_kw_get_size(kw.get()))
        throw std::runtime_error(std::string(__func__) +
                                 ": size mismatch between header and file.");

    if (header != rd_kw_get_header(kw.get()))
        throw std::runtime_error(std::string(__func__) +
                                 ": name mismatch between header and file.");
}

void FileKW::load_kw(ERT::FortIO &fortio) {
    if (!fortio.assert_stream_open())
        throw std::ios_base::failure(
            std::string(__func__) +
            ": trying to load a keyword after the backing file has "
            "been detached.");

    fortio.fseek(file_offset, SEEK_SET);
    // Note load_kw is only called when kw is nullptr
    kw.reset(rd_kw_fread_alloc(fortio));
    assert_kw();
}

rd_kw_type *FileKW::get_kw(ERT::FortIO &fortio) {
    if (!kw)
        load_kw(fortio);

    return kw.get();
}

bool FileKW::skip_data(ERT::FortIO &fortio) const {
    return rd_kw_fskip_data__(data_type, kw_size, fortio);
}

void FileKW::inplace_write(ERT::FortIO &fortio) const {
    assert_kw();
    fortio.fseek(file_offset, SEEK_SET);
    rd_kw_fskip_header(fortio);
    fortio.fclean();
    rd_kw_fwrite_data(kw.get(), fortio);
}

void FileKW::write_header(std::ostream &stream) const {
    size_t header_length = header.size();
    for (size_t i = 0; i < RD_STRING8_LENGTH; i++) {
        if (i < header_length)
            stream.put(header[i]);
        else
            stream.put(' ');
    }

    int type = rd_type_get_type(data_type);
    size_t type_size = rd_type_get_sizeof_iotype(data_type);
    stream.write(reinterpret_cast<const char *>(&kw_size), sizeof(kw_size));
    stream.write(reinterpret_cast<const char *>(&file_offset),
                 sizeof(file_offset));
    stream.write(reinterpret_cast<const char *>(&type), sizeof(type));
    stream.write(reinterpret_cast<const char *>(&type_size), sizeof(type_size));
}

std::vector<std::shared_ptr<FileKW>> FileKW::read(std::istream &stream,
                                                  size_t num) {
    std::vector<std::shared_ptr<FileKW>> kw_list;
    kw_list.reserve(num);

    for (size_t ikw = 0; ikw < num; ikw++) {
        std::array<char, RD_STRING8_LENGTH> header_buf{};
        stream.read(header_buf.data(), header_buf.size());
        auto header_end = std::find(header_buf.begin(), header_buf.end(), ' ');
        std::string header(header_buf.begin(), header_end);

        int kw_size = 0;
        offset_type file_offset = 0;
        int type = 0;
        size_t type_size = 0;
        stream.read(reinterpret_cast<char *>(&kw_size), sizeof(kw_size));
        stream.read(reinterpret_cast<char *>(&file_offset),
                    sizeof(file_offset));
        stream.read(reinterpret_cast<char *>(&type), sizeof(type));
        stream.read(reinterpret_cast<char *>(&type_size), sizeof(type_size));

        kw_list.push_back(std::make_shared<FileKW>(
            file_offset,
            rd_type_create(static_cast<rd_type_enum>(type), type_size), kw_size,
            std::move(header)));
    }
    return kw_list;
}

void FileKW::clear() { kw.reset(nullptr); }
