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

#ifndef ERT_TRANSACTION_H
#define ERT_TRANSACTION_H

#include <ert/ecl/ecl_file_view.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct transaction_struct transaction_type;

transaction_type * transaction_start(ecl_file_view_type * file_view);
void               transaction_end(transaction_type * transaction);
int                transaction_iget_value(transaction_type * tran, int index);
void               transaction_free(transaction_type * tran);


#ifdef __cplusplus
}
#endif

#endif
