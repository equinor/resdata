from resdata.grid import Grid
from resdata.summary import Summary
from tests import ResdataTest, equinor_test
from resdata.well import (
    WellInfo,
)


@equinor_test()
class ResdataWellTest3(ResdataTest):
    grid = None

    def test_rates(self):
        grid_path = self.createTestPath("Equinor/ECLIPSE/Gurbat/ECLIPSE.EGRID")
        rst_path = self.createTestPath("Equinor/ECLIPSE/Gurbat/ECLIPSE.UNRST")
        sum_path = self.createTestPath("Equinor/ECLIPSE/Gurbat/ECLIPSE.SMSPEC")

        grid = Grid(grid_path)
        well_info = WellInfo(grid, rst_path)
        sum = Summary(sum_path)

        for wtl in well_info:
            for well_state in wtl:
                # print "%03d  %g   %g " % (R , well_state.oilRate(), sum.get_from_report( "WOPR:%s" % well , R))
                if wtl.getName() == "OP_4":
                    pass
                    # print well_state.oilRate(), well_state.waterRate(), well_state.gasRate(), well_state.volumeRate()
                    # print well_state.oilRateSI(), well_state.waterRateSI(), well_state.gasRateSI(), well_state.volumeRateSI()
                    self.assertEqual(well_state.oilRate(), well_state.oilRateSI())
                    self.assertEqual(well_state.waterRate(), well_state.waterRateSI())
                    self.assertEqual(well_state.gasRate(), well_state.gasRateSI())
                    self.assertEqual(well_state.volumeRate(), well_state.volumeRateSI())
                    # print sum.get_from_report("WOPR:%s" % wtl.getName(), 1)
                    # print sum.get_from_report( "WWPR:%s" % wtl.getName(), 30 )

                    for conn in well_state.globalConnections():
                        # print conn.gasRate(), conn.waterRate(), conn.oilRate()
                        # print conn.gasRateSI(), conn.waterRateSI(), conn.oilRateSI()
                        self.assertEqual(conn.gasRate(), conn.gasRateSI())
                        self.assertEqual(conn.waterRate(), conn.waterRateSI())
                        self.assertEqual(conn.oilRate(), conn.oilRateSI())
                        self.assertEqual(conn.volumeRate(), conn.volumeRateSI())
                    #
                    # print sum.get_from_report("WGPR:%s" % wtl.getName(), 30)
                    #
                    # self.assertFloatEqual(well_state.oilRate(), sum.get_from_report("WOPR:%s" % wtl.getName(), 30))
                    # self.assertFloatEqual(well_state.waterRate(), sum.get_from_report("WWPR:%s" % wtl.getName(), 30))
                    # self.assertFloatEqual(well_state.gasRate(), sum.get_from_report("WGPR:%s" % wtl.getName(), 30))
