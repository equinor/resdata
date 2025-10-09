import argparse
import fnmatch
import logging
import os
import re
import sys
from collections import defaultdict
from collections.abc import Iterator, Sequence, Iterable
from dataclasses import dataclass
from datetime import datetime, timedelta
from enum import StrEnum
from functools import partial
from textwrap import dedent
from typing import IO, Any, Callable, assert_never
from itertools import tee

import numpy as np
import numpy.typing as npt
import pandas as pd
import resfo
from natsort import natsorted

from .summary_key_type import InvalidSummaryKey, make_summary_key

logger = logging.getLogger(__name__)

FileOpener = Callable[[], IO[Any]]


def stream_name(stream: IO[Any]) -> str:
    return getattr(stream, "name", "unknown stream")


class CliError(ValueError):
    """Indicates an issue that should be presented to the CLI user"""


class TimeUnit(StrEnum):
    """Units associated with the TIME vector in summary files."""

    HOURS = "Hours"
    DAYS = "Days"

    def make_delta(self, val: float) -> timedelta:
        """Build a ``timedelta`` corresponding to this unit and value.

        Args:
            val: Number of hours or days.
        Returns:
            A ``timedelta`` representing ``val`` in this unit.
        Raises:
            CliError: If the enum value is unsupported.
        """
        match self:
            case TimeUnit.HOURS:
                return timedelta(hours=val)
            case TimeUnit.DAYS:
                return timedelta(days=val)
            case default:
                assert_never(default)


def validate_array(
    kw: str, spec: str, vals: npt.NDArray[Any] | resfo.MESS
) -> npt.NDArray[Any]:
    """Validate that a RESFO entry value is a NumPy array.

    Args:
        kw: Keyword being read (for error messages).
        spec: Name of the source file/stream.
        vals: Value returned by ``resfo`` for ``kw``.
    Returns:
        ``vals`` if it is a NumPy array.
    Raises:
        CliError: If ``vals`` is of type ``resfo.MESS``.
    """
    if vals is resfo.MESS or isinstance(vals, resfo.MESS):
        raise CliError(f"{kw.strip()} in {spec} has incorrect type MESS")
    return vals


def decode_if_byte(key: bytes | str) -> str:
    """Decode a value that may be ``bytes`` into a UTF-8 string.
    Args:
        key: Bytes or string value.
    Returns:
        Decoded and stripped string.
    """
    return key.decode() if isinstance(key, bytes) else key


def key2str(key: bytes | str) -> str:
    """Normalize summary keyword values to a stripped Python ``str``.
    Args:
        key: Bytes or string value.
    Returns:
        The input as a string with leading/trailing whitespace removed.
    """
    return decode_if_byte(key).strip()


def patterns_to_matcher(patterns: Sequence[str]) -> Callable[[str], bool]:
    """
    Transform the list of keys (with * used as repeated wildcard) into
    a matcher.

    Examples:

        >>> match = _fetch_keys_to_matcher([""])
        >>> match("FOPR")
        False

        >>> match = _fetch_keys_to_matcher(["*"])
        >>> match("FOPR"), match("FO*")
        (True, True)


        >>> match = _fetch_keys_to_matcher(["F*PR"])
        >>> match("WOPR"), match("FOPR"), match("FGPR"), match("SOIL")
        (False, True, True, False)

        >>> match = _fetch_keys_to_matcher(["WGOR:*"])
        >>> match("FOPR"), match("WGOR:OP1"), match("WGOR:OP2"), match("WGOR")
        (False, True, True, False)

        >>> match = _fetch_keys_to_matcher(["FOPR", "FGPR"])
        >>> match("FOPR"), match("FGPR"), match("WGOR:OP2"), match("WGOR")
        (True, True, False, False)


    Args:
        patterns: Patterns (globs) to match against summary keys.


    Returns:
        A function that returns ``True`` if a given key matches any pattern.
    """
    if not patterns:
        return lambda _: False
    regex = re.compile("|".join(fnmatch.translate(key) for key in patterns))
    return lambda s: regex.fullmatch(s) is not None


