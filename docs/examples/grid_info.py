#!/usr/bin/env python
import sys
from ecl.ecl import EclGrid, EclRegion


def volum_min_max(grid):
    vmin = 10000000
    vmax = 0
    for a in range(grid.getNumActive()):
        v = grid.cell_volume( active_index = a )
        vmin = min(vmin,v)
        vmax = max(vmax,v)

    return vmin,vmax


if __name__ == "__main__":
    case = sys.argv[1]
    grid = EclGrid( case )

    vmin,vmax = volum_min_max( grid )

    dz_limit = 0.1
    region = EclRegion( grid, False )
    region.select_thin( dz_limit )

    print "Smallest cell     : %g" % vmin
    print "Largest cell      : %g" % vmax
    print "Thin active cells : %d" % region.active_size()

    for ai in region.get_active_list( ):
        ijk = grid.get_ijk( active_index = ai )
        print ijk

