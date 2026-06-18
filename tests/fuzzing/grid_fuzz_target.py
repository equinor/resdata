import atheris

with atheris.instrument_imports():
    from resdata.grid import Grid

import sys
import tempfile


def fuzz_grid(buffer: bytes) -> None:
    grid = None
    with tempfile.NamedTemporaryFile(delete_on_close=False, suffix=".EGRID") as fp:
        fp.write(buffer)
        fp.close()
        try:
            grid = Grid.load_from_file(fp.name)
        except Exception as e:
            return

    if grid is None:
        return

    nx, ny, nz, _nactive = grid.get_dims()

    if nx > 0 and ny > 0 and nz > 0 and nx < 10000 and ny < 10000 and nz < 10000:
        global_size = grid.get_global_size()
        if global_size > 0 and global_size < 1000000:
            if grid.active(ijk=(0, 0, 0)):
                _x, _y, _z = grid.get_xyz(ijk=(0, 0, 0))
        mid_cell = global_size // 2

        if mid_cell < global_size and grid.active(global_index=mid_cell):
            grid.cell_volume(global_index=mid_cell)


if __name__ == "__main__":
    atheris.Setup(
        sys.argv,
        atheris.instrument_func(fuzz_grid),
    )
    atheris.Fuzz()