@dataclass
class Spec:
    """The matched contents of a SMSPEC file

    Attributes:
        time_index: Index of the TIME entry in ``KEYWORDS``.
        start_date: start date of the summary (time=0.0).
        time_unit: Unit of the TIME vector (``Days`` or ``Hours``).
        matched_keywords: Keys selected for extraction.
        keyword_indices: Indices into PARAMS for the selected keys.
        restart: Restart case basename, if present, else ``None`` or empty string.
    """

    time_index: int
    start_date: datetime
    time_unit: TimeUnit
    matched_keywords: list[str]
    keyword_indices: npt.NDArray[np.int64]
    restart: str | None

    def order_keys_by(self, patterns: list[str]) -> None:
        """Reorder ``matched_keywords`` and ``keyword_indices`` by patterns.

        Args:
            patterns: List of explicit keys or glob patterns.
        """
        # The order of columns is implemented in this way
        # to preserve backwards-compatibility
        already_matched = set()
        new_matched_keywords = []
        for pat in patterns:
            if "*" in pat:
                for i in range(len(self.matched_keywords)):
                    kw = self.matched_keywords[i]
                    if kw in already_matched:
                        continue
                    if fnmatch.fnmatch(kw, pat):
                        new_matched_keywords.append((kw, self.keyword_indices[i]))
                        already_matched.add(kw)
                        new_matched_keywords = natsorted(
                            new_matched_keywords, key=lambda v: v[0]
                        )

            else:
                try:
                    i = self.matched_keywords.index(pat)
                    kw = self.matched_keywords[i]
                    new_matched_keywords.append((kw, self.keyword_indices[i]))
                    already_matched.add(kw)
                except ValueError:
                    logger.warning(
                        f"could not find variable: '{pat}' in summary file",
                    )
        self.matched_keywords = [k for k, _ in new_matched_keywords]
        self.keyword_indices = np.array(
            [i for _, i in new_matched_keywords], dtype=np.int64
        )


