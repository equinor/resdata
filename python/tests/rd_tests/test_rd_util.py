from resdata import FileType, ResdataTypeEnum, ResdataUtil
from resdata.grid import Grid
from tests import ResdataTest


class ResdataUtilTest(ResdataTest):
    def test_enums(self):
        source_file_path = "lib/include/resdata/rd_type.hpp"
        self.assertEnumIsFullyDefined(ResdataTypeEnum, "rd_type_enum", source_file_path)

    def test_file_type(self):
        file_type, fmt, report = ResdataUtil.inspectExtension("CASE.X0078")
        self.assertEqual(file_type, FileType.RESTART)

    def test_file_report_nr(self):
        report_nr = ResdataUtil.reportStep("CASE.X0080")
        self.assertEqual(report_nr, 80)

        with self.assertRaises(ValueError):
            ResdataUtil.reportStep("CASE.EGRID")
