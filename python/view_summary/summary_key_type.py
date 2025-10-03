from __future__ import annotations

import re
from enum import Enum, auto
from typing import TypeVar
from pydantic import PositiveInt

SPECIAL_KEYWORDS = [
    "NAIMFRAC",
    "NBAKFL",
    "NBYTOT",
    "NCPRLINS",
    "NEWTFL",
    "NEWTON",
    "NLINEARP",
    "NLINEARS",
    "NLINSMAX",
    "NLINSMIN",
    "NLRESMAX",
    "NLRESSUM",
    "NMESSAGE",
    "NNUMFL",
    "NNUMST",
    "NTS",
    "NTSECL",
    "NTSMCL",
    "NTSPCL",
    "ELAPSED",
    "MAXDPR",
    "MAXDSO",
    "MAXDSG",
    "MAXDSW",
    "STEPTYPE",
    "WNEWTON",
]


class SummaryKeyType(Enum):
    """Summary keys are divided into types based on summary variable name.

    This is a reimplmentation of rd::smspec_node::identify_var_type

    """

    AQUIFER = auto()
    BLOCK = auto()
    COMPLETION = auto()
    FIELD = auto()
    GROUP = auto()
    LOCAL_BLOCK = auto()
    LOCAL_COMPLETION = auto()
    LOCAL_WELL = auto()
    NETWORK = auto()
    SEGMENT = auto()
    WELL = auto()
    REGION = auto()
    INTER_REGION = auto()
    OTHER = auto()

    @classmethod
    def from_variable(cls, summary_variable: str) -> SummaryKeyType:
        """Returns the type corresponding to the given summary variable

        >>> SummaryKeyType.from_variable("FOPR").name
        'FIELD'
        >>> SummaryKeyType.from_variable("LWWIT").name
        'LOCAL_WELL'
        """
        KEYWORD_TYPE_MAPPING = {
            "A": cls.AQUIFER,
            "B": cls.BLOCK,
            "C": cls.COMPLETION,
            "F": cls.FIELD,
            "G": cls.GROUP,
            "LB": cls.LOCAL_BLOCK,
            "LC": cls.LOCAL_COMPLETION,
            "LW": cls.LOCAL_WELL,
            "N": cls.NETWORK,
            "S": cls.SEGMENT,
            "W": cls.WELL,
        }
        if not summary_variable:
            raise ValueError("Got empty summary keyword")
        if any(special in summary_variable for special in SPECIAL_KEYWORDS):
            return cls.OTHER
        if summary_variable[0] in KEYWORD_TYPE_MAPPING:
            return KEYWORD_TYPE_MAPPING[summary_variable[0]]
        if summary_variable[0:2] in KEYWORD_TYPE_MAPPING:
            return KEYWORD_TYPE_MAPPING[summary_variable[0:2]]
        if summary_variable == "RORFR":
            return cls.REGION

        if any(
            re.fullmatch(pattern, summary_variable)
            for pattern in [r"R.FT.*", r"R..FT.*", r"R.FR.*", r"R..FR.*", r"R.F"]
        ):
            return cls.INTER_REGION
        if summary_variable[0] == "R":
            return cls.REGION

        return cls.OTHER


def make_summary_key(
    keyword: str,
    number: int | None = None,
    name: str | None = None,
    nx: int | None = None,
    ny: int | None = None,
    lgr_name: str | None = None,
    li: int | None = None,
    lj: int | None = None,
    lk: int | None = None,
) -> str:
    """Converts values found in the smspec file to the summary_key format.

    This is a reimplementation of smspec_node::set_gen_keys

    >>> make_summary_key(keyword="WOPR", name="WELL1")
    'WOPR:WELL1'
    >>> make_summary_key(keyword="BOPR", number=4, nx=2, ny=2)
    'BOPR:2,2,1'
    """
    match SummaryKeyType.from_variable(keyword):
        case SummaryKeyType.FIELD | SummaryKeyType.OTHER:
            return keyword
        case SummaryKeyType.REGION | SummaryKeyType.AQUIFER:
            return f"{keyword}:{number}"
        case SummaryKeyType.BLOCK:
            nx, ny = _check_if_missing("block", "dimens", nx, ny)
            (number,) = _check_if_missing("block", "nums", number)
            i, j, k = _cell_index(number - 1, nx, ny)
            return f"{keyword}:{i},{j},{k}"
        case SummaryKeyType.GROUP | SummaryKeyType.WELL:
            return f"{keyword}:{name}"
        case SummaryKeyType.SEGMENT:
            return f"{keyword}:{name}:{number}"
        case SummaryKeyType.COMPLETION:
            nx, ny = _check_if_missing("completion", "dimens", nx, ny)
            (number,) = _check_if_missing("completion", "nums", number)
            i, j, k = _cell_index(number - 1, nx, ny)
            return f"{keyword}:{name}:{i},{j},{k}"
        case SummaryKeyType.INTER_REGION:
            (number,) = _check_if_missing("inter region", "nums", number)
            r1 = number % 32768
            r2 = ((number - r1) // 32768) - 10
            return f"{keyword}:{r1}-{r2}"
        case SummaryKeyType.LOCAL_WELL:
            (name,) = _check_if_missing("local well", "WGNAMES", name)
            (lgr_name,) = _check_if_missing("local well", "LGRS", lgr_name)
            return f"{keyword}:{lgr_name}:{name}"
        case SummaryKeyType.LOCAL_BLOCK:
            li, lj, lk = _check_if_missing("local block", "NUMLX", li, lj, lk)
            (lgr_name,) = _check_if_missing("local block", "LGRS", lgr_name)
            return f"{keyword}:{lgr_name}:{li},{lj},{lk}"
        case SummaryKeyType.LOCAL_COMPLETION:
            li, lj, lk = _check_if_missing("local completion", "NUMLX", li, lj, lk)
            (name,) = _check_if_missing("local completion", "WGNAMES", name)
            (lgr_name,) = _check_if_missing("local completion", "LGRS", lgr_name)
            return f"{keyword}:{lgr_name}:{name}:{li},{lj},{lk}"
        case SummaryKeyType.NETWORK:
            (name,) = _check_if_missing("network", "WGNAMES", name)
            return f"{keyword}:{name}"


T = TypeVar("T")


def _check_if_missing(
    keyword_name: str, missing_key: str, *test_vars: T | None
) -> list[T]:
    if any(v is None for v in test_vars):
        raise ValueError(
            f"Found {keyword_name} keyword in summary "
            f"specification without {missing_key} keyword"
        )
    return test_vars  # type: ignore


def _cell_index(
    array_index: int, nx: PositiveInt, ny: PositiveInt
) -> tuple[int, int, int]:
    k = array_index // (nx * ny)
    array_index -= k * (nx * ny)
    j = array_index // nx
    array_index -= j * nx
    return array_index + 1, j + 1, k + 1
