#!/usr/bin/env python
import sys
import matplotlib.pyplot as plt

# Import the required symbols from the ecl.ecl package.
from ecl.ecl import EclFile, EclGrid, EclRestartFile


def compute_avg_pressure(p, sw, pv, region, region_id, result):
    """Calculate the average pressure for all the cells in the region using three
     different methods.

    """
    if not region:
        return None, None, None
    p_pv = p*pv
    p_hc_pv = p*pv*(1 - sw)
    hc_pv = pv*(1 - sw)

    total_pv = pv.sum(mask=region)
    total_hc_pv = hc_pv.sum(mask=region)

    p1 = p.sum(mask=region) / region.active_size()

    if total_pv > 0:
        p2 = p_pv.sum(mask=region) / total_pv
    else:
        p2 = None

    if total_hc_pv > 0:
        p3 = p_hc_pv.sum(mask=region) / total_hc_pv
    else:
        p3 = None
    return p1, p2, p3

def avg_pressure(p, sw, pv, region, region_id, result):
    p1,p2,p3 = compute_avg_pressure(p, sw, pv, region, region_id, result)
    if not region_id in result:
        result[region_id] = [[],[],[]]

    result[region_id][0].append(p1)
    result[region_id][1].append(p2)
    result[region_id][2].append(p3)


def main(grid, rst, init):
    # Create PORV keyword where all the inactive cells have been removed.
    pv = grid.compressed_kw_copy(init["PORV"][0])

    # Extract an integer region keyword from the init file
    region_kw = init["EQLNUM"][0]


    sim_days = []
    result = {}
    for header in rst.headers():
        line = {}
        rst_block = rst.restart_view(report_step=header.get_report_step())
        p = rst_block["PRESSURE"][0]
        sw = rst_block["SWAT"][0]

        for region_id in range(region_kw.get_max() + 1):
            region = grid.region()
            region.select_equal(region_kw, region_id)
            avg_pressure(p, sw, pv, region, region_id, result)

        avg_pressure(p, sw, pv, grid.region(preselect=True), "field", result)
        sim_days.append(header.get_sim_days())

    for key in result.keys():
        plt.figure(1)
        for index,p in enumerate(result[key]):
            plt.plot(sim_days, p, label="Region:%s  P%d" % (key, index + 1))
        plt.legend()
        plt.show()


if __name__ == "__main__":
    if len(sys.argv) < 2:
        exit('Usage: avg_pressure CASE')

    case = sys.argv[1]
    grid = EclGrid("%s.EGRID" % case)
    rst_file = EclRestartFile(grid, "%s.UNRST" % case)
    init_file = EclFile("%s.INIT" % case)
    main(grid, rst_file, init_file)
