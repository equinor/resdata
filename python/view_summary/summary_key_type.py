from __future__ import annotations

import re
from enum import Enum, auto
from typing import TypeVar, assert_never

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


class InvalidSummaryKey(ValueError):
    pass


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

    See :ref:`about_summary_keys`.

    >>> make_summary_key(keyword="WOPR", name="WELL1")
    'WOPR:WELL1'
    >>> make_summary_key(keyword="BOPR", number=4, nx=2, ny=2)
    'BOPR:2,2,1'


    Args:
        keyword: Summary variable name (e.g., ``"WOPR"``, ``"BPR"``).
        number: Numeric qualifier from ``NUMS`` (cell index, region id, etc.).
        name: Text qualifier from ``WGNAMES`` (well/group name).
        nx: Grid dimension in x for block/completion keys.
        ny: Grid dimension in y for block/completion keys.
        lgr_name: Local grid name for local keys.
        li: Local i-index for local block/completion.
        lj: Local j-index for local block/completion.
        lk: Local k-index for local block/completion.

    Raises:
        InvalidSummaryKey: If the key is invalid
    """
    match SummaryKeyType.from_variable(keyword):
        case SummaryKeyType.FIELD | SummaryKeyType.OTHER:
            return keyword
        case SummaryKeyType.REGION:
            (number,) = _check_if_missing("region", "nums", number)
            _check_is_positive_number("region", number)
            return f"{keyword}:{number}"
        case SummaryKeyType.AQUIFER:
            (number,) = _check_if_missing("aquifer", "nums", number)
            _check_is_positive_number("aquifer", number)
            return f"{keyword}:{number}"
        case SummaryKeyType.BLOCK:
            nx, ny = _check_if_missing("block", "dimens", nx, ny)
            (number,) = _check_if_missing("block", "nums", number)
            _check_is_positive_number("block", number)
            i, j, k = _cell_index(number - 1, nx, ny)
            return f"{keyword}:{i},{j},{k}"
        case SummaryKeyType.WELL:
            (name,) = _check_if_missing("well", "name", name)
            _check_if_valid_name("well", name)
            return f"{keyword}:{name}"
        case SummaryKeyType.GROUP:
            (name,) = _check_if_missing("group", "name", name)
            _check_if_valid_name("group", name)
            return f"{keyword}:{name}"
        case SummaryKeyType.SEGMENT:
            (name,) = _check_if_missing("segment", "name", name)
            _check_if_valid_name("segment", name)
            (number,) = _check_if_missing("segment", "nums", number)
            _check_is_positive_number("segment", number)
            return f"{keyword}:{name}:{number}"
        case SummaryKeyType.COMPLETION:
            nx, ny = _check_if_missing("completion", "dimens", nx, ny)
            (number,) = _check_if_missing("completion", "nums", number)
            _check_is_positive_number("completion", number)
            (name,) = _check_if_missing("completion", "name", name)
            _check_if_valid_name("completion", name)
            i, j, k = _cell_index(number - 1, nx, ny)
            return f"{keyword}:{name}:{i},{j},{k}"
        case SummaryKeyType.INTER_REGION:
            (number,) = _check_if_missing("inter region", "nums", number)
            _check_is_positive_number("inter region", number)
            r1 = number % 32768
            r2 = ((number - r1) // 32768) - 10
            return f"{keyword}:{r1}-{r2}"
        case SummaryKeyType.LOCAL_WELL:
            (name,) = _check_if_missing("local well", "WGNAMES", name)
            _check_if_valid_name("local well", name)
            (lgr_name,) = _check_if_missing("local well", "LGRS", lgr_name)
            return f"{keyword}:{lgr_name}:{name}"
        case SummaryKeyType.LOCAL_BLOCK:
            li, lj, lk = _check_if_missing("local block", "NUMLX", li, lj, lk)
            (lgr_name,) = _check_if_missing("local block", "LGRS", lgr_name)
            return f"{keyword}:{lgr_name}:{li},{lj},{lk}"
        case SummaryKeyType.LOCAL_COMPLETION:
            (name,) = _check_if_missing("local completion", "WGNAMES", name)
            _check_if_valid_name("local completion", name)
            li, lj, lk = _check_if_missing("local completion", "NUMLX", li, lj, lk)
            (lgr_name,) = _check_if_missing("local completion", "LGRS", lgr_name)
            return f"{keyword}:{lgr_name}:{name}:{li},{lj},{lk}"
        case SummaryKeyType.NETWORK:
            (name,) = _check_if_missing("network", "WGNAMES", name)
            return f"{keyword}:{name}"
        case default:
            assert_never(default)


T = TypeVar("T")


def _check_if_missing(
    keyword_name: str, missing_key: str, *test_vars: T | None
) -> list[T]:
    if any(v is None for v in test_vars):
        raise InvalidSummaryKey(f"{keyword_name} keyword without {missing_key}")
    return test_vars  # type: ignore


_DUMMY_NAME = ":+:+:+:+"


def _check_if_valid_name(keyword_name: str, name: str) -> None:
    if not name or name == _DUMMY_NAME:
        raise InvalidSummaryKey(f"{keyword_name} keyword given invalid name '{name}'")


def _check_is_positive_number(keyword_name: str, number: int) -> None:
    if number < 0:
        raise InvalidSummaryKey(
            f"{keyword_name} keyword given negative number {number}"
        )


def _cell_index(
    array_index: int, nx: PositiveInt, ny: PositiveInt
) -> tuple[int, int, int]:
    """Convert a flat (0-based) index to 1-based (i, j, k) grid indices.

    Args:
        array_index: Zero-based flat index into a grid laid
            out as ``k`` layers of ``ny*nx``.
        nx: Number of cells in the x-direction (strictly positive).
        ny: Number of cells in the y-direction (strictly positive).

    Returns:
        A tuple ``(i, j, k)`` where each component is **1-based**.
    """
    k = array_index // (nx * ny)
    array_index -= k * (nx * ny)
    j = array_index // nx
    array_index -= j * nx
    return array_index + 1, j + 1, k + 1