def read_spec(spec_opener: FileOpener, key_patterns: Sequence[str]) -> Spec:
    """Read an SMSPEC file and return a :class:`Spec` describing it.

    This function performs minimal validation, determines the index of the
    TIME vector and the unit, discovers all available keys, and filters them
    according to ``key_patterns``.

    Args:
        spec: Open file-like object for the SMSPEC (binary or text depending on format).
        key_patterns: Patterns identifying which keys to keep.

    Raises:
        CliError: On malformed content (e.g., missing UNITS, STARTDAT, etc.).
        resfo.ResfoParsingError: If ``resfo`` fails to parse the file.
    """
    date = None
    n = None
    nx = None
    ny = None
    wgnames = None
    spec_name = ""
    try:
        with spec_opener() as spec:
            spec_name = stream_name(spec)

            arrays: dict[str, npt.NDArray[Any] | None] = dict.fromkeys(
                [
                    "NUMS    ",
                    "KEYWORDS",
                    "NUMLX   ",
                    "NUMLY   ",
                    "NUMLZ   ",
                    "LGRS    ",
                    "UNITS   ",
                    "RESTART ",
                ],
                None,
            )
            for entry in resfo.lazy_read(spec):
                if all(p is not None for p in [date, n, nx, ny, *arrays.values()]):
                    break
                kw = entry.read_keyword()
                if kw in arrays:
                    arrays[kw] = validate_array(kw, spec_name, entry.read_array())
                if kw in {"WGNAMES ", "NAMES   "}:
                    wgnames = validate_array(kw, spec_name, entry.read_array())
                if kw == "DIMENS  ":
                    vals = validate_array(kw, spec_name, entry.read_array())
                    size = len(vals)
                    n = vals[0] if size > 0 else None
                    nx = vals[1] if size > 1 else None
                    ny = vals[2] if size > 2 else None
                if kw == "STARTDAT":
                    vals = validate_array(kw, spec_name, entry.read_array())
                    size = len(vals)
                    day = vals[0] if size > 0 else 0
                    month = vals[1] if size > 1 else 0
                    year = vals[2] if size > 2 else 0
                    hour = vals[3] if size > 3 else 0
                    minute = vals[4] if size > 4 else 0
                    microsecond = vals[5] if size > 5 else 0
                    try:
                        date = datetime(
                            day=day,
                            month=month,
                            year=year,
                            hour=hour,
                            minute=minute,
                            second=microsecond // 10**6,
                            microsecond=microsecond % 10**6,
                        )
                    except Exception as err:
                        raise CliError(
                            f"SMSPEC {spec} contains invalid STARTDAT: {err}"
                        ) from err
    except OSError as err:
        raise CliError(
            f"Could not read from summary spec {err.filename}: {err.strerror}"
        )

    keywords = arrays["KEYWORDS"]
    nums = arrays["NUMS    "]
    numlx = arrays["NUMLX   "]
    numly = arrays["NUMLY   "]
    numlz = arrays["NUMLZ   "]
    lgr_names = arrays["LGRS    "]

    if date is None:
        raise CliError(f"Keyword startdat missing in {spec_name}")
    if keywords is None:
        raise CliError(f"Keywords missing in {spec_name}")
    if n is None:
        n = len(keywords)
    elif n > len(keywords):
        logger.warning(
            f"** Warning: number of keywords given in DIMENS {n} is larger than the "
            f"length of KEYWORDS {len(keywords)}, truncating size to match",
        )
        n = len(keywords)

    indices: list[int] = []
    keys: list[str] = []
    index_mapping: dict[str, int] = {}
    date_index = None

    matcher = patterns_to_matcher(key_patterns)

    def should_load_key(kw):
        return kw != "TIME" and matcher(kw)

    def optional_get(arr: npt.NDArray[Any] | None, idx: int) -> Any:
        if arr is None:
            return None
        if len(arr) <= idx:
            return None
        return arr[idx]

    for i in range(n):
        keyword = key2str(keywords[i])
        if keyword == "TIME":
            date_index = i

        name = optional_get(wgnames, i)
        if name is not None:
            name = key2str(name)
        num = optional_get(nums, i)
        lgr_name = optional_get(lgr_names, i)
        if lgr_name is not None:
            lgr_name = key2str(lgr_name)
        li = optional_get(numlx, i)
        lj = optional_get(numly, i)
        lk = optional_get(numlz, i)

        try:
            key = make_summary_key(keyword, num, name, nx, ny, lgr_name, li, lj, lk)
        except InvalidSummaryKey as err:
            logger.info(
                f"{spec_name} contains invalid keyword '{keyword}': {err.args[0]}"
            )
            continue

        if should_load_key(key):
            if key in index_mapping:
                # only keep the index of the last occurrence of a key
                # this is done for backwards compatability
                # and to have unique keys
                indices[index_mapping[key]] = i
            else:
                index_mapping[key] = len(indices)
                indices.append(i)
                keys.append(key)

    keys_array = np.array(keys)
    rearranged = keys_array.argsort()
    keys_array = keys_array[rearranged]

    indices_array = np.array(indices, dtype=np.int64)[rearranged]

    units = arrays["UNITS   "]
    if units is None:
        raise CliError(f"Keyword units missing in {spec}")
    if date_index is None:
        raise CliError(f"KEYWORDS did not contain TIME in {spec}")
    if date_index >= len(units):
        raise CliError(f"Unit missing for TIME in {spec}")

    unit_key = key2str(units[date_index])
    try:
        date_unit = TimeUnit[unit_key]
    except KeyError:
        raise CliError(
            f"Unknown date unit in {stream_name(spec)}: {unit_key}"
        ) from None

    restart = arrays["RESTART "]
    if restart is None:
        restart = [""]
    restart = "".join(decode_if_byte(s) for s in restart).strip()
    if not os.path.isabs(restart):
        restart = os.path.join(os.path.dirname(spec_name), restart)

    return Spec(
        date_index,
        date,
        date_unit,
        list(keys_array),
        indices_array,
        restart=restart,
    )


def read_smry(
    summaries: list[FileOpener], spec: Spec, report_step_only: bool
) -> Iterator[tuple[float, npt.NDArray[np.float32]]]:
    """Iterate (time, values) tuples from a unified summary file.
    Args:
        summary: Open unified summary file (UNSMRY/FUNSMRY).
        spec: The :class:`Spec` produced by :func:`read_spec`.
        report_step_only: If ``True``, yield only at report steps (``DATES``).
    Yields:
        Tuples ``(time_value, values)`` where ``time_value`` is a float (days or hours)
        and ``values`` is an array of the selected keyword values in the order given by
        ``spec.keyword_indices`` at that step.
    """

    last_params = None
    try:
        for smry_opener in summaries:
            with smry_opener() as smry:
                summary_name = stream_name(smry)

                def read_params() -> Iterator[tuple[float, npt.NDArray[np.float32]]]:
                    nonlocal last_params
                    if last_params is not None:
                        vals = validate_array(
                            "PARAMS", summary_name, last_params.read_array()
                        )
                        last_params = None
                        yield vals[spec.time_index], vals[spec.keyword_indices]

                for entry in resfo.lazy_read(smry):
                    kw = entry.read_keyword()
                    if last_params and not report_step_only:
                        yield from read_params()
                    if kw == "PARAMS  ":
                        last_params = entry
                    if report_step_only and kw == "SEQHDR  ":
                        yield from read_params()
                yield from read_params()
    except OSError as err:
        raise CliError(
            f"Could not read from summary file {err.filename}: {err.strerror}"
        )


