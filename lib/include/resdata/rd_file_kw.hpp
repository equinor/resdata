#pragma once
#include <cstddef>
#include <istream>
#include <memory>
#include <utility>
#include <ostream>
#include <variant>
#include <vector>
#include <string>

#include <ert/util/util.hpp>

#include <resdata/rd_kw.hpp>
#include <resdata/rd_kw_header.hpp>
#include <resdata/FortIO.hpp>
#include "resdata/rd_type.hpp"

/** FileKW holds the header information (name, size, type) for an rd_kw
    and the offset in a file containing the keyword.

    If and when the keyword is actually queried for, the
    get_kw() method will seek to the keyword position in an
    open fortio instance and read the rd_kw. */
class FileKW {
    offset_type file_offset;
    std::variant<rd::KWHeader, rd::KW> kw;

public:
    FileKW(offset_type file_offset, rd_data_type data_type, size_t kw_size,
           std::string name)
        : file_offset(file_offset),
          kw(rd::KWHeader{kw_size, data_type, std::move(name)}) {};
    /** Create a new FileKW based on header information from
        the input keyword.

        Typically only the header has been loaded from the keyword.

        It is the users responsibility that the @offset argument comes
        from the same fortio instance as used when calling get_kw().*/
    FileKW(const rd::KW *rd_kw, offset_type offset)
        : FileKW(offset, rd_kw->data_type(), rd::kw_get_size(rd_kw),
                 rd_kw->name()) {}
    [[nodiscard]] bool operator==(const FileKW &other) const {
        if (file_offset != other.file_offset)
            return false;

        if (get_size() != other.get_size())
            return false;

        if (!rd_type_is_equal(get_data_type(), other.get_data_type()))
            return false;

        return get_header() == other.get_header();
    }
    [[nodiscard]] const std::string &get_header() const {
        return std::visit(
            [](const auto &kw) -> const std::string & { return kw.name(); },
            kw);
    };
    [[nodiscard]] size_t get_size() const {
        return std::visit([](auto &kw) { return kw.size(); }, kw);
    };
    [[nodiscard]] offset_type get_offset() const { return file_offset; };
    [[nodiscard]] rd_data_type get_data_type() const {
        return std::visit([](auto &kw) { return kw.data_type(); }, kw);
    };

    /** The rd_kw, if one is read, otherwise returns nullptr. */
    [[nodiscard]] rd::KW *get_kw_ptr();

    /** Return the rd_kw. If it is not loaded, the method will read it
       from @fortio. The kw is then cached. */
    rd::KW *get_kw(ERT::FortIO &fortio);

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
