/*
   Copyright (C) 2018  Statoil ASA, Norway.

   The file 'ecl_nnc_export_intersect.c' is part of ERT - Ensemble based
   Reservoir Tool.

   ERT is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   ERT is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.

   See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html>
   for more detals.
*/
// #include <stdlib.h>
// #include <stdbool.h>
#include <signal.h>

#include <ert/util/test_util.hpp>

#include <ert/ecl/ecl_nnc_export.hpp>
#include <ert/ecl/ecl_kw_magic.hpp>
#include <ert/ecl/ecl_nnc_data.hpp>
#include <ert/ecl/ecl_grid.hpp>
#include <ert/ecl/ecl_file.hpp>

#include <ert/util/ert_unique_ptr.hpp>
#include <ert/util/util.hpp>



namespace {

const auto GRIDX = 10, GRIDY = 10, GRIDZ = 10;
const auto GRID_NNC_NUM = 342;
const auto INIT_NNC_NUM = 298;


void install_SIGNALS(void) {
  signal(SIGSEGV , util_abort_signal);    /* Segmentation violation, i.e. overwriting memory ... */
  signal(SIGINT  , util_abort_signal);    /* Control C */
  signal(SIGTERM , util_abort_signal);    /* If killing the program with SIGTERM (the default kill signal) you will get a backtrace.
                                             Killing with SIGKILL (-9) will not give a backtrace.*/
}


ERT::ert_unique_ptr<ecl_grid_type, ecl_grid_free>
make_intersect_grid() {
   auto out = ERT::ert_unique_ptr<ecl_grid_type, ecl_grid_free>(
         ecl_grid_alloc_rectangular(GRIDX, GRIDY, GRIDZ, 1., 1., 1., nullptr)
         );
   for(auto i = 0; i < GRID_NNC_NUM; ++i)
      ecl_grid_add_self_nnc(out.get(), 0, 0, i);
   return out;
}


ERT::ert_unique_ptr<ecl_file_type, ecl_file_close>
make_intersect_init_file() {
   // Create keywords with useless data
   auto nnc1_kw = ecl_kw_alloc(NNC1_KW, INIT_NNC_NUM, ECL_INT);
   auto nnc2_kw = ecl_kw_alloc(NNC2_KW, INIT_NNC_NUM, ECL_INT);
   auto tran_kw = ecl_kw_alloc(TRANNNC_KW, INIT_NNC_NUM, ECL_DOUBLE);
   for(auto i = 0; i < INIT_NNC_NUM; ++i) {
      ecl_kw_iset_int(nnc1_kw, i, 1);
      ecl_kw_iset_int(nnc2_kw, i, 2);
      ecl_kw_iset_double(tran_kw, i, 2. * i);
   }

   // write to file directly using fortio
   auto init_filename = util_alloc_tmp_file("/tmp", "ecl_nnc_export_intersect_init_file", false);
   auto fortio = fortio_open_writer(init_filename, false, true);
   ecl_kw_fwrite(nnc1_kw, fortio);
   ecl_kw_fwrite(nnc2_kw, fortio);
   ecl_kw_fwrite(tran_kw, fortio);
   fortio_fclose(fortio);

   // reopen the file as an ecl file
   auto out = ERT::ert_unique_ptr<ecl_file_type, ecl_file_close>(
         ecl_file_open(init_filename, 0)
         );

   free(init_filename);
   return out;
}
} /* unnamed namespace */



int main(int argc, char ** argv) {
  install_SIGNALS();

  const auto grid = make_intersect_grid();
  const auto init_file = make_intersect_init_file();

  test_assert_true(ecl_nnc_intersect_format(grid.get(), init_file.get()));
  test_assert_int_equal(ecl_nnc_export_get_size(grid.get(), init_file.get()), INIT_NNC_NUM);

  auto nnc_data = std::vector<ecl_nnc_type>(
        ecl_nnc_export_get_size(grid.get(), init_file.get())
        );
  auto const total_valid_trans = ecl_nnc_export(grid.get(), init_file.get(), nnc_data.data());
  test_assert_int_equal(total_valid_trans, INIT_NNC_NUM);
  test_assert_int_equal(int(nnc_data.size()), INIT_NNC_NUM);

  for(auto i = 0; i < int(nnc_data.size()); ++i) {
     auto const& nnc = nnc_data[i];
     test_assert_int_equal(nnc.grid_nr1, 0);
     test_assert_int_equal(nnc.grid_nr2, 0);
     test_assert_int_equal(nnc.global_index1, 1); // as set in make_intersect_init_file()
     test_assert_int_equal(nnc.global_index2, 2); // as set in make_intersect_init_file()
     test_assert_double_equal(nnc.trans, 2. * i); // as set in make_intersect_init_file()
     test_assert_int_equal(nnc.input_index, i);
  }

  return 0;
}
