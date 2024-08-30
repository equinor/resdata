import datetime
from textwrap import dedent

from resdata import FileType, ResdataTypeEnum, ResdataUtil
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


def test_num_cpu_from_data_file(tmp_path):
    data_file = tmp_path / "dfile"
    data_file_num_cpu = 4
    data_file.write_text(
        dedent(
            f"""\
            PARALLEL
            {data_file_num_cpu} DISTRIBUTED/
            """
        )
    )
    assert ResdataUtil.get_num_cpu(str(data_file)) == 4


def test_get_start_date_reads_from_start_kw_in_data_file(tmp_path):
    data_file = tmp_path / "dfile"
    data_file.write_text(
        dedent(
            """\
            START
            4 Apr 2024 /
            """
        )
    )
    assert ResdataUtil.get_start_date(str(data_file)) == datetime.datetime(
        2024, 4, 4, 0, 0
    )
