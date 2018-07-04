#include <vector>

#include <ert/util/TestArea.hpp>
#include <ert/util/test_util.hpp>
#include <ert/util/double_vector.h>
#include <ert/util/vector.h>

#include <ert/ecl/ecl_sum.hpp>


void assert_values(ecl_sum_type * ecl_sum) {
  const ecl_smspec_type * smspec_resampled = ecl_sum_get_smspec(ecl_sum);
  const smspec_node_type * node2 = ecl_smspec_iget_node(smspec_resampled, 2);

  test_assert_string_equal( "BPR" , smspec_node_get_keyword(node2) );
  test_assert_string_equal( "BARS" , smspec_node_get_unit(node2) );

  double_vector_type * data = ecl_sum_alloc_data_vector( ecl_sum, 2, false );

  test_assert_double_equal( double_vector_iget(data, 1), 2.0 );
  test_assert_double_equal( double_vector_iget(data, 2), 6.0 );
  test_assert_double_equal( double_vector_iget(data, 3), 10.0 );

  double_vector_free( data );
}


void write_ecl_sum(bool unified) {
  time_t start_time = util_make_date_utc( 1,1,2010 );
  ecl_sum_type * ecl_sum = ecl_sum_alloc_writer( "CASE" , false , unified , ":" , start_time , true , 10 , 10 , 10 );
  double sim_seconds = 0;

  int num_dates = 4;
  double ministep_length = 86400; // seconds in a day

  smspec_node_type * node1 = ecl_sum_add_var( ecl_sum , "FOPT" , NULL   , 0   , "Barrels" , 99.0 );
  smspec_node_type * node2 = ecl_sum_add_var( ecl_sum , "BPR"  , NULL   , 567 , "BARS"    , 0.0  );
  smspec_node_type * node3 = ecl_sum_add_var( ecl_sum , "WWCT" , "OP-1" , 0   , "(1)"     , 0.0  );

  for (int report_step = 0; report_step < num_dates; report_step++) {
      {
        ecl_sum_tstep_type * tstep = ecl_sum_add_tstep( ecl_sum , report_step + 1 , sim_seconds );
        ecl_sum_tstep_set_from_node( tstep , node1 , report_step*2.0 );
        ecl_sum_tstep_set_from_node( tstep , node2 , report_step*4.0 + 2.0 );
        ecl_sum_tstep_set_from_node( tstep , node3 , report_step*6.0 + 4.0 );
      }
      sim_seconds += ministep_length * 3;
  }
  assert_values( ecl_sum );
  ecl_sum_fwrite( ecl_sum );
  ecl_sum_free(ecl_sum);
}


void write_restart_sum() {
  time_t start_time = util_make_date_utc( 1,1,2010 );
  int num_dates = 3;
  double ministep_length = 86400; // seconds in a day
  double sim_seconds = ministep_length * 2.5 * 3;
  ecl_sum_type * ecl_sum = ecl_sum_alloc_restart_writer( "CASE_RESTART" , "CASE", false , true , ":" , start_time , true , 10 , 10 , 10 );
 
  smspec_node_type * node2 = ecl_sum_add_var( ecl_sum , "BPR"  , NULL   , 567 , "BARS"    , 0.0  ); 
  smspec_node_type * node1 = ecl_sum_add_var( ecl_sum , "FOPT" , NULL   , 0   , "Barrels" , 99.0 );

  for (int report_step_ = 0; report_step_ < num_dates; report_step_++) {
      {
        int report_step = report_step_ + 3;
        ecl_sum_tstep_type * tstep = ecl_sum_add_tstep( ecl_sum , report_step + 1 , sim_seconds );
        ecl_sum_tstep_set_from_node( tstep , node1 , report_step*3.0 );
        ecl_sum_tstep_set_from_node( tstep , node2 , report_step*4.0 + 2.0 );
      }
      sim_seconds += ministep_length * 3;
  }  
  ecl_sum_fwrite( ecl_sum );
  ecl_sum_free(ecl_sum);
}

