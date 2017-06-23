/*
   Copyright (C) 2017  Statoil ASA, Norway.

   The file 'util_unlink.c' is part of ERT - Ensemble based Reservoir Tool.

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


#if defined(ERT_HAVE_UNISTD)
  #include <unistd.h>
#elif defined(ERT_HAVE_IO)
  #include <io.h>
#else
  #include <ert/util/util.h>
#endif

int util_unlink(const char * filename) {
#if defined(ERT_HAVE_UNISTD)
 return unlink(filename);
#elif defined(ERT_HAVE_IO)
 return _unlink(filename);
#else
 util_abort("%s: No unlink functionality available.\n", __func__);
 return -1;
#endif
}
