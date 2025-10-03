import argparse
import pandas as pd
from dataclasses import dataclass
from collections import defaultdict
import sys
from textwrap import dedent
import os
from typing import Callable, Sequence, Any, Iterator
import resfo
import numpy as np
import numpy.typing as npt
from datetime import datetime, timedelta
from enum import StrEnum
import fnmatch
import re
from .summary_key_type import make_summary_key


class CliError(ValueError):
    pass


class TimeUnit(StrEnum):
    HOURS = "Hours"
    DAYS = "Days"

    def make_delta(self, val: float) -> timedelta:
        if self == TimeUnit.HOURS:
            return timedelta(hours=val)
        if self == TimeUnit.DAYS:
            return timedelta(days=val)
        raise CliError(f"Unknown date unit {val}")


def check_vals(
    kw: str, spec: str, vals: npt.NDArray[Any] | resfo.MESS
) -> npt.NDArray[Any]:
    if vals is resfo.MESS or isinstance(vals, resfo.MESS):
        raise CliError(f"{kw.strip()} in {spec} has incorrect type MESS")
    return vals


def decode_if_byte(key: bytes | str) -> str:
    return key.decode() if isinstance(key, bytes) else key


def key2str(key: bytes | str) -> str:
    return decode_if_byte(key).strip()


def fetch_keys_to_matcher(fetch_keys: Sequence[str]) -> Callable[[str], bool]:
    """
    Transform the list of keys (with * used as repeated wildcard) into
    a matcher.

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
    """
    if not fetch_keys:
        return lambda _: False
    regex = re.compile("|".join(fnmatch.translate(key) for key in fetch_keys))
    return lambda s: regex.fullmatch(s) is not None


@dataclass
class Spec:
    time_index: int  # Index of the TIME in KEYWORDS
    start_date: datetime  # The start time of the summary (time=0.0)
    time_unit: TimeUnit  # The unit of the time index (DAYS or HOURS)
    matched_keywords: list[str]  # The name of matched keywords
    keyword_indecies: npt.NDArray[np.int64]  # The index of matched keywords
    restart: str | None

    def order_keys_by(self, patterns: list[str]) -> None:
        # The order of columns is implemented in this way
        # to preserve backwards-compatibility
        already_matched = set()
        new_indecies = []
        new_matched_keywords = []
        for pat in patterns:
            if "*" in pat:
                for i in range(len(self.matched_keywords)):
                    kw = self.matched_keywords[i]
                    if kw in already_matched:
                        continue
                    if fnmatch.fnmatch(kw, pat):
                        new_matched_keywords.append(kw)
                        new_indecies.append(self.keyword_indecies[i])
                        already_matched.add(kw)

            else:
                try:
                    i = self.matched_keywords.index(pat)
                    kw = self.matched_keywords[i]
                    new_matched_keywords.append(kw)
                    new_indecies.append(self.keyword_indecies[i])
                    already_matched.add(kw)
                except ValueError:
                    print(
                        f"** Warning: could not find variable: '{pat}' in summary file",
                        file=sys.stderr,
                    )
        self.matched_keywords = new_matched_keywords
        self.keyword_indecies = np.array(new_indecies, dtype=np.int64)


