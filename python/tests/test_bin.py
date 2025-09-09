import sys
import pytest
import subprocess
import signal


@pytest.mark.skipif(
    sys.platform != "linux", reason="ecl utils are only available on Linux at this time"
)
@pytest.mark.parametrize(
    "name,returncode,stderr",
    [
        ("convert.x", 1, b"Usage: convert.x"),
        ("rd_pack.x", 0, b""),
        ("rd_unpack.x", 1, b"rd_unpack UNIFIED_FILE1"),
        ("grdecl_test.x", -signal.SIGABRT, b"util_fopen: failed to open:(null)"),
        ("kw_extract", 0, b"src_file target_file kw1 kw2 kw3"),
        ("kw_list.x", 0, b""),
        ("load_test.x", 0, b""),
        ("make_grid", 1, b"make_grid: basename nx ny nz"),
        ("segment_info", -signal.SIGSEGV, b""),
        ("select_test.x", 0, b""),
        ("summary.x", 1, b""),
    ],
)
def test_exec(name: str, returncode: int, stderr: str) -> None:
    status = subprocess.run([name], stderr=subprocess.PIPE)
    assert status.returncode == returncode
    assert stderr in status.stderr
