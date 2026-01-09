"""
This is the entry point for the command line utility
``summary.x``. See :func:`make_parser` for description
of the interface.

 See
[OPM flow 2024.04 manual Appendix F.9](https://opm-project.org/?page_id=955)
for specification of the summary file format.
"""

import argparse
import fnmatch
import logging
import warnings
import os
import re
import sys
from collections import defaultdict
from collections.abc import Iterator, Sequence
from dataclasses import dataclass
from datetime import datetime, timedelta
from enum import StrEnum
from textwrap import dedent
from typing import Callable, assert_never

import numpy as np
import numpy.typing as npt
import pandas as pd
from resfo_utilities import (
    SummaryReader,
    InvalidSummaryError,
    InvalidSummaryKeyError,
    make_summary_key,
)
from natsort import natsorted


logger = logging.getLogger(__name__)


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
          results for keys 'WWCT:F-36', 'FOPT' and 'FWPT' on standard out:

            -- Days   dd/mm/yyyy          WWCT:F-36             FOPT             FWPT
            -------------------------------------------------------------------------
               1.00   02/01/2014         5.6299e+16       5.6299e+16       5.6299e+16
               2.00   03/01/2014         5.6299e+16       5.6299e+16       5.6299e+16
               3.00   04/01/2014         5.6299e+16       5.6299e+16       5.6299e+16

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

    logging.captureWarnings(True)

    def custom_formatwarning(message, category, filename, lineno, line=None, file=None):
        return message

    warnings.formatwarning = custom_formatwarning
    sys.exit(run(sys.argv))


def run(argv: list[str]) -> int:
    """Entry point for the CLI logic.

    Parses arguments, reads the case, and prints the requested tabular output
    to stdout. Errors are printed to stderr and a non-zero exit status is returned.

    Args:
        argv: Full argument vector, typically ``sys.argv``.

    Returns:
        ``0`` on success, ``-1`` on user/parse errors.
    """
    args = parse_arguments(argv)
    setup_logging(args.verbose)
    if args.list or not args.keys:
        return list_keys(args.CASE, args.keys, args.restart)
    try:
        column_list, df = read_case(
            args.CASE, args.keys, args.report_only, args.restart
        )
        print_table(column_list, df, args.header)
    except InvalidSummaryError as err:
        logger.error(f"Could not read case files {args.CASE.case_path}: {err.args[0]}")
        return -1
    except CliError as err:
        logger.error(err.args[0])
        return -1
    else:
        return 0


def setup_logging(verbosity: int):
    """Map verbosity count to logging levels"""
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


def print_table(column_list: list[str], df: pd.DataFrame, header: bool) -> None:
    if header:
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


def list_keys(summary: SummaryReader, patterns: list[str], fetch_restart: bool) -> int:
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
        matched_keywords = fetch_keys(summary, patterns, fetch_restart)
        for i, key in enumerate(matched_keywords):
            print(f"{key:24s} ", end=None if i % 5 == 4 else "")
        print()
    except InvalidSummaryError as err:
        logger.error(f"Could not read smspec {summary.smspec_filename}: {err.args[0]}")
        return -1
    except CliError as err:
        logger.error(err.args[0])
        return -1
    else:
        return 0


def fetch_keys(
    summary: SummaryReader, patterns: list[str], fetch_restart: bool
) -> list[str]:
    """Get all keys in the case that matches the given patterns.

    Args:
        patterns: User-provided key patterns, will only fetch those that matches.
        fetch_restart: Whether to traverse and include keys from a restart case.
    Returns:
        A list of all matched key names in display order.
    """
    spec = read_spec(summary, patterns)
    spec.order_keys_by(patterns)
    matched_keywords = spec.matched_keywords
    restart_keys = []
    if spec.restart and fetch_restart:
        try:
            restart_case = ExistingCase(
                warn_message=(
                    lambda case_name: f"could not open restart case: '{case_name}'"
                ),
            )(spec.restart)
            if restart_case is not None:
                restart_keys = fetch_keys(restart_case, patterns, fetch_restart)
                already_in = set(matched_keywords)
                for rk in restart_keys:
                    if rk not in already_in:
                        matched_keywords.append(rk)
                        already_in.add(rk)
        except Exception as err:
            logger.error(f"Error while reading restart case: {err}")
    return matched_keywords


def read_case(
    summary: SummaryReader,
    patterns: list[str],
    report_only: bool,
    fetch_restart: bool,
) -> tuple[list[str], pd.DataFrame]:
    """Read a case and return selected keys as a DataFrame.

    Args:
        patterns: Key patterns to select and order columns.
        report_only: If ``True``, restrict to report steps.
        fetch_restart: Whether to prepend data from a restart case, if present.

    Returns:
        A tuple ``(column_list, df)`` where ``column_list`` is the ordered set
        of all selected key names with duplicates, and ``df`` is the corresponding
        time series data.
    """
    spec = read_spec(summary, patterns)
    spec.order_keys_by(patterns)
    restart_keys, restart_df = None, None
    if spec.restart and fetch_restart:
        try:
            restart_case = ExistingCase(
                warn_message=(
                    lambda case_name: f"could not open restart case: '{case_name}'"
                ),
            )(spec.restart)
            if restart_case:
                restart_keys, restart_df = read_case(
                    restart_case, patterns, report_only, fetch_restart
                )
        except Exception as err:
            logger.error(f"Error while reading restart case: {err}")
    result = defaultdict(list)
    for date_val, values in read_smry(summary, spec, report_only):
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
        """
        match self:
            case TimeUnit.HOURS:
                return timedelta(hours=val)
            case TimeUnit.DAYS:
                return timedelta(days=val)
            case default:
                assert_never(default)


@dataclass
class Spec:
    """The matched contents of a SMSPEC file.

    Attributes:
        time_index: Index of the TIME entry in the ``KEYWORDS`` array.
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
        sort_until = None
        for pat in patterns:
            if "*" in pat:
                for i in range(len(self.matched_keywords)):
                    kw = self.matched_keywords[i]
                    if kw in already_matched:
                        continue
                    if fnmatch.fnmatch(kw, pat):
                        new_matched_keywords.append((kw, self.keyword_indices[i]))
                        already_matched.add(kw)
                        sort_until = len(new_matched_keywords)

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
        if sort_until is not None:
            new_matched_keywords = (
                natsorted(new_matched_keywords[:sort_until], key=lambda v: v[0])
                + new_matched_keywords[sort_until:]
            )
        self.matched_keywords = [k for k, _ in new_matched_keywords]
        self.keyword_indices = np.array(
            [i for _, i in new_matched_keywords], dtype=np.int64
        )


