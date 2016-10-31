import datetime
from ert.ecl import EclGrid, EclKW, EclTypeEnum, openFortIO, FortIO, EclFile, EclSubsidence

from ert.test import ExtendedTestCase , TestAreaContext

import numpy as np


def create_init(grid, case):
    poro = EclKW("PORO", grid.getNumActive(), EclTypeEnum.ECL_FLOAT_TYPE)
    porv = poro.copy()
    porv.setName("PORV")
    for g in range(grid.getGlobalSize()):
        porv[g] *= grid.cell_volume(global_index=g)

    with openFortIO("%s.INIT" % case, mode=FortIO.WRITE_MODE) as f:
        poro.fwrite(f)
        porv.fwrite(f)


def create_restart(grid, case):
    with openFortIO("%s.UNRST" % case, mode=FortIO.WRITE_MODE) as f:
        seq_hdr = EclKW("SEQNUM", 1, EclTypeEnum.ECL_FLOAT_TYPE)
        seq_hdr[0] = 10
        p = EclKW("PRESSURE", grid.getNumActive(), EclTypeEnum.ECL_FLOAT_TYPE)
        p[0] = 1

        header = EclKW("INTEHEAD", 67, EclTypeEnum.ECL_INT_TYPE)
        header[64] = 1
        header[65] = 1
        header[66] = 2000

        seq_hdr.fwrite(f)
        header.fwrite(f)
        p.fwrite(f)


class GeertsmaTest(ExtendedTestCase):

    @staticmethod
    def test_geertsma_kernel():
        grid = EclGrid.create_rectangular(dims=(1, 1, 1), dV=(50, 50, 50))

        with TestAreaContext("Subsidence"):
            create_restart(grid, "TEST")
            create_init(grid, "TEST")

            init = EclFile("TEST.INIT")
            restart_file = EclFile("TEST.UNRST")

            restart_view1 = restart_file.restartView(sim_time=datetime.date(2000, 1, 1))

            subsidence = EclSubsidence(grid, init)
            subsidence.add_survey_PRESSURE("S1", restart_view1)

            youngs_modulus = 5E8
            poisson_ratio = 0.3

            dz = subsidence.evalGeertsma("S1", None, (1000, 1000, 0), youngs_modulus, poisson_ratio)
            np.testing.assert_almost_equal(dz, -1.256514072122196e-07, 15)

            dz = subsidence.evalGeertsma("S1", None, (1000, 1000, 1900), youngs_modulus, poisson_ratio)
            np.testing.assert_almost_equal(dz, 6.530473913611929e-05, 15)
