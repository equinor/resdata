#include <vector>

#include <ert/util/vector.hpp>

#include <ert/ecl/ecl_smspec.hpp>
#include <ert/ecl/ecl_sum_tstep.hpp>
#include <ert/ecl/ecl_file.hpp>

namespace ecl {

#define INVALID_MINISTEP_NR -1
#define INVALID_TIME_T 0

class ecl_sum_file_data {

public:
  ecl_sum_file_data(const ecl_smspec_type * smspec);
  ~ecl_sum_file_data();
  const ecl_smspec_type * smspec() const;

  int                  length_before(time_t end_time) const;
  void                 get_time(int length, time_t *data);
  void                 get_data(int params_index, int length, double *data);
  int                  get_length() const;
  time_t               get_data_start() const;
  time_t               get_sim_end() const;
  double               iget( int time_index , int params_index ) const;
  time_t               iget_sim_time(int time_index ) const;
  ecl_sum_tstep_type * iget_ministep( int internal_index ) const;
  double               get_days_start() const;
  double               get_sim_length() const;

  std::pair<int,int>   report_range(int report_step) const;
  bool                 report_step_equal( const ecl_sum_file_data& other, bool strict) const;
  int                  report_before(time_t end_time) const;
  int                  get_time_report(int max_internal_index, time_t *data);
  int                  get_data_report(int params_index, int max_internal_index, double *data);
  int                  first_report() const;
  int                  last_report() const;
  bool                 has_report(int report_step ) const;
  int                  report_step_from_days(double sim_days) const;
  int                  report_step_from_time(time_t sim_time) const;

  ecl_sum_tstep_type * add_new_tstep(int report_step , double sim_seconds);
  void                 fwrite_unified( fortio_type * fortio ) const;
  void                 fwrite_multiple( const char * ecl_case , bool fmt_case ) const;
  bool                 fread(const stringlist_type * filelist);

private:
  const ecl_smspec_type         * ecl_smspec;
  double                          days_start;
  double                          sim_length;
  int                             first_report_step;
  int                             last_report_step;

  std::vector<std::pair<int,int>> report_map; // This will map from a report step to first and last internal index.
  std::pair<time_t, time_t>       time_range;
  vector_type                   * data;
  bool                            index_valid;


  void                 clear_index();
  void                 append_tstep(ecl_sum_tstep_type * tstep);
  void                 build_index();
  void                 fwrite_report( int report_step , fortio_type * fortio) const;
  bool                 check_file( ecl_file_type * ecl_file );
  void                 add_ecl_file(int report_step, const ecl_file_view_type * summary_view, const ecl_smspec_type * smspec);
};




}