def print_header(keys: list[str]) -> None:
    """Print the header for tabular output.
    Args:
        keys: Ordered list of columns that will be printed.
    """
    header = (
        f"--{' Hours' if 'Hours' in keys else ''}{' Days' if 'Days' in keys else ''}"
        "   dd/mm/yyyy   "
        + "".join(
            f" {k.rjust(15)} "  # trailing whitespace for backwards-compatability
            for k in keys
            if k not in {"Hours", "Days", "dd/mm/yyyy"}
        )
    )
    print(header)
    print("-" * len(header))


def fetch_keys(
    smspec: FileOpener, patterns: list[str], fetch_restart: bool
) -> list[str]:
    """Get all keys in the case that matches the given patterns.

    Args:
        smspec: Open SMSPEC file.
        patterns: User-provided key patterns, will only fetch those that matches.
        fetch_restart: Whether to traverse and include keys from a restart case.
    Returns:
        A list of all matched key names in display order.
    """
    spec = read_spec(smspec, patterns)
    spec.order_keys_by(patterns)
    matched_keywords = spec.matched_keywords
    restart_keys = []
    if spec.restart and fetch_restart:
        restart_case = ExistingCase(
            warn_message=(
                lambda case_name: f"could not open restart case: '{case_name}'"
            ),
        )(spec.restart)
        if restart_case is not None:
            restart_keys = fetch_keys(restart_case[0], patterns, fetch_restart)
            already_in = set(matched_keywords)
            for rk in restart_keys:
                if rk not in already_in:
                    matched_keywords.append(rk)
                    already_in.add(rk)
    return matched_keywords


def list_keys(smspec: FileOpener, patterns: list[str], fetch_restart: bool) -> int:
    """List all keys in smspec that match the given patterns.

    Args:
        smspec: Open SMSPEC file.
        atterns: Patterns to filter keys; if empty, uses ``["*"]`` (all keys).
        fetch_restart: Whether to include keys from any restart case.

    Returns:
    ``0`` on success, ``-1`` otherwise (after printing a user-friendly error).
    """
    if not patterns:
        patterns = ["*"]
    try:
        matched_keywords = fetch_keys(smspec, patterns, fetch_restart)
        for i, key in enumerate(matched_keywords):
            print(f"{key:24s} ", end=None if i % 5 == 4 else "")
        print()
    except resfo.ResfoParsingError as err:
        logger.error(f"Could not read smspec {smspec}: {err.args[0]}")
        return -1
    except CliError as err:
        logger.error(err.args[0])
        return -1
    else:
        return 0


def read_case(
    smspec: FileOpener,
    summaries: list[FileOpener],
    patterns: list[str],
    report_only: bool,
    fetch_restart: bool,
) -> tuple[list[str], pd.DataFrame]:
    """Read a case and return selected keys as a DataFrame.

    Args:
        smspec: Open SMSPEC file.
        unsmry: Open unified summary file.
        patterns: Key patterns to select and order columns.
        report_only: If ``True``, restrict to report steps.
        fetch_restart: Whether to prepend data from a restart case, if present.

    Returns:
        A tuple ``(column_list, df)`` where ``column_list`` is the ordered set
        of all selected key names with duplicates, and ``df`` is the corresponding
        time series data.
    """
    spec = read_spec(smspec, patterns)
    spec.order_keys_by(patterns)
    restart_keys, restart_df = None, None
    if spec.restart and fetch_restart:
        restart_case = ExistingCase(
            warn_message=(
                lambda case_name: f"could not open restart case: '{case_name}'"
            ),
        )(spec.restart)
        if restart_case is not None:
            restart_keys, restart_df = read_case(
                *restart_case, patterns, report_only, fetch_restart
            )
    result = defaultdict(list)
    for date_val, values in read_smry(summaries, spec, report_only):
        date = spec.start_date + spec.time_unit.make_delta(float(date_val))
        result[spec.time_unit.value].append(date_val)
        result["dd/mm/yyyy"].append(date)
        already_added = set()
        for i, kw in enumerate(spec.matched_keywords):
            if kw not in already_added:
                result[kw].append(values[i])
                already_added.add(kw)
    df = pd.DataFrame(result)
    if restart_df is not None:
        restart_df = restart_df[restart_df["dd/mm/yyyy"] < df["dd/mm/yyyy"].min()]
        df = pd.concat([restart_df, df]).fillna(-99)

    all_matched = spec.matched_keywords.copy()
    already_in = set(all_matched)
    if restart_keys:
        for kw in restart_keys:
            if kw not in already_in:
                all_matched.append(kw)
    if spec.time_unit.value not in already_in:
        all_matched.append(spec.time_unit.value)
    return (all_matched, df)


