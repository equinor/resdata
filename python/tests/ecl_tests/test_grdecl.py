#  Copyright (C) 2018  Equinor ASA, Norway.
#
#  The file 'test_grdecl.py' is part of ERT - Ensemble based Reservoir Tool.
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
from numpy import allclose

import cwrap
from ecl.eclfile import EclKW


def test_eclkw_read_grdecl(tmp_path):
    block_size = 10
    num_blocks = 5
    value = 0.15
    with open(tmp_path / "test.grdecl", "w") as f:
        f.write("COORD\n")
        for _ in range(num_blocks):
            f.write(f"{block_size}*{value} \n")
        f.write("/\n")

    with cwrap.open(str(tmp_path / "test.grdecl")) as f:
        kw = EclKW.read_grdecl(f, "COORD")

    assert kw.get_name() == "COORD"
    assert len(kw.numpy_view()) == block_size * num_blocks
    assert allclose(kw.numpy_view(), value)
