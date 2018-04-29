/*
   Copyright (C) 2018  Statoil ASA, Norway.

   The file 'util_portability.c' is part of ERT - Ensemble based Reservoir Tool.

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

#include <ert/util/util_portability.h>

#include <ert/util/ert_api_config.h>
#include "ert/util/build_config.h"



#ifdef ERT_HAVE_UNISTD
#include <unistd.h>
#endif



/**
   WIndows does not have the usleep() function, on the other hand
   Sleep() function in windows has millisecond resolution, instead of
   seconds as in linux.
*/

void util_usleep( unsigned long micro_seconds ) {
#ifdef HAVE__USLEEP
  usleep( micro_seconds );
#else
#ifdef ERT_WINDOWS
  {
    int milli_seconds = micro_seconds / 1000;
    Sleep( milli_seconds );
  }
#endif
#endif
}