def run(argv: list[str]) -> int:
    """Entry point for the CLI logic (non-``__main__`` usage).

    Parses arguments, reads the case, and prints the requested tabular output
    to stdout. Errors are printed to stderr and a non-zero exit status is returned.

    Args:
        argv: Full argument vector, typically ``sys.argv``.

    Returns:
        ``0`` on success, ``-1`` on user/parse errors.
    """
    args = parse_arguments(argv)
    setup_logging(args.verbose)
    smspec, unsmry = args.CASE
    if args.list or not args.keys:
        return list_keys(smspec, args.keys, args.restart)
    try:
        column_list, df = read_case(
            smspec, unsmry, args.keys, args.report_only, args.restart
        )
        if args.header:
            print_header(column_list)
        for _, row in df.iterrows():
            if "Hours" in column_list:
                print(f"{row['Hours']:7.4f}   ", end="")
            if "Days" in column_list:
                print(f"{row['Days']:7.2f}   ", end="")
            print(f"{row['dd/mm/yyyy'].strftime('%d/%m/%Y')}   ", end="")

            for c in column_list:
                if c in {"Hours", "Days", "dd/mm/yyyy"}:
                    continue
                # will have trailing whitespace for backwards-compatibility
                print(f" {row[c]:15.6g} ", end="")
            print()
    except resfo.ResfoParsingError as err:
        logger.error(f"Could not read case files {args.CASE}: {err.args[0]}")
        return -1
    except CliError as err:
        logger.error(err.args[0])
        return -1
    else:
        return 0


def has_extension(path: str, ext: str) -> bool:
    """
    >>> has_extension("ECLBASE.SMSPEC", "smspec")
    True
    >>> has_extension("BASE.SMSPEC", "smspec")
    False
    >>> has_extension("BASE.FUNSMRY", "smspec")
    False
    >>> has_extension("ECLBASE.smspec", "smspec")
    True
    >>> has_extension("ECLBASE.tar.gz.smspec", "smspec")
    True

    Args:
        path: File name to check.
        ext: Allowed extension regex.

    Returns:
        ``True`` if the file has any of the extensions in ``exts``.
    """
    if "." not in path:
        return False
    splitted = path.split(".")
    return re.fullmatch(ext, splitted[-1].lower()) is not None


def is_base_with_extension(base: str, path: str, ext: str) -> bool:
    """
    >>> is_base_with_extension("ECLBASE", "ECLBASE.SMSPEC", ["smspec"])
    True
    >>> is_base_with_extension("ECLBASE", "BASE.SMSPEC", ["smspec"])
    False
    >>> is_base_with_extension("ECLBASE", "BASE.FUNSMRY", ["smspec"])
    False
    >>> is_base_with_extension("ECLBASE", "ECLBASE.smspec", ["smspec"])
    True
    >>> is_base_with_extension("ECLBASE.tar.gz", "ECLBASE.tar.gz.smspec", ["smspec"])
    True

    Args:
        base: Basename without extension.
        path: Candidate path.
        exts: Allowed extension regex pattern.

    Returns:
        ``True`` if ``path`` is ``base`` with one of ``exts``.
    """
    if "." not in path:
        return False
    splitted = path.split(".")
    return (
        ".".join(splitted[0:-1]) == base
        and re.fullmatch(ext, splitted[-1].lower()) is not None
    )


ANY_SUMMARY_EXTENSION = r"unsmry|smspec|funsmry|fsmspec|s\d\d\d\d|a\d\d\d\d"


