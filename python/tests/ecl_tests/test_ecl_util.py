from ecl import EclTypeEnum, EclFileEnum, EclPhaseEnum, EclUnitTypeEnum, EclUtil
from ecl.grid import EclGrid
from tests import EclTest


class EclUtilTest(EclTest):
    def test_enums(self):
        source_file_path = "lib/include/ert/ecl/ecl_util.hpp"
        self.assertEnumIsFullyDefined(EclFileEnum, "ecl_file_enum", source_file_path)
        self.assertEnumIsFullyDefined(EclPhaseEnum, "ecl_phase_enum", source_file_path)
        self.assertEnumIsFullyDefined(
            EclUnitTypeEnum, "ert_ecl_unit_enum", source_file_path
        )

        source_file_path = "lib/include/ert/ecl/ecl_type.hpp"
        self.assertEnumIsFullyDefined(EclTypeEnum, "ecl_type_enum", source_file_path)

    def test_file_type(self):
        file_type, fmt, report = EclUtil.inspectExtension("CASE.X0078")
        self.assertEqual(file_type, EclFileEnum.ECL_RESTART_FILE)

    def test_file_report_nr(self):
        report_nr = EclUtil.reportStep("CASE.X0080")
        self.assertEqual(report_nr, 80)

        with self.assertRaises(ValueError):
            EclUtil.reportStep("CASE.EGRID")
