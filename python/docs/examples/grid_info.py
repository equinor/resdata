#!/usr/bin/env python
import sys

from resdata.grid import Grid, ResdataRegion


def volume_min_max(grid):
    vols = [c.volume for c in grid if c.active]
    return min(vols), max(vols)


def main(grid):
    vmin, vmax = volume_min_max(grid)

    dz_limit = 0.3
    region = ResdataRegion(grid, False)
    region.select_thin(dz_limit)

    print(f"Smallest cell     : {vmin}")
    print(f"Largest cell      : {vmax}")
    print(f"Thin active cells : {region.active_size()}")

    for ai in region.get_active_list():
        c = grid.cell(active_index=ai)
        print(f"dz({c.i:2d}, {c.j:2d}, {c.k:2d}) = {c.dz:.3f}")


if __name__ == "__main__":
    if len(sys.argv) < 2:
        sys.exit("usage: grid_info.py path/to/file.EGRID")
    case = sys.argv[1]
    grid = Grid(case)
    main(grid)
