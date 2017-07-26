#  Copyright (C) 2014  Statoil ASA, Norway.
#
#  This file is part of ERT - Ensemble based Reservoir Tool.
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

from ecl.util import UtilPrototype


def installAbortSignals():
    install_signals()


def updateAbortSignals():
    """
    Will install the util_abort_signal for all UNMODIFIED signals.
    """
    update_signals()


install_signals = UtilPrototype("void util_install_signals()")
update_signals = UtilPrototype("void util_update_signals()")
