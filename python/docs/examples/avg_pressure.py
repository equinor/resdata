#!/usr/bin/env python
import sys

import matplotlib.pyplot as plt

from resdata.grid import Grid, ResdataRegion
from resdata.resfile import ResdataFile, ResdataRestartFile


# Calculate the average pressure for all the cells in the region using
# three different methods.
def avg_pressure(p, sw, pv, region, region_id, result):
    if region:
        p_pv = p * pv
        p_hc_pv = p * pv * (1 - sw)
        hc_pv = pv * (1 - sw)

        total_pv = pv.sum(mask=region)
        total_hc_pv = hc_pv.sum(mask=region)

        p1 = p.sum(mask=region) / region.active_size()

        p2 = p_pv.sum(mask=region) / total_pv if total_pv > 0 else None

        p3 = p_hc_pv.sum(mask=region) / total_hc_pv if total_hc_pv > 0 else None
    else:
        p1 = None
        p2 = None
        p3 = None

    if not region_id in result:
        result[region_id] = [[], [], []]

    result[region_id][0].append(p1)
    result[region_id][1].append(p2)
    result[region_id][2].append(p3)


# -----------------------------------------------------------------

if __name__ == "__main__":
    case = sys.argv[1]
    grid = Grid(f"{case}.EGRID")
    rst_file = ResdataRestartFile(grid, f"{case}.UNRST")
    init_file = ResdataFile(f"{case}.INIT")

    # Create PORV keyword where all the inactive cells have been removed.
    pv = grid.compressed_kw_copy(init_file["PORV"][0])

    # Extract an integer region keyword from the init file
    region_kw = init_file["EQLNUM"][0]

    sim_days = []
    result = {}
    for header in rst_file.headers():
        line = {}
        rst_block = rst_file.restart_view(report_step=header.get_report_step())
        p = rst_block["PRESSURE"][0]
        sw = rst_block["SWAT"][0]

        for region_id in range(region_kw.get_max() + 1):
            region = ResdataRegion(grid, False)
            region.select_equal(region_kw, region_id)
            avg_pressure(p, sw, pv, region, region_id, result)

        avg_pressure(p, sw, pv, ResdataRegion(grid, True), "field", result)
        sim_days.append(header.get_sim_days())

    for key, value in result.items():
        plt.figure(1)
        for index, p in enumerate(value):
            plt.plot(sim_days, p, label="Region:%s  P%d" % (key, index + 1))
        plt.legend()
        plt.show()
