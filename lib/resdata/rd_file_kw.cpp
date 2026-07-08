#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ios>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <ert/util/size_t_vector.hpp>
#include <ert/util/util.hpp>

#include <resdata/rd_kw.hpp>
#include <resdata/rd_file_kw.hpp>
#include <resdata/FortIO.hpp>
#include <ert/util/perm_vector.hpp>
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

void FileKW::write_header(FILE *stream) {
    size_t header_length = header.size();
    for (size_t i = 0; i < RD_STRING8_LENGTH; i++) {
        if (i < header_length)
            fputc(header[i], stream);
        else
            fputc(' ', stream);
    }

    util_fwrite_int(kw_size, stream);
    util_fwrite_offset(file_offset, stream);
    util_fwrite_int(rd_type_get_type(data_type), stream);
    util_fwrite_size_t(rd_type_get_sizeof_iotype(data_type), stream);
}

std::vector<std::shared_ptr<FileKW>> FileKW::read(FILE *stream, int num) {

    size_t file_kw_size = RD_STRING8_LENGTH + 2 * sizeof(int) +
                          sizeof(offset_type) + sizeof(size_t);
    size_t buffer_size = num * file_kw_size;
    std::unique_ptr<char, decltype(&std::free)> buffer(
        (char *)util_malloc(buffer_size), &std::free);
    size_t num_read = fread(buffer.get(), 1, buffer_size, stream);

    if (num_read != buffer_size) {
        throw std::runtime_error("error reading rd_file_type index file");
    }

    std::vector<std::shared_ptr<FileKW>> kw_list(num);
    for (int ikw = 0; ikw < num; ikw++) {
        int buffer_offset = ikw * file_kw_size;
        char header[RD_STRING8_LENGTH + 1];
        int kw_size;
        offset_type file_offset;
        rd_type_enum rd_type;
        size_t type_size;
        {
            int index = 0;
            while (true) {
                if (buffer.get()[index + buffer_offset] != ' ')
                    header[index] = buffer.get()[index + buffer_offset];
                else
                    break;

                index++;
                if (index == RD_STRING8_LENGTH)
                    break;
            }
            header[index] = '\0';
            buffer_offset += RD_STRING8_LENGTH;
        }

        memcpy(&kw_size, &buffer.get()[buffer_offset], sizeof kw_size);
        buffer_offset += sizeof kw_size;

        memcpy(&file_offset, &buffer.get()[buffer_offset], sizeof file_offset);
        buffer_offset += sizeof file_offset;

        memcpy(&rd_type, &buffer.get()[buffer_offset], sizeof rd_type);
        buffer_offset += sizeof rd_type;

        memcpy(&type_size, &buffer.get()[buffer_offset], sizeof type_size);
        buffer_offset += sizeof type_size;

        kw_list[ikw] = std::make_shared<FileKW>(
            file_offset, rd_type_create(rd_type, type_size), kw_size, header);
    }
    return kw_list;
}

void FileKW::clear() { kw.reset(nullptr); }
