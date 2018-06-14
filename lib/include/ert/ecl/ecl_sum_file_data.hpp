
#include <ert/util/vector.hpp>
#include <ert/util/double_vector.h>

#include <ert/ecl/ecl_smspec.hpp>
#include <ert/ecl/ecl_sum_tstep.hpp>
#include <ert/ecl/ecl_file.hpp>

namespace ecl {

#define INVALID_MINISTEP_NR -1
#define INVALID_TIME_T 0

class ecl_sum_file_data_type {

  double  days_start;
  double  sim_length;
  int first_report_step;
  int last_report_step;
  ecl_smspec_type * smspec;
  vector_type * data;
  int_vector_type * report_first_index;
  int_vector_type * report_last_index;
  bool         index_valid;
  time_t       start_time;             /* In the case of restarts the start might disagree with the value reported
                                          in the smspec file. */
  time_t       end_time;


  void                 clear_index();
  void                 append_tstep__(ecl_sum_tstep_type * tstep);

  void                 build_index();
  bool                 has_report_step(int report_step ) const;
  void                 report2internal_range(int report_step , int * index1 , int * index2 ) const;
  void                 fwrite_report__( int report_step , fortio_type * fortio) const;
  bool                 check_file( ecl_file_type * ecl_file );
  void                 add_ecl_file(int report_step, const ecl_file_view_type * summary_view, const ecl_smspec_type * smspec);

  public:
    ecl_sum_file_data_type(ecl_smspec_type * smspec);
    ~ecl_sum_file_data_type();

    int                  get_length();
    time_t               get_data_start();
    time_t               get_sim_end();
    int                  get_first_report_step();
    int                  get_last_report_step();
    double               iget( int time_index , int params_index );
    ecl_sum_tstep_type * iget_ministep( int internal_index ) const;

    ecl_sum_tstep_type * add_new_tstep(int report_step , double sim_seconds);
    void                 update_data_vector( double_vector_type * data_vector , int data_index , bool report_only, int end_min_step);
    void                 update_report_vectors( int_vector_type * parent_report_first_index, int_vector_type * parent_report_last_index, int first_mini_step, int last_ministep );
    void                 fwrite_unified( fortio_type * fortio ) const;
    void                 fwrite_multiple( const char * ecl_case , bool fmt_case ) const;
    bool                 fread(const stringlist_type * filelist);


};




}
