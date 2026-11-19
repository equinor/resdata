import datetime
from textwrap import dedent

import pytest
from resdata import FileType, ResdataTypeEnum, ResdataUtil

from tests import ResdataTest


class ResdataUtilTest(ResdataTest):
    def test_enums(self):
        source_file_path = "lib/include/resdata/rd_type.hpp"
        self.assertEnumIsFullyDefined(ResdataTypeEnum, "rd_type_enum", source_file_path)

    def test_file_type(self):
        file_type, fmt, report = ResdataUtil.inspect_extension("CASE.X0078")
        assert file_type == FileType.RESTART
        file_type, fmt, report = ResdataUtil.inspect_extension("CASE.UNRST")
        assert file_type == FileType.UNIFIED_RESTART
        assert report is None
        assert ResdataUtil.get_file_type("CASE.UNRST") == FileType.UNIFIED_RESTART

    def test_file_report_nr(self):
        report_nr = ResdataUtil.report_step("CASE.X0080")
        self.assertEqual(report_nr, 80)

        with self.assertRaises(ValueError):
            ResdataUtil.report_step("CASE.EGRID")


def test_num_cpu_from_data_file(tmp_path):
    data_file = tmp_path / "dfile"
    data_file_num_cpu = 4
    data_file.write_text(
        dedent(f"""\
            PARALLEL
            {data_file_num_cpu} DISTRIBUTED/
            """)
    )
    assert ResdataUtil.get_num_cpu(str(data_file)) == 4


def test_get_start_date_reads_from_start_kw_in_data_file(tmp_path):
    data_file = tmp_path / "dfile"
    data_file.write_text(
        dedent("""\
            START
            4 Apr 2024 /
            """)
    )
    assert ResdataUtil.get_start_date(str(data_file)) == datetime.datetime(
        2024, 4, 4, 0, 0
    )


def test_that_get_start_date_raises_value_error_when_start_is_missing(tmp_path):
    data_file = tmp_path / "dfile"
    data_file.write_text("RUNSPEC\n")
    with pytest.raises(ValueError, match="could not find START"):
        ResdataUtil.get_start_date(str(data_file))


def test_that_get_start_date_raises_value_error_for_an_invalid_month(tmp_path):
    data_file = tmp_path / "dfile"
    data_file.write_text("START\n4 Foo 2024 /\n")
    with pytest.raises(ValueError, match="not a valid month name"):
        ResdataUtil.get_start_date(str(data_file))


def test_that_get_start_date_raises_os_error_for_a_missing_file(tmp_path):
    with pytest.raises(OSError, match="failed to open"):
        ResdataUtil.get_start_date(str(tmp_path / "does_not_exist"))


def test_that_get_num_cpu_raises_value_error_for_unterminated_slaves(tmp_path):
    data_file = tmp_path / "dfile"
    data_file.write_text("SLAVES\n" + "'RES' 'base' 'host' 'dir' 4\n" * 12)
    with pytest.raises(ValueError, match="SLAVES"):
        ResdataUtil.get_num_cpu(str(data_file))


def test_that_get_start_date_accepts_a_single_quoted_month_name(tmp_path):
    data_file = tmp_path / "dfile"
    data_file.write_text("START\n 1 'JAN' 2000 /\n")
    assert ResdataUtil.get_start_date(str(data_file)) == datetime.datetime(2000, 1, 1)


def test_that_get_start_date_accepts_a_double_quoted_month_name(tmp_path):
    data_file = tmp_path / "dfile"
    data_file.write_text('START\n 1 "FEB" 2000 /\n')
    assert ResdataUtil.get_start_date(str(data_file)) == datetime.datetime(2000, 2, 1)


def test_that_get_start_date_accepts_a_quoted_day_and_year(tmp_path):
    data_file = tmp_path / "dfile"
    data_file.write_text("START\n '1' 'MAR' '2000' /\n")
    assert ResdataUtil.get_start_date(str(data_file)) == datetime.datetime(2000, 3, 1)


def test_that_get_start_date_keeps_a_different_quote_character_inside_a_quoted_month(
    tmp_path,
):
    data_file = tmp_path / "dfile"
    data_file.write_text('START\n 1 "A\'PR" 2000 /\n')
    with pytest.raises(ValueError, match="A'PR not a valid month name"):
        ResdataUtil.get_start_date(str(data_file))


def test_that_get_start_date_raises_value_error_for_an_unterminated_quotation(
    tmp_path,
):
    data_file = tmp_path / "dfile"
    data_file.write_text("START\n 1 'JAN 2000 /\n")
    with pytest.raises(ValueError, match="termination of START"):
        ResdataUtil.get_start_date(str(data_file))


def test_that_get_num_cpu_reads_a_quoted_cpu_count_from_parallel(tmp_path):
    data_file = tmp_path / "dfile"
    data_file.write_text("PARALLEL\n '4' DISTRIBUTED /\n")
    assert ResdataUtil.get_num_cpu(str(data_file)) == 4


def test_that_get_num_cpu_sums_the_cpus_of_quoted_slaves_entries(tmp_path):
    data_file = tmp_path / "dfile"
    data_file.write_text(
        dedent("""\
            SLAVES
            'RES1' 'base1' 'host' 'dir' 4 /
            'RES2' 'base2' 'host' 'dir' 3 /
            /
            """)
    )
    assert ResdataUtil.get_num_cpu(str(data_file)) == 4 + 3 + 1


def test_that_get_num_cpu_treats_a_quoted_slaves_entry_with_spaces_as_one_token(
    tmp_path,
):
    data_file = tmp_path / "dfile"
    data_file.write_text("SLAVES\n'RES 1' 'base 1' 'host' 'my dir' 4 /\n/\n")
    assert ResdataUtil.get_num_cpu(str(data_file)) == 4 + 1


def test_that_get_num_cpu_treats_an_escaped_quote_as_part_of_a_quoted_slaves_entry(
    tmp_path,
):
    data_file = tmp_path / "dfile"
    data_file.write_text("SLAVES\n'RES\\' 1' 'base' 'host' 'dir' 4 /\n/\n")
    assert ResdataUtil.get_num_cpu(str(data_file)) == 4 + 1


def test_that_get_num_cpu_raises_value_error_for_an_unterminated_quotation_in_slaves(
    tmp_path,
):
    data_file = tmp_path / "dfile"
    data_file.write_text("SLAVES\n'RES1 'base1' 'host' 'dir' 4 /\n/\n")
    with pytest.raises(ValueError, match="could not find quotation closing"):
        ResdataUtil.get_num_cpu(str(data_file))
