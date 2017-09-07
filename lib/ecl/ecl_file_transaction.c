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


#include <ert/ecl/ecl_file_transaction.h>


struct ecl_file_transaction_struct {
  int * value;
};


ecl_file_transaction_type * ecl_file_transaction_start(int * value) {
  ecl_file_transaction_type * t = util_malloc(sizeof * t);
  t->value = value;
  return t;
}

int * ecl_file_transaction_get_ref_counts(ecl_file_transaction_type * transaction) {
  return transaction->value;
}

void ecl_file_transaction_end(ecl_file_transaction_type * transaction) {
  free(transaction->value);
  free(transaction);
}