def read_spec(spec: str, fetch_keys: Sequence[str]) -> Spec:
    date = None
    n = None
    nx = None
    ny = None
    wgnames = None

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
    if spec.lower().endswith("fsmspec"):
        mode = "rt"
        assumed_format = resfo.Format.FORMATTED
    else:
        mode = "rb"
        assumed_format = resfo.Format.UNFORMATTED

    with open(spec, mode) as fp:
        for entry in resfo.lazy_read(fp, assumed_format):
            if all(p is not None for p in [date, n, nx, ny, *arrays.values()]):
                break
            kw = entry.read_keyword()
            if kw in arrays:
                arrays[kw] = check_vals(kw, spec, entry.read_array())
            if kw in {"WGNAMES ", "NAMES   "}:
                wgnames = check_vals(kw, spec, entry.read_array())
            if kw == "DIMENS  ":
                vals = check_vals(kw, spec, entry.read_array())
                size = len(vals)
                n = vals[0] if size > 0 else None
                nx = vals[1] if size > 1 else None
                ny = vals[2] if size > 2 else None
            if kw == "STARTDAT":
                vals = check_vals(kw, spec, entry.read_array())
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
    keywords = arrays["KEYWORDS"]
    nums = arrays["NUMS    "]
    numlx = arrays["NUMLX   "]
    numly = arrays["NUMLY   "]
    numlz = arrays["NUMLZ   "]
    lgr_names = arrays["LGRS    "]

    if date is None:
        raise CliError(f"Keyword startdat missing in {spec}")
    if keywords is None:
        raise CliError(f"Keywords missing in {spec}")
    if n is None:
        n = len(keywords)

    indices: list[int] = []
    keys: list[str] = []
    index_mapping: dict[str, int] = {}
    date_index = None

    def should_load_key(kw):
        return kw != "TIME" and fetch_keys_to_matcher(fetch_keys)

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

        key = make_summary_key(keyword, num, name, nx, ny, lgr_name, li, lj, lk)
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
        raise CliError(f"Unknown date unit in {spec}: {unit_key}") from None

    restart = arrays["RESTART "]
    restart = [""] if restart is None else restart

    return Spec(
        date_index,
        date,
        date_unit,
        list(keys_array),
        indices_array,
        restart="".join(decode_if_byte(s) for s in restart).strip(),
    )


def read_unsmry(
    summary: str, spec: Spec, report_step_only: bool
) -> Iterator[tuple[float, npt.NDArray[np.float32]]]:
    if summary.lower().endswith("funsmry"):
        mode = "rt"
        assumed_format = resfo.Format.FORMATTED
    else:
        mode = "rb"
        assumed_format = resfo.Format.UNFORMATTED

    last_params = None

    def read_params() -> Iterator[tuple[float, npt.NDArray[np.float32]]]:
        nonlocal last_params
        if last_params is not None:
            vals = check_vals("PARAMS", summary, last_params.read_array())
            last_params = None
            yield vals[spec.time_index], vals[spec.keyword_indecies]

    with open(summary, mode) as fp:
        for entry in resfo.lazy_read(fp, assumed_format):
            kw = entry.read_keyword()
            if last_params and not report_step_only:
                yield from read_params()
            if kw == "PARAMS  ":
                last_params = entry
            if report_step_only and kw == "SEQHDR  ":
                yield from read_params()
        yield from read_params()


def print_header(keys: list[str]) -> None:
    header = (
        f"--{' Hours' if 'Hours' in keys else ''}{' Days' if 'Days' in keys else ''}   dd/mm/yyyy   "
        + "".join(
            f" {k.rjust(15)} "  # trailing whitespace for backwards-compatability
            for k in keys
            if k not in {"Hours", "Days", "dd/mm/yyyy"}
        )
    )
    print(header)
    print("-" * len(header))


def list_mode(smspec: str, keys: list[str]) -> int:
    if not keys:
        keys = ["*"]
    try:
        spec = read_spec(smspec, keys)
        spec.order_keys_by(keys)
        for i, key in enumerate(spec.matched_keywords):
            print(f"{key:24s} ", end=None if i % 5 == 4 else "")
        print()
        return 0
    except resfo.ResfoParsingError as err:
        print(f"Could not read smspec {smspec}: {err.args[0]}", file=sys.stderr)
        return -1
    except CliError as err:
        print(err.args[0], file=sys.stderr)
        return -1


