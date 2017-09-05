/*
   Copyright (C) 2016  Statoil ASA, Norway.

   The file 'transactions.c' is part of ERT - Ensemble based Reservoir Tool.

   ERT is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   ERT is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.

   See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html>
   for more details.
*/

#include <ert/ecl/ecl_file_view.h>
#include <ert/ecl/ecl_file_kw.h>
#include <ert/ecl/ecl_file_transaction.h>


struct ecl_file_transaction_struct {
  int * value;
  int size;
};


ecl_file_transaction_type * ecl_file_transaction_start(ecl_file_view_type * file_view) {
  ecl_file_transaction_type * transaction = util_malloc( sizeof * transaction );
  transaction->size = ecl_file_view_get_size( file_view );
  transaction->value = util_malloc( transaction->size * sizeof(int));
  for (int i = 0; i < transaction->size; i++) {
    ecl_file_kw_type * file_kw = ecl_file_view_iget_file_kw( file_view, i);
    transaction->value[i] = ecl_file_kw_get_ref_count(file_kw);
  }
  return transaction;
}

int ecl_file_transaction_iget_value(ecl_file_transaction_type * tran, int index) {
  return tran->value[index];
}

void ecl_file_transaction_end(ecl_file_transaction_type * tran) {
  free(tran->value);
  free(tran);
}
