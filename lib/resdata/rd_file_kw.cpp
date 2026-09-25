#include <algorithm>
#include <array>
#include <cstddef>
#include <ios>
#include <istream>
#include <limits>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <variant>
#include <fmt/format.h>

#include <ert/util/util.hpp>

#include <resdata/rd_kw.hpp>
#include <resdata/rd_kw_header.hpp>
#include <resdata/rd_file_kw.hpp>
#include <resdata/FortIO.hpp>
#include <resdata/rd_type.hpp>

template <class... Ts> struct overload : Ts... {
    using Ts::operator()...;
};
template <class... Ts> overload(Ts...) -> overload<Ts...>;

rd::KW *FileKW::get_kw(ERT::FortIO &fortio) {
    return std::visit(
        overload{[&](rd::KWHeader &header) {
                     if (!fortio.assert_stream_open())
                         throw std::ios_base::failure(
                             std::string(__func__) +
                             ": trying to load a keyword after the backing "
                             "file has been detached.");

                     fortio.fseek(file_offset, SEEK_SET);
                     auto file_header = rd::KWHeader::fread(fortio);
                     if (header != file_header)
                         throw std::runtime_error(fmt::format(
                             "{}: mismatch between header and file: "
                             "expected name=\"{}\" size={} type={}, got "
                             "name=\"{}\" size={} type={}.",
                             __func__, header.name(), header.size(),
                             rd_type_name(header.data_type()),
                             file_header.name(), file_header.size(),
                             rd_type_name(file_header.data_type())));
                     auto data = file_header.fread_data(fortio);
                     this->kw.emplace<rd::KW>(std::move(header),
                                              std::move(data));
                     return &std::get<rd::KW>(this->kw);
                 },
                 [](rd::KW &kw) { return &kw; }},
        kw);
}

bool FileKW::skip_data(ERT::FortIO &fortio) const {
    return std::visit([&fortio](auto &kw) { return kw.fskip_data(fortio); },
                      kw);
}

void FileKW::inplace_write(ERT::FortIO &fortio) const {
    const rd::KW *rd_kw = std::get_if<rd::KW>(&this->kw);
    if (!rd_kw)
        throw std::runtime_error(
            "cannot write FileKW in place: keyword has not been loaded");
    fortio.fseek(file_offset, SEEK_SET);
    rd::KW::fskip_header(fortio);
    fortio.fclean();
    rd_kw->fwrite_data(fortio);
}

void FileKW::write_header(std::ostream &stream) const {
    std::string header = get_header();
    size_t header_length = header.size();
    for (size_t i = 0; i < RD_STRING8_LENGTH; i++) {
        if (i < header_length)
            stream.put(header[i]);
        else
            stream.put(' ');
    }

    auto data_type = get_data_type();
    size_t size = get_size();
    if (size > std::numeric_limits<int>::max())
        throw std::invalid_argument(
            fmt::format("Size of rd_kw exceeded int max: {}", size));
    int kw_size = static_cast<int>(size);
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

void FileKW::clear() {
    std::visit(overload{[](rd::KWHeader &header) { return; },
                        [this](rd::KW &kw) {
                            this->kw.emplace<rd::KWHeader>(kw.header());
                        }},
               kw);
}

rd::KW *FileKW::get_kw_ptr() {
    return std::visit(overload{[](rd::KWHeader &header) {
                                   return static_cast<rd::KW *>(nullptr);
                               },
                               [](rd::KW &kw) { return &kw; }},
                      kw);
};
