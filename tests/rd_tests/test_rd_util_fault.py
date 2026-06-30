import pytest
from resdata import ResdataUtil
from resdata.rd_util import get_file_type, get_num_cpu, get_start_date

DATA_FILE = "test-data/local/ECLIPSE/simple/SIMPLE.DATA"


def test_inspect_extension_rejects_bytearray_restart_filename():
    with pytest.raises(TypeError):
        ResdataUtil.inspect_extension(bytearray(b"CASE.X0078"))


def test_camel_inspect_extension_rejects_bytearray_restart_filename():
    with pytest.raises(TypeError):
        ResdataUtil.inspectExtension(bytearray(b"CASE.F0078"))


def test_get_file_type_rejects_bytearray_unified_restart_filename():
    with pytest.raises(TypeError):
        ResdataUtil.get_file_type(bytearray(b"CASE.UNRST"))


def test_module_get_file_type_rejects_bytearray_summary_filename():
    with pytest.raises(TypeError):
        get_file_type(bytearray(b"CASE.S0001"))


def test_report_step_rejects_bytearray_restart_filename():
    with pytest.raises(TypeError):
        ResdataUtil.report_step(bytearray(b"CASE.X0080"))


def test_camel_report_step_rejects_bytearray_formatted_restart_filename():
    with pytest.raises(TypeError):
        ResdataUtil.reportStep(bytearray(b"CASE.F0080"))


def test_report_step_rejects_bytearray_non_report_filename_before_value_check():
    with pytest.raises(TypeError):
        ResdataUtil.report_step(bytearray(b"CASE.EGRID"))


def test_get_num_cpu_rejects_bytearray_data_file_path():
    with pytest.raises(TypeError):
        ResdataUtil.get_num_cpu(bytearray(DATA_FILE.encode()))


def test_module_get_num_cpu_rejects_bytearray_data_file_path():
    with pytest.raises(TypeError):
        get_num_cpu(bytearray(DATA_FILE.encode()))


def test_module_get_start_date_rejects_bytearray_data_file_path():
    with pytest.raises(TypeError):
        get_start_date(bytearray(DATA_FILE.encode()))
