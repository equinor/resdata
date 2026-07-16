#pragma once
#include <cstddef>
#include <istream>
#include <memory>
#include <utility>
#include <ostream>
#include <vector>
#include <string>

#include <ert/util/util.hpp>

#include <resdata/rd_kw.hpp>
#include <resdata/FortIO.hpp>
#include "resdata/rd_type.hpp"

/** FileKW holds the header information (name, size, type) for an rd_kw
    and the offset in a file containing the keyword.

    If and when the keyword is actually queried for, the
    get_kw() method will seek to the keyword position in an
    open fortio instance and read the rd_kw. */
class FileKW {
    offset_type file_offset;
    rd_data_type data_type;
    int kw_size;
    std::string header;
    rd_kw_ptr kw{nullptr, &rd_kw_free};

    void assert_kw() const;
    void load_kw(ERT::FortIO &fortio);

public:
    FileKW(offset_type file_offset, rd_data_type data_type, int kw_size,
           std::string header)
        : file_offset(file_offset), data_type(data_type), kw_size(kw_size),
          header(std::move(header)) {};
    /** Create a new FileKW based on header information from
        the input keyword.

        Typically only the header has been loaded from the keyword.

        It is the users responsibility that the @offset argument comes
        from the same fortio instance as used when calling get_kw().*/
    FileKW(const rd_kw_type *rd_kw, offset_type offset)
        : FileKW(offset, rd_kw_get_data_type(rd_kw), rd_kw_get_size(rd_kw),
                 rd_kw_get_header(rd_kw)) {}
    [[nodiscard]] bool operator==(const FileKW &other) const {
        if (file_offset != other.file_offset)
            return false;

        if (kw_size != other.kw_size)
            return false;

        if (!rd_type_is_equal(data_type, other.data_type))
            return false;

        return header == other.header;
    }
    [[nodiscard]] const std::string &get_header() const { return header; };
    [[nodiscard]] int get_size() const { return kw_size; };
    [[nodiscard]] offset_type get_offset() const { return file_offset; };
    [[nodiscard]] rd_data_type get_data_type() const { return data_type; };

    /** The rd_kw, if one is read, otherwise returns nullptr. */
    [[nodiscard]] rd_kw_type *get_kw_ptr() const { return kw.get(); };

    /** Return the rd_kw. If it is not loaded, the method will read it
       from @fortio. The kw is then cached. */
    rd_kw_type *get_kw(ERT::FortIO &fortio);

    bool skip_data(ERT::FortIO &fortio) const;
    /** Read @num keyword headers from @stream.

       The stream is expected to have its exception mask configured (e.g.
       std::ios_base::failbit | std::ios_base::badbit) */
    static std::vector<std::shared_ptr<FileKW>> read(std::istream &stream,
                                                     size_t num);

    /** Clear the cached kw. Note: previous pointers to
       kws are invalidated. */
    void clear();

    /** Overwrite the file contents with the new content of the rd_kw. */
    void inplace_write(ERT::FortIO &fortio) const;
    /** Write this keyword's header to @stream.

       The stream is expected to have its exception mask configured (e.g.
       std::ios_base::failbit | std::ios_base::badbit) */
    void write_header(std::ostream &stream) const;
};
