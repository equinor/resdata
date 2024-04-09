import datetime
import pytest
from resdata import ResDataType
from resdata.resfile import ResdataKW, openFortIO, FortIO, ResdataFile
from resdata.grid import Grid
from resdata.gravimetry import ResdataSubsidence

from resdata.util.test import TestAreaContext
from tests import ResdataTest
from tests.rd_tests.create_restart import create_restart

import numpy as np


def create_init(grid, case):
    poro = ResdataKW("PORO", grid.getNumActive(), ResDataType.RD_FLOAT)
    porv = poro.copy()
    porv.setName("PORV")
    for g in range(grid.getGlobalSize()):
        porv[g] *= grid.cell_volume(global_index=g)

    with openFortIO("%s.INIT" % case, mode=FortIO.WRITE_MODE) as f:
        poro.fwrite(f)
        porv.fwrite(f)


class GeertsmaTest(ResdataTest):
    @staticmethod
    def test_geertsma_kernel():
        grid = Grid.createRectangular(dims=(1, 1, 1), dV=(50, 50, 50))
        with TestAreaContext("Subsidence"):
            p1 = [1]
            create_restart(grid, "TEST", p1)
            create_init(grid, "TEST")

            init = ResdataFile("TEST.INIT")
            restart_file = ResdataFile("TEST.UNRST")

            restart_view1 = restart_file.restartView(sim_time=datetime.date(2000, 1, 1))

            subsidence = ResdataSubsidence(grid, init)
            subsidence.add_survey_PRESSURE("S1", restart_view1)
            subsidence.add_survey_PRESSURE("S2", restart_view1)

            youngs_modulus = 5e8
            poisson_ratio = 0.3
            seabed = 0
            above = 100
            topres = 2000
            receiver = (1000, 1000, 0)

            dz = subsidence.evalGeertsma(
                "S1", None, receiver, youngs_modulus, poisson_ratio, seabed
            )
            np.testing.assert_almost_equal(dz, 3.944214576168326e-09)

            receiver = (1000, 1000, topres - seabed - above)

            dz = subsidence.evalGeertsma(
                "S1", None, receiver, youngs_modulus, poisson_ratio, seabed
            )
            np.testing.assert_almost_equal(dz, 5.8160298201497136e-08)

            assert subsidence.eval(
                "S1", "S2", receiver, youngs_modulus, poisson_ratio
            ) == pytest.approx(0.0)

    @staticmethod
    def test_geertsma_kernel_2_source_points_2_vintages():
        grid = Grid.createRectangular(dims=(2, 1, 1), dV=(100, 100, 100))

        with TestAreaContext("Subsidence"):
            p1 = [1, 10]
            p2 = [10, 20]
            create_restart(grid, "TEST", p1, p2)
            create_init(grid, "TEST")

            init = ResdataFile("TEST.INIT")
            restart_file = ResdataFile("TEST.UNRST")

            restart_view1 = restart_file.restartView(sim_time=datetime.date(2000, 1, 1))
            restart_view2 = restart_file.restartView(sim_time=datetime.date(2010, 1, 1))

            subsidence = ResdataSubsidence(grid, init)
            subsidence.add_survey_PRESSURE("S1", restart_view1)
            subsidence.add_survey_PRESSURE("S2", restart_view2)

            youngs_modulus = 5e8
            poisson_ratio = 0.3
            seabed = 0
            receiver = (1000, 1000, 0)

            dz1 = subsidence.evalGeertsma(
                "S1", None, receiver, youngs_modulus, poisson_ratio, seabed
            )
            np.testing.assert_almost_equal(dz1, 8.65322541521704e-07)

            dz2 = subsidence.evalGeertsma(
                "S2", None, receiver, youngs_modulus, poisson_ratio, seabed
            )
            np.testing.assert_almost_equal(dz2, 2.275556615015282e-06)

            np.testing.assert_almost_equal(dz1 - dz2, -1.4102340734935779e-06)

            dz = subsidence.evalGeertsma(
                "S1", "S2", receiver, youngs_modulus, poisson_ratio, seabed
            )
            np.testing.assert_almost_equal(dz, dz1 - dz2)

    @staticmethod
    def test_geertsma_kernel_seabed():
        grid = Grid.createRectangular(dims=(1, 1, 1), dV=(50, 50, 50))
        with TestAreaContext("Subsidence"):
            p1 = [1]
            create_restart(grid, "TEST", p1)
            create_init(grid, "TEST")

            init = ResdataFile("TEST.INIT")
            restart_file = ResdataFile("TEST.UNRST")

            restart_view1 = restart_file.restartView(sim_time=datetime.date(2000, 1, 1))

            subsidence = ResdataSubsidence(grid, init)
            subsidence.add_survey_PRESSURE("S1", restart_view1)

            youngs_modulus = 5e8
            poisson_ratio = 0.3
            seabed = 300
            above = 100
            topres = 2000
            receiver = (1000, 1000, topres - seabed - above)

            dz = subsidence.evalGeertsma(
                "S1", None, receiver, youngs_modulus, poisson_ratio, seabed
            )
            np.testing.assert_almost_equal(dz, 5.819790154474284e-08)

    @staticmethod
    def test_geertsma_kernel_seabed():
        grid = Grid.createRectangular(dims=(1, 1, 1), dV=(50, 50, 50))
        with TestAreaContext("Subsidence"):
            p1 = [1]
            create_restart(grid, "TEST", p1)
            create_init(grid, "TEST")

            init = ResdataFile("TEST.INIT")
            restart_file = ResdataFile("TEST.UNRST")

            restart_view1 = restart_file.restartView(sim_time=datetime.date(2000, 1, 1))

            subsidence = ResdataSubsidence(grid, init)
            subsidence.add_survey_PRESSURE("S1", restart_view1)

            youngs_modulus = 5e8
            poisson_ratio = 0.3
            seabed = 300
            above = 100
            topres = 2000
            receiver = (1000, 1000, topres - seabed - above)

            dz = subsidence.evalGeertsma(
                "S1", None, receiver, youngs_modulus, poisson_ratio, seabed
            )
            np.testing.assert_almost_equal(dz, 5.819790154474284e-08)

    def test_geertsma_rporv_kernel_2_source_points_2_vintages(self):
        grid = Grid.createRectangular(dims=(2, 1, 1), dV=(100, 100, 100))

        with TestAreaContext("Subsidence"):
            p1 = [1, 10]
            p2 = [10, 20]
            create_restart(
                grid,
                "TEST",
                p1,
                p2,
                rporv1=[10**5, 10**5],
                rporv2=[9 * 10**4, 9 * 10**4],
            )
            create_init(grid, "TEST")

            init = ResdataFile("TEST.INIT")
            restart_file = ResdataFile("TEST.UNRST")

            restart_view1 = restart_file.restartView(sim_time=datetime.date(2000, 1, 1))
            restart_view2 = restart_file.restartView(sim_time=datetime.date(2010, 1, 1))

            subsidence = ResdataSubsidence(grid, init)
            subsidence.add_survey_PRESSURE("S1", restart_view1)
            subsidence.add_survey_PRESSURE("S2", restart_view2)

            youngs_modulus = 5e8
            poisson_ratio = 0.3
            seabed = 0
            receiver = (1000, 1000, 0)

            dz1 = subsidence.eval_geertsma_rporv(
                "S1", None, receiver, youngs_modulus, poisson_ratio, seabed
            )
            dz2 = subsidence.eval_geertsma_rporv(
                "S2", None, receiver, youngs_modulus, poisson_ratio, seabed
            )
            dz = subsidence.eval_geertsma_rporv(
                "S1", "S2", receiver, youngs_modulus, poisson_ratio, seabed
            )

            np.testing.assert_almost_equal(dz, dz1 - dz2)
            self.assertTrue(dz > 0)