def get_summary_filenames(
    filepath: str,
) -> tuple[list[str], str]:
    directory, file_name = os.path.split(filepath)
    if "." in file_name:
        case_name = ".".join(file_name.split(".")[:-1])
    else:
        case_name = file_name
    specified_formatted = has_extension(file_name, r"funsmry|fsmspec|a\d\d\d\d")
    specified_unformatted = has_extension(file_name, r"unsmry|smspec|s\d\d\d\d")
    specified_unified = has_extension(file_name, "funsmry")
    specified_split = has_extension(file_name, r"x\d\d\d\d|a\d\d\d\d")
    spec_candidates, smry_candidates = tee(
        map(
            lambda x: os.path.join(directory, x),
            filter(
                lambda x: is_base_with_extension(
                    path=x, base=case_name, ext=ANY_SUMMARY_EXTENSION
                ),
                os.listdir(directory or "."),
            ),
        )
    )

    def filter_extension(ext: str, lst: Iterable[str]) -> Iterator[str]:
        return filter(partial(has_extension, ext=ext), lst)

    smry_candidates = filter_extension(
        r"unsmry|funsmry|s\d\d\d\d|a\d\d\d\d", smry_candidates
    )
    if specified_split:
        smry_candidates = filter_extension(r"s\d\d\d\d|a\d\d\d\d", smry_candidates)
    if specified_unified:
        smry_candidates = filter_extension("unsmry|funsmry", smry_candidates)
    if specified_formatted:
        smry_candidates = filter_extension("funsmry", smry_candidates)
    if specified_unformatted:
        smry_candidates = filter_extension("unsmry", smry_candidates)
    all_summary = natsorted(list(smry_candidates))
    summary = []
    pat = None
    for pat in ("unsmry", r"s\d\d\d\d", "funsmry", r"a\d\d\d\d"):
        summary = list(filter_extension(pat, all_summary))
        if summary:
            break

    if len(summary) != len(all_summary):
        logger.warning(f"More than one type of summary file, found {all_summary}")
    if not summary:
        raise argparse.ArgumentTypeError(
            f"Could not find any summary files matching {filepath}"
        )

    if pat in ("unsmry", r"s\d\d\d\d"):
        spec_candidates = filter_extension("smspec", spec_candidates)
    else:
        spec_candidates = filter_extension("fsmspec", spec_candidates)

    spec = list(spec_candidates)
    if len(spec) > 1:
        logger.warning(f"More than one type of summary spec file, found {spec}")

    if not spec:
        raise argparse.ArgumentTypeError(
            f"Could not find any summary spec matching {filepath}"
        )
    return summary, spec[-1]


class ExistingCase:
    """Argparse type/validator for existing reservoir simulation cases.


    Instances are callable and return a function that returns open file
    handles to the SMSPEC and summary files for a given case name, or
    ``None`` if validation is configured to warn instead of raising.
    """

    def __init__(self, warn_message: Callable[[str], str] | None = None) -> None:
        """
        Args:
            warn_message: takes the argument given on the command-line and
                returns a warning message in case of invalid case name. The message
                is appended with reason. If no warn_message is given then
                an error is given instead.

                warn_message=None will raise argparse.ArgumentTypeError, and validation
                of cli arguments fail.

                warn_message=lambda case_name: f"{case_name} is not a vaild case"
                    will print "SPE1 is not a valid case: could not find SPE1.UNSMRY"
                    to stderr and return None
        """
        self.warn_message = warn_message

    def __call__(self, case_name: str) -> tuple[FileOpener, list[FileOpener]] | None:
        try:
            summaries, spec = get_summary_filenames(case_name)
        except argparse.ArgumentTypeError as err:
            if self.warn_message is None:
                raise
            else:
                logger.warning(f"{self.warn_message(case_name)}: {err}")
                return None
        except Exception as err:
            if self.warn_message is None:
                raise argparse.ArgumentTypeError(
                    f"No summary data found for case: {case_name}'" + str(err)
                ) from err
            else:
                logger.warning(f"{self.warn_message(case_name)}: {err.args[0]}")
                return None
        mode = "rt" if spec.lower().endswith("fsmspec") else "rb"

        def opener(s):
            def inner():
                return open(os.path.abspath(s), mode)

            return inner

        result = (opener(spec), [opener(s) for s in summaries])
        try:
            # We open at this point to make sure files are
            # openable. We might still get an error later
            # that is due to the file changing which must
            # be handled separately
            result[0]().close()
            for r in result[1]:
                r().close()
        except OSError as err:
            if self.warn_message is None:
                raise argparse.ArgumentTypeError(
                    f"Could not open file {err.filename} for case {case_name}: "
                    + err.strerror
                    if err.strerror
                    else ""
                ) from err
            else:
                logger.warning(f"{self.warn_message(case_name)}: {err.strerror or ''}")
                return None
        else:
            return result


