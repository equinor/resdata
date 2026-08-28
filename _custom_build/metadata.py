from __future__ import annotations

import sys
from typing import Any, Mapping


def dynamic_metadata(
    settings: Mapping[str, Any], project: Mapping[str, Any]
) -> dict[str, Any]:
    scripts = {}
    if sys.platform.startswith("linux"):
        scripts = {
            "rd_pack.x": "resdata.bin:main",
            "rd_unpack.x": "resdata.bin:main",
            "summary.x": "view_summary.__main__:main",
        }
    return {"scripts": scripts}
