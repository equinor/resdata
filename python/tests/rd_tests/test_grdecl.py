from numpy import allclose

import cwrap
from resdata.resfile import ResdataKW


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
        kw = ResdataKW.read_grdecl(f, "COORD")

    assert kw.get_name() == "COORD"
    assert len(kw.numpy_view()) == block_size * num_blocks
    assert allclose(kw.numpy_view(), value)