void write_restart_sum_1() {
  time_t start_time = util_make_date_utc( 1,1,2010 );
  int num_dates = 2;
  double ministep_length = 86400; // seconds in a day
  double sim_seconds = ministep_length * 4.0 * 3;
  ecl_sum_type * ecl_sum = ecl_sum_alloc_restart_writer( "CASE_RESTART_1" , "CASE_RESTART", false , true , ":" , start_time , true , 10 , 10 , 10 );
 
  smspec_node_type * node3 = ecl_sum_add_var( ecl_sum , "WWCT" , "OP-1" , 0   , "(1)"     , 0.0  );
  smspec_node_type * node2 = ecl_sum_add_var( ecl_sum , "BPR"  , NULL   , 567 , "BARS"    , 0.0  );
  smspec_node_type * node1 = ecl_sum_add_var( ecl_sum , "FOPT" , NULL   , 0   , "Barrels" , 99.0 ); 

  for (int report_step_ = 0; report_step_ < num_dates; report_step_++) {
      {
        int report_step = report_step_ + 5;
        ecl_sum_tstep_type * tstep = ecl_sum_add_tstep( ecl_sum , report_step + 1 , sim_seconds );
        ecl_sum_tstep_set_from_node( tstep , node1 , report_step*5.0 );
        ecl_sum_tstep_set_from_node( tstep , node2 , report_step*4.0 + 2.0 );
        ecl_sum_tstep_set_from_node( tstep , node3 , report_step*6.0 + 4.0 );
      }
      sim_seconds += ministep_length * 3;
  }  
  ecl_sum_fwrite( ecl_sum );
  ecl_sum_free(ecl_sum);
}


void test_read_write(bool unified) {
  write_ecl_sum(unified);
  ecl_sum_type * ecl_sum = ecl_sum_fread_alloc_case( "CASE" , ":");
  assert_values(ecl_sum);
  ecl_sum_free( ecl_sum );

  write_restart_sum();
  ecl_sum_type * ecl_sum_restart = ecl_sum_fread_alloc_case( "CASE_RESTART" , ":");

  test_assert_int_equal( ecl_sum_get_data_length( ecl_sum_restart ), 6 );
  double_vector_type * data = ecl_sum_alloc_data_vector( ecl_sum_restart, 2, false );
  test_assert_double_equal( double_vector_iget(data, 1),  0.0 );
  test_assert_double_equal( double_vector_iget(data, 2),  2.0 );
  test_assert_double_equal( double_vector_iget(data, 3),  4.0 );
  test_assert_double_equal( double_vector_iget(data, 4),  9.0 );
  test_assert_double_equal( double_vector_iget(data, 5), 12.0 );
  test_assert_double_equal( double_vector_iget(data, 6), 15.0 );
  double_vector_free( data );
  ecl_sum_free( ecl_sum_restart );

  write_restart_sum_1();
  ecl_sum_type * ecl_sum_restart_1 = ecl_sum_fread_alloc_case( "CASE_RESTART_1" , ":");
  test_assert_int_equal( ecl_sum_get_data_length( ecl_sum_restart_1 ), 7 );
  data = ecl_sum_alloc_data_vector( ecl_sum_restart_1, 3, false );
  test_assert_double_equal( double_vector_iget(data, 1),  0.0 );
  test_assert_double_equal( double_vector_iget(data, 2),  2.0 );
  test_assert_double_equal( double_vector_iget(data, 3),  4.0 );
  test_assert_double_equal( double_vector_iget(data, 4),  9.0 );
  test_assert_double_equal( double_vector_iget(data, 5), 12.0 );
  test_assert_double_equal( double_vector_iget(data, 6), 25.0 );
  test_assert_double_equal( double_vector_iget(data, 7), 30.0 );
  double_vector_free( data );

  data = ecl_sum_alloc_data_vector( ecl_sum_restart_1, 1, false );
  test_assert_double_equal( double_vector_iget(data, 1),  0.0 );
  test_assert_double_equal( double_vector_iget(data, 2),  0.0 );
  test_assert_double_equal( double_vector_iget(data, 3),  0.0 );
  test_assert_double_equal( double_vector_iget(data, 4),  0.0 );
  test_assert_double_equal( double_vector_iget(data, 5),  0.0 );
  test_assert_double_equal( double_vector_iget(data, 6), 34.0 );
  test_assert_double_equal( double_vector_iget(data, 7), 40.0 );
  double_vector_free( data );

  const ecl_smspec_type * smspec = ecl_sum_get_smspec(ecl_sum_restart_1);
  const smspec_node_type * node3 = ecl_smspec_iget_node(smspec, 3);
  time_t day = 86400;
  time_t t0 = util_make_date_utc( 1,1,2010 );
  time_t t1 = t0 + 3 * day;     //step 2
  time_t t2 = t0 + 6.75 * day;  //directly between step 3 and 4

  test_assert_double_equal(2.0000, ecl_sum_get_from_sim_time( ecl_sum_restart_1, t1, node3) );
  test_assert_double_equal(6.5000, ecl_sum_get_from_sim_time( ecl_sum_restart_1, t2, node3) );

  ecl_sum_free( ecl_sum_restart_1 );
}



int main() {
  test_read_write(true);
  test_read_write(false);
  return 0;
}
