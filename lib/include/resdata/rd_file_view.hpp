#pragma once
#include <ctime>
#include <istream>
#include <iterator>
#include <ostream>
#include <unordered_map>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <optional>

#include <ert/util/int_vector.hpp>

#include <resdata/rd_kw.hpp>
#include <resdata/rd_file_kw.hpp>
#include <resdata/FortIO.hpp>
#include <resdata/rd_file_flag.hpp>
#include <resdata/rd_kw_magic.hpp>

namespace rd {

using inv_map_type = std::unordered_map<const rd_kw_type *, FileKW *>;

struct FileContext {
    ERT::FortIO fortio;
    FileMode flags;
    inv_map_type inv_map;

    FileContext(ERT::FortIO fortio, FileMode flags)
        : fortio(std::move(fortio)), flags(flags) {}
};

class FileView {
    std::vector<std::shared_ptr<FileKW>> kw_list;
    std::map<std::string, std::vector<size_t>> kw_index;
    std::vector<std::string>
        distinct_kw; /* A list of the keywords occuring in the file - each string occurs ONLY ONCE. */
    std::shared_ptr<FileContext> context;
    [[nodiscard]] std::shared_ptr<FileKW> get_file_kw(const std::string &kw,
                                                      size_t ith) const {
        return get_file_kw(kw_index.at(kw).at(ith));
    }
    [[nodiscard]] rd_kw_type *get_kw(const std::shared_ptr<FileKW> &file_kw);
    [[nodiscard]] size_t get_occurence(size_t global_index);

    /** Finds the occurrence of the first block for which @predicate holds for @header_kw.

        Each SEQNUM keyword begins a block that extends to the next SEQNUM, or to
        the end of the file for the last block. The block's header keyword is the
        first @header_kw occurring inside it. @predicate is invoked with that
        keyword; the first block for which it returns true yields its block index.
        Returns std::nullopt if there is no such block.*/
    template <typename Predicate>
    [[nodiscard]] std::optional<size_t> find_block(const std::string &header_kw,
                                                   Predicate predicate) {
        auto seqnum_it = kw_index.find(SEQNUM_KW);
        if (seqnum_it == kw_index.end())
            return std::nullopt;
        auto header_it = kw_index.find(header_kw);
        if (header_it == kw_index.end())
            return std::nullopt;

        const std::vector<size_t> &seqnum_list = seqnum_it->second;
        const std::vector<size_t> &header_list = header_it->second;
        auto header_cursor = header_list.begin();

        for (auto start = seqnum_list.begin(); start != seqnum_list.end();
             ++start) {
            auto next = std::next(start);
            const size_t block_end =
                (next != seqnum_list.end()) ? *next : kw_list.size();

            // Advance to the first header keyword inside this block.
            while (header_cursor != header_list.end() &&
                   *header_cursor < *start)
                ++header_cursor;

            if (header_cursor == header_list.end())
                break;

            if (*header_cursor < block_end && predicate(get_kw(*header_cursor)))
                return static_cast<size_t>(
                    std::distance(seqnum_list.begin(), start));
        }
        return std::nullopt;
    }

public:
    explicit FileView(std::shared_ptr<FileContext> context)
        : context(std::move(context)) {};

    [[nodiscard]] bool has_flags(FileMode flags) const;
    bool drop_flags(FileMode flags);
    void add_flag(FileMode flag);

    [[nodiscard]] const std::string &filename() const;
    [[nodiscard]] std::shared_ptr<FileKW>
    get_file_kw(size_t global_index) const {
        return kw_list.at(global_index);
    }
    [[nodiscard]] auto begin() { return kw_list.begin(); }
    [[nodiscard]] auto end() { return kw_list.end(); }
    [[nodiscard]] std::vector<std::string> get_distinct_kw() const {
        return distinct_kw;
    }
    [[nodiscard]] size_t num_distinct_kw() const { return distinct_kw.size(); }
    [[nodiscard]] size_t size() const { return kw_list.size(); }
    [[nodiscard]] size_t num_named_kw(const std::string &kw) const {
        auto it = kw_index.find(kw);
        return (it != kw_index.end()) ? it->second.size() : 0;
    }

    void add_kw(std::shared_ptr<FileKW> file_kw) {
        kw_list.push_back(std::move(file_kw));
    }

    /** Builds the internal index.

        Must be called every time the content of the kw_list vector is
        modified (otherwise the rd_file instance will be in an
        inconsistent state). */
    void make_index();

    [[nodiscard]] bool has_kw(const std::string &kw) const {
        return kw_index.find(kw) != kw_index.end();
    }
    rd_kw_type *get_kw(size_t index) { return get_kw(get_file_kw(index)); }
    rd_kw_type *get_kw(const std::string &kw, size_t ith) {
        return get_kw(get_file_kw(kw, ith));
    }

    void index_fload_kw(const std::string &kw, int index,
                        const int_vector_type *index_map, char *io_buffer);
    void write(ERT::FortIO &target, size_t offset);

    /** Creates a FileView with keywords from @start_kw to @end_kw.

        Will go from the ith=@occurence keyword named @start_kw to the
        the first keyword named @end_kw. Returns nullptr if there is no such
        start keyword.

        If start_kw==nullopt, goes from the start of the FileView.
        If end_kw==nullopt, goes to the end of the FileView. */
    std::shared_ptr<FileView>
    blockview(const std::optional<std::string> &start_kw = std::nullopt,
              const std::optional<std::string> &end_kw = std::nullopt,
              size_t occurence = 0);

    bool has_report_step(int report_step) {
        return find_block(SEQNUM_KW,
                          [&](const rd_kw_type *seqnum_kw) {
                              return rd_kw_data_equal(seqnum_kw, &report_step);
                          })
            .has_value();
    }
    /** The sim_date of the ith=@seqnum_index step in a restart file.

      throws std::out_of_range if there is no such index. */
    time_t restart_sim_date(size_t seqnum_index);
    /** The number of days since start of the ith=@seqnum_index step in a restart file.

      throws std::out_of_range if there is no such index. */
    double restart_sim_days(size_t seqnum_index);
    /** The kw index of the INTHEAD kw with the given sim_time.

      returns nullopt if there is no such step */
    std::optional<size_t> find_sim_time(time_t sim_time);
    bool has_sim_time(time_t sim_time);

    std::shared_ptr<FileView> restart_view_from_seqnum_index(size_t index);
    std::shared_ptr<FileView> restart_view_from_report_step(int report_step);
    std::shared_ptr<FileView> restart_view_from_sim_time(time_t sim_time);
    std::shared_ptr<FileView> restart_view_from_sim_days(double sim_days);

    std::shared_ptr<FileView> summary_view(int report_step);

    void close();
    /** Write the index of this view to @ostream.

       The stream is expected to have its exception mask configured (e.g.
       std::ios_base::failbit | std::ios_base::badbit) */
    void write_index(std::ostream &ostream) const;

    /** Read a view index from @istream.

       The stream is expected to have its exception mask configured (e.g.
       std::ios_base::failbit | std::ios_base::badbit) */
    static std::shared_ptr<FileView> read(std::shared_ptr<FileContext> context,
                                          std::istream &istream);

    void clear();
};
} // namespace rd