def setup_logging(verbosity: int):
    # Map verbosity count to logging levels
    if verbosity >= 2:
        level = logging.DEBUG
    elif verbosity == 1:
        level = logging.INFO
    else:
        level = logging.WARNING

    logging.basicConfig(
        level=level,
        format="** %(levelname)s: %(message)s",
        handlers=[logging.StreamHandler(sys.stderr)],
    )


def make_parser(prog: str = "summary.x") -> argparse.ArgumentParser:
    ap = argparse.ArgumentParser(
        prog=prog,
        description=dedent(
            """
        The summary.x program is used to quickly extract summary vectors
        from summary files. The program is invoked as:

        computer> summary.x /Path/to/CASE key1 key2 key3 ....

        Here CASE is the name of an existing case, you can give it with
        extension, or without; the case need not be in the current directory.

        The keys are formed by combining variable names and
        qualifiers from the WGNAMES and NUMS arrays. Examples of keys are:

           WWCT:F-36          - The watercut in the well F-36.
           FOPT               - The total field oil production.
           RPR:3              - The region pressure in region 3.
           GGIT:NORTH         - The total gas injection group NORTH.
           SPR:F-12:18        - The segment pressure in well F-12, segment 18.
           BPR:10,10,10       - The block pressure in cell 10,10,10.
           LBPR:LGR3:10,10,10 - The block pressure in cell 10,10,10 - in LGR3
        """
        ),
        formatter_class=argparse.RawTextHelpFormatter,
        epilog=dedent(
            """\
        The options should come before the basename.

        Example1:

          computer> summary.x  CASE1_XXX WWCT:F-36   FOPT   FWPT

          This example will load results from case 'CASE1_XXX' and print the
          results for keys 'WWCT:F-36', 'FOPT' and 'FWPT' on standard out.

        Example2:

          computer> summary.x  --list CASE2_XXX "*:F-36"  "BPR:*"

          This example will list all the available keys which end with
          ':F-36' and those which start with 'BPR:'. Observe the use of
          quoting characters "" when using shell wildcards.

        The summary.x program will look for and load both unified and
        non-unified and formatted and non-formatted files. The default
        search order is: UNSMRY, Snnnn, FUNSMRY, Annnn, however you can
        manipulate this with the extension to the basename:

        * If the extension corresponds to an unformatted file, summary.x
          will only look for unformatted files.

        * If the extension corresponds to a unified file, summary.x will
          only look for unified files."""
        ),
    )
    ap.add_argument(
        "CASE",
        type=ExistingCase(),
        help="name of an existing case, you can give it with extension, or without.",
    )
    ap.add_argument(
        "--list",
        action="store_true",
        help="This option can be used to list all available keys.",
    )
    ap.add_argument(
        "--restart",
        action=argparse.BooleanOptionalAction,
        default=True,
        help=dedent(
            """\
           If the simulation in question is a restart, i.e a prediction
           which starts at the end of the historical period, the summary.x
           program will by default also load historical data. If the --no-restart
           option is used the program will not look for old results."""
        ),
    )
    ap.add_argument(
        "--header",
        action=argparse.BooleanOptionalAction,
        default=True,
        help=dedent(
            """\
           By default summary.x will print a header line at the top, with the
           option --no-header this will be suppressed."""
        ),
    )
    ap.add_argument(
        "--report-only",
        action="store_true",
        help="Will only report results at report times (i.e. DATES).",
    )
    ap.add_argument(
        "-v",
        "--verbose",
        action="count",
        default=0,
        help="Increase output verbosity (e.g., -v, -vv, -vvv)",
    )
    ap.add_argument("keys", nargs="*", help="list of summary keys to extract.")
    return ap


def parse_arguments(argv: list[str]) -> argparse.Namespace:
    ap = make_parser(argv[0])
    return ap.parse_args(argv[1:])


def main() -> None:
    sys.exit(run(sys.argv))


if __name__ == "__main__":
    main()
