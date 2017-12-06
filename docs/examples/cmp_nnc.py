#!/usr/bin/env python
import sys
from operator import itemgetter
from ecl.ecl import EclFile, EclGrid

def print_nnc_info(global_index_1, global_index_2, T):
    c1 = grid[global_index_1]
    c2 = grid[global_index_2]
    from_str = '(%02d,%02d,%02d)' % c1.ijk
    to_str = '(%02d,%02d,%02d)' % c2.ijk
    print("%s -> %s T:%g" % (from_str, to_str, T))

def main(grid, grid_file, init_file):
    nnc1 = grid_file["NNC1"][0]
    nnc2 = grid_file["NNC2"][0]
    tran = init_file["TRANNNC"][0]

    nnc_list = list(zip(nnc1,nnc2,tran))
    nnc_list = sorted(nnc_list, key=itemgetter(0))

    for (g1,g2,T) in nnc_list:
        print_nnc_info(g1, g2, T)

if __name__ == "__main__":
    if len(sys.argv) < 2:
        exit('Usage: cmp_nnc CASE')
    case = sys.argv[1]
    grid_file = EclFile("%s.EGRID" % case)
    init_file = EclFile("%s.INIT" % case)
    grid = EclGrid("%s.EGRID" % case)
    main(grid, grid_file, init_file)