def read_spec(summary: SummaryReader, key_patterns: Sequence[str]) -> Spec:
    """Read an SMSPEC file and return a :class:`Spec` describing it.

    This function performs validation, determines the index of the
    TIME vector and the unit, discovers all available keys, and filters them
    according to ``key_patterns``.

    Args:
        spec: A function that returns a file-like object for the
              SMSPEC (binary or text depending on format).
        key_patterns: Patterns identifying which keys to keep.

    Raises:
        CliError: On malformed content (e.g., missing UNITS, STARTDAT, etc.).
        InvalidSummaryError: If the smspec file contains invalid contents.
    """
    date = summary.start_date
    dims = summary.dimensions
    if dims is None:
        raise CliError(f"Keyword startdat missing in {summary.smspec_filename}")
    nx, ny = dims[0:2]
    keywords = summary.summary_keywords

    if date is None:
        raise CliError(f"Keyword startdat missing in {summary.smspec_filename}")

    indices: list[int] = []
    keys: list[str] = []
    index_mapping: dict[str, int] = {}
    date_index = None
    date_unit_str = None

    def patterns_to_matcher(patterns: Sequence[str]) -> Callable[[str], bool]:
        """
        Transform the list of fnmatch pattens (* used as repeated wildcard) into
        a matcher.

        Examples:

            >>> match = patterns_to_matcher(["*"])
            >>> match("FOPR"), match("FO*")
            (True, True)

            >>> match = patterns_to_matcher(["F*PR"])
            >>> match("WOPR"), match("FOPR"), match("FGPR"), match("SOIL")
            (False, True, True, False)

            >>> match = patterns_to_matcher(["WGOR:*"])
            >>> match("FOPR"), match("WGOR:OP1"), match("WGOR:OP2"), match("WGOR")
            (False, True, True, False)

            >>> match = patterns_to_matcher(["FOPR", "FGPR"])
            >>> match("FOPR"), match("FGPR"), match("WGOR:OP2"), match("WGOR")
            (True, True, False, False)

            >>> match = patterns_to_matcher([""])
            >>> match("FOPR")
            False

        """
        if not patterns:
            return lambda _: False
        regex = re.compile("|".join(fnmatch.translate(key) for key in patterns))
        return lambda s: regex.fullmatch(s) is not None

    matcher = patterns_to_matcher(key_patterns)

    def should_load_key(kw):
        return kw != "TIME" and matcher(kw)

    for i, kw in enumerate(keywords):
        try:
            key = make_summary_key(
                kw.summary_variable,
                kw.number,
                kw.name,
                nx,
                ny,
                kw.lgr_name,
                kw.li,
                kw.lj,
                kw.lk,
            )
            if kw.summary_variable == "TIME":
                date_index = i
                date_unit_str = kw.unit
        except InvalidSummaryKeyError as err:
            logger.info(
                f"{summary.smspec_filename} contains invalid"
                f" keyword '{kw.summary_variable}': {err.args[0]}"
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

    if date_index is None:
        raise CliError(f"KEYWORDS did not contain TIME in {summary.smspec_filename}")
    if date_unit_str is None:
        raise CliError(f"Unit missing for TIME in {summary.smspec_filename}")

    try:
        date_unit = TimeUnit[date_unit_str]
    except KeyError:
        raise CliError(
            f"Unknown date unit in {summary.smspec_filename}: {date_unit_str}"
        ) from None

    restart = summary.restart
    if restart and not os.path.isabs(restart):
        restart = os.path.join(os.path.dirname(summary.smspec_filename), restart)

    return Spec(
        date_index,
        date,
        date_unit,
        list(keys_array),
        indices_array,
        restart=restart,
    )


def read_smry(
    summary: SummaryReader, spec: Spec, report_step_only: bool
) -> Iterator[tuple[float, npt.NDArray[np.float32]]]:
    """Iterate (time, values) tuples from a unified summary file.
    Args:
        report_step_only: If ``True``, yield only at report steps (``DATES``).
    Yields:
        Tuples ``(time_value, values)`` where ``time_value`` is a float (days or hours)
        and ``values`` is an array of the selected keyword values in the order given by
        ``spec.keyword_indices`` at that step.
    Raises:
        InvalidSummaryError: If the smspec file contains invalid contents.
    """

    for v in summary.values(report_step_only):
        yield v[spec.time_index], v[spec.keyword_indices]


class ExistingCase:
    """Argparse type/validator for existing reservoir simulation cases.


    Instances are callable and return a SummaryReader
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

    def __call__(self, case_name: str) -> SummaryReader | None:
        try:
            summary = SummaryReader(case_path=case_name)
            logger.info(f"Found summary files for {case_name}")
            return summary
        except InvalidSummaryError as err:
            if self.warn_message is None:
                raise argparse.ArgumentTypeError(err) from err
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


if __name__ == "__main__":
    main()
