/*
   Copyright (C) 2017  Statoil ASA, Norway.

   This file is part of ERT - Ensemble based Reservoir Tool.

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

#ifndef ERT_ECL_FILE_TRANSACTION_H
#define ERT_ECL_FILE_TRANSACTION_H

#include <ert/ecl/ecl_file_view.h>
#include <ert/ecl/ecl_file_kw.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef struct ecl_file_transaction_struct ecl_file_transaction_type;

ecl_file_transaction_type * ecl_file_transaction_start(ecl_file_view_type * file_view);
void                        ecl_file_transaction_end(ecl_file_transaction_type * transaction);
int                         ecl_file_transaction_iget_value(ecl_file_transaction_type * tran, int index);


#ifdef __cplusplus
}
#endif

#endif
