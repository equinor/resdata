
import pytest
from resdata.grid import Grid


def write_zero_grdecl(file_path, num_x, num_y, num_z):
    """
    Writes a grdecl file containing a a grid with
    the given dimensions, no active cells and all coordinates
    at 0,0,0.
    """

    def write_coord(fstream, num_points):
        fstream.write("COORD\n")
        for _ in range(num_points):
            fstream.write("0.0 " * 6 + "\n")
        fstream.write("/\n")

    def write_zcorn(fstream, num_cells):
        fstream.write("ZCORN\n")
        for _ in range(num_cells):
            fstream.write("0.0 " * 8 + "\n")
        fstream.write("/\n")

    def write_actnum(fstream, num_cells):
        fstream.write("ACTNUM\n")
        for _ in range(num_cells // 20):
            fstream.write("0 " * 20 + "\n")

    def write_header(fstream, num_x, num_y, num_z):
        fstream.write("0 " * (num_cells % 20) + "\n")
        fstream.write("/\n")
        grdecl_header = f"""SPECGRID
        {num_x} {num_y} {num_z} 1 F
        /
        """
        fstream.write(grdecl_header)

    num_cells = num_x * num_y * num_z
    with file_path.open("w") as stream:
        write_header(stream, num_x, num_y, num_z)
        write_coord(stream, (num_x + 1) * (num_y + 1))
        write_zcorn(stream, num_cells)
        write_actnum(stream, num_cells)


def test_large_case(tmp_path):
    big_grdecl = tmp_path / "big.grdecl"

    num_x = 250
    num_y = 300
    num_z = 50

    write_zero_grdecl(big_grdecl, num_x, num_y, num_z)

    grid = Grid.load_from_grdecl(str(big_grdecl))
    assert grid.get_nx() == num_x
    assert grid.get_ny() == num_y
    assert grid.get_nz() == num_z

    x, y, z = grid.get_cell_corner(0, ijk=(num_x - 1, num_y - 1, num_z - 1))
    assert x == pytest.approx(0.0)
    assert y == pytest.approx(0.0)
    assert z == pytest.approx(0.0)
