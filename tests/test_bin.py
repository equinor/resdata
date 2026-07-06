import signal
import subprocess
import sys

import pytest


@pytest.mark.skipif(
    sys.platform != "linux", reason="ecl utils are only available on Linux at this time"
)
@pytest.mark.parametrize(
    "name,returncode,stderr",
    [
        ("rd_pack.x", 0, b""),
        ("rd_unpack.x", 1, b"rd_unpack UNIFIED_FILE1"),
        ("summary.x", 2, b"the following arguments are required: CASE"),
    ],
)
def test_exec(name: str, returncode: int, stderr: bytes) -> None:
    status = subprocess.run([name], stderr=subprocess.PIPE)
    assert status.returncode == returncode
    assert stderr in status.stderr