def read(
    smspec: str, unsmry: str, keys: list[str], report_only: bool, fetch_restart: bool
) -> tuple[list[str], pd.DataFrame]:
    spec = read_spec(smspec, keys)
    spec.order_keys_by(keys)
    if spec.restart and fetch_restart:
        restart_keys, restart_df = read(
            *ExistingCase()(spec.restart), keys, report_only, fetch_restart
        )
    else:
        restart_keys, restart_df = None, None
    result = defaultdict(list)
    for date_val, values in read_unsmry(unsmry, spec, report_only):
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
    args = parse_arguments(argv)
    smspec, unsmry = args.CASE
    if args.list:
        return list_mode(smspec, args.keys)
    try:
        column_list, df = read(
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
        return 0
    except resfo.ResfoParsingError as err:
        print(f"Could not read case files {args.CASE}: {err.args[0]}", file=sys.stderr)
        return -1
    except CliError as err:
        print(err.args[0], file=sys.stderr)
        return -1


def has_extension(path: str, exts: list[str]) -> bool:
    """
    >>> has_extension("ECLBASE.SMSPEC", ["smspec"])
    True
    >>> has_extension("BASE.SMSPEC", ["smspec"])
    False
    >>> has_extension("BASE.FUNSMRY", ["smspec"])
    False
    >>> has_extension("ECLBASE.smspec", ["smspec"])
    True
    >>> has_extension("ECLBASE.tar.gz.smspec", ["smspec"])
    True
    """
    if "." not in path:
        return False
    splitted = path.split(".")
    return splitted[-1].lower() in exts


def is_base_with_extension(base: str, path: str, exts: list[str]) -> bool:
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
    """
    if "." not in path:
        return False
    splitted = path.split(".")
    return ".".join(splitted[0:-1]) == base and splitted[-1].lower() in exts


def is_funsmry(base: str, path: str) -> bool:
    return is_base_with_extension(base, path, ["funsmry"])


def is_fsmspec(base: str, path: str) -> bool:
    return is_base_with_extension(base, path, ["fsmspec"])


def is_unsmry(base: str, path: str) -> bool:
    return is_base_with_extension(base, path, ["unsmry"])


def is_smspec(base: str, path: str) -> bool:
    return is_base_with_extension(base, path, ["smspec"])


def is_any_unsmry(base: str, path: str) -> bool:
    return is_base_with_extension(base, path, ["unsmry", "funsmry"])


def is_any_smspec(base: str, path: str) -> bool:
    return is_base_with_extension(base, path, ["smspec", "fsmspec"])


def find_file_matching(
    kind: str, case: str, predicate: Callable[[str, str], bool]
) -> str:
    directory, base = os.path.split(case)
    candidates = list(
        filter(lambda x: predicate(base, x), os.listdir(directory or "."))
    )
    if not candidates:
        raise FileNotFoundError(f"Could not find any {kind} matching case path {case}")
    if len(candidates) > 1:
        raise argparse.ArgumentTypeError(
            f"Ambiguous reference to {kind} in {case}, could be any of: {', '.join(candidates)}"
        )
    return os.path.join(directory, candidates[0])


def get_summary_filenames(
    filepath: str, specified_formatting: bool | None
) -> tuple[str, str]:
    match specified_formatting:
        case None:
            summary = find_file_matching(
                "unified summary file", filepath, is_any_unsmry
            )
            spec = find_file_matching("smspec file", filepath, is_any_smspec)
            return summary, spec
        case True:
            summary = find_file_matching("unified summary file", filepath, is_funsmry)
            spec = find_file_matching("smspec file", filepath, is_fsmspec)
            return summary, spec
        case False:
            summary = find_file_matching("unified summary file", filepath, is_unsmry)
            spec = find_file_matching("smspec file", filepath, is_smspec)
            return summary, spec


class ExistingCase:
    def __call__(self, case_name: str) -> tuple[str, str]:
        specified_formatting = None
        file_name = case_name
        if has_extension(case_name, ["unsmry", "smspec", "funsmry", "fsmspec"]):
            specified_formatting = has_extension(case_name, ["funsmry", "fsmspec"])
            case_name = ".".join(case_name.split(".")[:-1])
        try:
            summary, spec = get_summary_filenames(case_name, specified_formatting)
        except FileNotFoundError as err:
            raise argparse.ArgumentTypeError(
                f"No summary data found for case: {file_name}'"
            ) from err
        return spec, summary


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
    ap.add_argument("keys", nargs="*", help="list of summary keys to extract.")
    return ap


def parse_arguments(argv: list[str]) -> argparse.Namespace:
    ap = make_parser(argv[0])
    return ap.parse_args(argv[1:])


def main() -> None:
    sys.exit(run(sys.argv))


if __name__ == "__main__":
    main()
