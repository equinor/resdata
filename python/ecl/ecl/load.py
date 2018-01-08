#  Copyright (C) 2018  Statoil ASA, Norway.
#
#  The file 'load.py' is part of ERT - Ensemble based Reservoir Tool.
#
#  ERT is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  ERT is distributed in the hope that it will be useful, but WITHOUT ANY
#  WARRANTY; without even the implied warranty of MERCHANTABILITY or
#  FITNESS FOR A PARTICULAR PURPOSE.
#
#  See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html>
#  for more details.
import os.path

from ecl.ecl import EclFileEnum
from ecl.ecl import EclInitFile,EclRestartFile,EclGrid,EclUtil

load_map = {EclFileEnum.ECL_INIT_FILE : EclInitFile,
            EclFileEnum.ECL_RESTART_FILE : EclRestartFile,
            EclFileEnum.ECL_UNIFIED_RESTART_FILE : EclRestartFile,
            EclFileEnum.ECL_GRID_FILE : EclGrid,
            EclFileEnum.ECL_EGRID_FILE : EclGrid }


def load(filename):
   if not os.path.isfile(filename):
       raise IOError("No such file: %s" % filename)

   file_type =  EclUtil.get_file_type(filename)
   if not file_type in load_map:
       raise ValueError("No load type registered for file type:%s - can not load:%s" % (file_type, filename))

   constructor = load_map[file_type]
   return constructor(filename)
