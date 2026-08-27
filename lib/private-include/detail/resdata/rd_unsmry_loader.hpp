#include <ctime>
#include <memory>
#include <string>
#include <vector>

#include <resdata/rd_smspec.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/rd_file_flag.hpp>
#include <resdata/rd_file_view.hpp>

namespace rd {

class unsmry_loader {
public:
    unsmry_loader(const rd_smspec_type *smspec, const std::string &filename,
                  FileMode file_options = FileMode::DEFAULT);

    std::vector<double> get_vector(size_t pos) const;
    std::vector<double> sim_seconds() const;
    std::vector<time_t> sim_time() const;
    size_t length() const;

    std::vector<size_t> report_steps(size_t offset) const;
    double iget(size_t time_index, size_t params_index) const;

private:
    size_t size; //Number of entries in the smspec index
    rd::TimeInfo time_info;
    time_t sim_start;
    size_t m_length; //Number of PARAMS in the UNSMRY file

    std::unique_ptr<rd::File> file{nullptr};
    std::shared_ptr<rd::FileView> file_view;
};

} // namespace rd
