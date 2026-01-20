from view_summary.__main__ import parse_arguments, run
from contextlib import suppress
import resfo
import logging
from collections import defaultdict
from textwrap import dedent
import pytest
from pathlib import Path
from hypothesis import given, settings, HealthCheck
import hypothesis.strategies as st
from io import StringIO
from resfo_utilities.testing import (
    summaries,
    Unsmry,
    SummaryMiniStep,
    SummaryStep,
    Smspec,
    Date,
    SmspecIntehead,
    Simulator,
    UnitSystem,
    summary_variables,
)
import pandas as pd
import numpy as np


def test_help_string(capsys):
    with pytest.raises(SystemExit) as sysexit:
        parse_arguments(["summary.x", "-h"])
    assert sysexit.value.code == 0
    captured = capsys.readouterr()
    assert captured.out == """\
usage: summary.x [-h] [--list] [--restart | --no-restart]
                 [--header | --no-header] [--report-only] [-v]
                 CASE [keys ...]

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

positional arguments:
  CASE                  name of an existing case, you can give it with extension, or without.
  keys                  list of summary keys to extract.

options:
  -h, --help            show this help message and exit
  --list                This option can be used to list all available keys.
  --restart, --no-restart
                        If the simulation in question is a restart, i.e a prediction
                        which starts at the end of the historical period, the summary.x
                        program will by default also load historical data. If the --no-restart
                        option is used the program will not look for old results.
  --header, --no-header
                        By default summary.x will print a header line at the top, with the
                        option --no-header this will be suppressed.
  --report-only         Will only report results at report times (i.e. DATES).
  -v, --verbose         Increase output verbosity (e.g., -v, -vv, -vvv)

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
  only look for unified files.
"""


def test_that_giving_non_existing_case_is_invalid(tmp_path, capsys):
    try:
        parse_arguments(["summary.x", str(tmp_path / "DOES_NOT_EXIST")])
    except BaseException:
        # will produce exit code 2
        pass
    assert (
        f"Could not find any summary files matching {tmp_path / 'DOES_NOT_EXIST'}"
        in capsys.readouterr().err
    )


def create_summary(
    summary_keys=("FOPR",),
    time_units="DAYS",
    case="TEST",
    formatted="",
    names=None,
    numbers=None,
    restart=None,
    start_date=None,
    times=(0.0,),
):
    start_date = start_date or Date(
        day=1, month=1, year=2014, hour=0, minutes=0, micro_seconds=0
    )
    num_values = len(summary_keys)
    unsmry = Unsmry(
        steps=[
            SummaryStep(
                seqnum=i,
                ministeps=[
                    SummaryMiniStep(
                        mini_step=0, params=[time] + [5.629901e16] * num_values
                    ),
                ],
            )
            for i, time in enumerate(times)
        ]
    )
    smspec = Smspec(
        nx=2,
        ny=2,
        nz=2,
        restarted_from_step=0,
        num_keywords=1 + len(summary_keys),
        restart=restart if restart else "        ",
        keywords=["TIME    ", *summary_keys],
        well_names=(
            [":+:+:+:+", *(["A_NAME  "] * len(summary_keys))]
            if names is None
            else names
        ),
        region_numbers=(
            [-32676, *([0] * len(summary_keys))] if numbers is None else numbers
        ),
        units=[time_units.ljust(8), *(["SM3"] * len(summary_keys))],
        start_date=start_date,
        intehead=SmspecIntehead(
            unit=UnitSystem.METRIC,
            simulator=Simulator.ECLIPSE_100,
        ),
    )
    format = resfo.Format.FORMATTED if formatted == "F" else resfo.Format.UNFORMATTED
    smspec.to_file(f"{case}.{formatted}SMSPEC", file_format=format)
    unsmry.to_file(f"{case}.{formatted}UNSMRY", file_format=format)


@pytest.mark.usefixtures("use_tmpdir")
@pytest.mark.parametrize("formatted", ["", "F"])
def test_that_giving_basename_of_existing_summary_files_is_valid(tmp_path, formatted):
    create_summary(formatted=formatted)
    parse_arguments(["summary.x", str(tmp_path / "TEST")])


@pytest.mark.usefixtures("use_tmpdir")
@pytest.mark.parametrize("formatted", ["", "F"])
def test_that_giving_file_name_with_extension_of_existing_summary_files_is_valid(
    tmp_path, formatted
):
    create_summary(formatted=formatted)
    parse_arguments(["summary.x", str(tmp_path / f"TEST.{formatted}SMSPEC")])
    parse_arguments(["summary.x", str(tmp_path / f"TEST.{formatted}UNSMRY")])


@pytest.mark.usefixtures("use_tmpdir")
def test_that_case_name_with_formatted_extension_does_not_match_unformatted_summaries(
    tmp_path, capsys
):
    create_summary()
    try:
        parse_arguments(["summary.x", str(tmp_path / "TEST.FSMSPEC")])
    except BaseException:
        # will produce exit code 2
        pass
    assert f"Could not find any summary files matching" in capsys.readouterr().err


@pytest.mark.usefixtures("use_tmpdir")
def test_that_case_name_with_unformatted_extension_does_not_match_formatted_summaries(
    tmp_path, capsys
):
    try:
        create_summary(
            formatted="F"  # creates file named "TEST.FSMSPEC", not "TEST.SMSPEC"
        )
        parse_arguments(["summary.x", str(tmp_path / "TEST.SMSPEC")])
    except BaseException:
        # will produce exit code 2
        pass
    assert f"Could not find any summary files matching" in capsys.readouterr().err


@pytest.fixture
def run_cli(capsys, use_tmpdir):
    def inner(cli_args, *args, **kwargs):
        create_summary(*args, **kwargs)
        capsys.readouterr()  # Ensure that captured output is empty at the start
        run(["summary.x", kwargs.get("CASE", "TEST"), *cli_args])
        return capsys.readouterr()

    return inner


@pytest.mark.usefixtures("use_tmpdir")
def test_that_by_default_a_header_is_written(run_cli):
    run_cli(cli_args=("FGIP",), summary_keys=("FGIP",)).out.startswith(
        "-- Days   dd/mm/yyyy               FGIP\n"
    )


@pytest.mark.usefixtures("use_tmpdir")
def test_that_header_can_be_omitted_by_cli_option(run_cli):
    captured = run_cli(cli_args=("FGIP", "--no-header"), summary_keys=("FGIP",))
    assert "-- Days   dd/mm/yyyy               FGIP\n" not in captured.out


@pytest.mark.usefixtures("use_tmpdir")
def test_that_header_includes_all_matching_keys_from_input_with_correct_indent(capsys):
    create_summary(summary_keys=("FGIP", "FOPR", "FWPT", "FOPT"))

    run(["summary.x", "TEST", "FGIP", "FOPR"])
    assert capsys.readouterr().out.startswith(
        "-- Days   dd/mm/yyyy               FGIP             FOPR "
    )

    run(["summary.x", "TEST", "FGIP", "FOPR", "FWPT"])
    assert capsys.readouterr().out.startswith(
        "-- Days   dd/mm/yyyy               FGIP             FOPR             FWPT"
    )

    run(["summary.x", "TEST", "FGIP", "FOPR", "*"])
    assert capsys.readouterr().out.startswith(
        "-- Days   dd/mm/yyyy               FGIP             FOPR"
        "             FOPT             FWPT"
    )


def keys_in_header(output):
    return output.splitlines()[0].split()[3:]


def test_that_the_list_option_prints_matched_keys(run_cli):
    assert run_cli(
        cli_args=("--list",),
        summary_keys=(
            "FGIP",
            "FGIP",
            "FGIT",
            "FGOR",
            "FGPR",
            "FGPT",
            "FOIP",
            "FOIPG",
            "FOIPL",
        ),
    ).out == dedent(
        """\
            FGIP                     FGIT                     FGOR                     FGPR                     FGPT                     
            FOIP                     FOIPG                    FOIPL                    
            """
    )


def test_that_empty_key_list_also_prints_matched_keys(run_cli):
    assert run_cli(
        cli_args=tuple(),
        summary_keys=(
            "FGIP",
            "FGIP",
            "FGIT",
            "FGOR",
            "FGPR",
            "FGPT",
            "FOIP",
            "FOIPG",
            "FOIPL",
        ),
    ).out == dedent(
        """\
            FGIP                     FGIT                     FGOR                     FGPR                     FGPT                     
            FOIP                     FOIPG                    FOIPL                    
            """
    )


@pytest.mark.usefixtures("use_tmpdir")
def test_that_keys_from_restart_is_listed_by_default_and_controlled_by_cli_option(
    capsys,
):
    create_summary(case="RESTART", summary_keys=("FOPT",))
    create_summary(restart="RESTART", summary_keys=("FOPR", "FGIP"))

    run(["summary.x", "TEST"])
    assert capsys.readouterr().out.split() == ["FGIP", "FOPR", "FOPT"]

    run(["summary.x", "--no-restart", "TEST"])
    assert capsys.readouterr().out.split() == ["FGIP", "FOPR"]

    run(["summary.x", "--list", "TEST", "FO*"])
    assert capsys.readouterr().out.split() == ["FOPR", "FOPT"]


def test_that_for_non_wildcard_keywords_the_order_of_columns_is_as_in_the_input(capsys):
    create_summary(summary_keys=("FGIP", "FOPR", "FWPT", "FOPT"))

    run(["summary.x", "TEST", "FGIP", "FOPR"])
    assert keys_in_header(capsys.readouterr().out) == ["FGIP", "FOPR"]

    run(["summary.x", "TEST", "FOPR", "FGIP"])
    assert keys_in_header(capsys.readouterr().out) == ["FOPR", "FGIP"]


def test_that_for_non_wildcard_keywords_are_repeated_if_repeated_in_the_input(run_cli):
    assert keys_in_header(
        run_cli(
            cli_args=("summary.x", "TEST", "FGIP", "FGIP"),
            summary_keys=("FGIP", "FOPR", "FWPT", "FOPT"),
        ).out
    ) == ["FGIP", "FGIP"]


def test_that_keywords_matching_wildcard_is_omitted_if_already_in_header(run_cli):
    assert sorted(
        keys_in_header(
            run_cli(
                cli_args=("summary.x", "TEST", "FOPT", "*"),
                summary_keys=("FGIP", "FOPR", "FWPT", "FOPT"),
            ).out
        )
    ) == sorted(["FOPT", "FGIP", "FOPR", "FWPT"])


@pytest.mark.usefixtures("use_tmpdir")
def test_that_using_wildcard_keys_causes_preceding_headers_to_be_sorted(capsys):
    create_summary(
        summary_keys=["FOPR"] + ["WWIR"] * 5 + ["WBHP"] * 5,
        names=["", ""] + [f"AQ_{i}" for i in range(1, 6)] * 2,
    )
    run(["summary.x", "-v", "TEST", "WWIR:AQ*", "WBHP:AQ*", "FOPR"])
    assert keys_in_header(capsys.readouterr().out) == [
        "WBHP:AQ_1",
        "WBHP:AQ_2",
        "WBHP:AQ_3",
        "WBHP:AQ_4",
        "WBHP:AQ_5",
        "WWIR:AQ_1",
        "WWIR:AQ_2",
        "WWIR:AQ_3",
        "WWIR:AQ_4",
        "WWIR:AQ_5",
        "FOPR",  # Not sorted because FOPR comes after wildcards
    ]


@pytest.mark.usefixtures("use_tmpdir")
def test_that_sorting_columns_is_done_numerically(capsys):
    create_summary(
        summary_keys=[
            "WWIR",
            "WWIR",
            "WWIR",
            "WWIR",
            "WWIR",
            "WWIR",
            "WOPT",
            "WOPT",
        ],
        names=[
            "",
            "I-3",
            "I-2",
            "I-10",
            "I-9",
            "I-10B",
            "I-11",
            "P-9",
            "P-10",
        ],
    )
    run(["summary.x", "TEST", "*"])
    assert keys_in_header(capsys.readouterr().out) == [
        "WOPT:P-9",
        "WOPT:P-10",
        "WWIR:I-2",
        "WWIR:I-3",
        "WWIR:I-9",
        "WWIR:I-10",
        "WWIR:I-10B",
        "WWIR:I-11",
    ]


def test_that_header_displays_the_time_units_from_spec(run_cli):
    run_cli(cli_args="FGIP", summary_keys=("FGIP",), time_units="HOURS").out.startswith(
        "-- Hours   dd/mm/yyyy"
    )


def test_that_a_warning_is_displayed_for_unmatched_patterns(run_cli, caplog):
    run_cli(cli_args=("UNK",), summary_keys=("FGIP", "FOPR", "FWPT", "FOPT"))
    assert any("could not find variable: 'UNK'" in r.message for r in caplog.records)


patterns = st.one_of(st.lists(summary_variables(), min_size=1), st.just("*"))


def report_step_value(unsmry: Unsmry, report_step: int, kw_index: int):
    return unsmry.steps[report_step].ministeps[-1].params[kw_index]


def output_as_df(output: str):
    lines = output.splitlines()
    del lines[1]  # Remove separator between header and cells
    lines[0] = lines[0][3:]  # Remove initial "-- "
    lines = [line.strip() for line in lines]  # remove initial and trailing whitespace

    dtypes = defaultdict(lambda: "Float64")
    dtypes["dd/mm/yyyy"] = "str"
    return pd.read_csv(StringIO("\n".join(lines)), sep=r"\s+", dtype=dtypes)  # type: ignore


@given(summary=summaries(), patterns=st.lists(patterns, min_size=1))
@pytest.mark.usefixtures("use_tmpdir")
@settings(suppress_health_check=[HealthCheck.function_scoped_fixture])
def test_that_value_placed_in_given_cell_is_the_value_from_the_summary_report_steps(
    summary, patterns, capsys
):
    capsys.readouterr()  # Ensure that captured output is empty at the start
    smspec, unsmry = summary
    smspec.to_file("TEST.SMSPEC")
    unsmry.to_file("TEST.UNSMRY")
    run(["summary.x", "--no-restart", "--report-only", "TEST", *patterns])
    output = capsys.readouterr().out
    df = output_as_df(output)

    for kw_index, keyword in enumerate(smspec.summary_keys()):
        if keyword in df.columns:
            for report_step, val in enumerate(df[keyword]):
                assert report_step_value(
                    unsmry, report_step, kw_index
                ) == pytest.approx(np.float32(f"{val:15.6g}"), abs=1.0e-5, rel=1.0e-5)


def step_value(unsmry: Unsmry, index: int, kw_index: int):
    while index >= 0:
        for step in unsmry.steps:
            if index < len(step.ministeps):
                return step.ministeps[index].params[kw_index]
            index -= len(step.ministeps)


@given(summary=summaries(), patterns=st.lists(patterns, min_size=1))
@pytest.mark.usefixtures("use_tmpdir")
@settings(suppress_health_check=[HealthCheck.function_scoped_fixture])
def test_that_value_placed_in_given_cell_is_the_value_from_the_step(
    summary, patterns, capsys
):
    smspec, unsmry = summary
    smspec.to_file("TEST.SMSPEC")
    unsmry.to_file("TEST.UNSMRY")
    capsys.readouterr()  # Ensure that captured output is empty at the start
    run(["summary.x", "--no-restart", "TEST", *patterns])
    df = output_as_df(capsys.readouterr().out)

    for kw_index, keyword in enumerate(smspec.summary_keys()):
        if keyword in df.columns:
            for step, val in enumerate(df[keyword]):
                assert step_value(unsmry, step, kw_index) == pytest.approx(
                    np.float32(f"{val:15.6g}"), abs=1.0e-5, rel=1.0e-5
                )


@pytest.mark.usefixtures("use_tmpdir")
def test_that_the_restart_is_read_by_default_and_controlled_by_cli_option(capsys):
    create_summary(
        case="RESTART", summary_keys=("FGIP", "FOPR", "FWPT", "FOPT"), times=[1.0]
    )
    create_summary(
        restart="RESTART", summary_keys=("FGIP", "FOPR", "FWPT", "FOPT"), times=[2.0]
    )

    run(["summary.x", "TEST", "FGIP", "FOPR"])
    assert len(output_as_df(capsys.readouterr().out)) == 2

    run(["summary.x", "--no-restart", "TEST", "FGIP", "FOPR"])
    assert len(output_as_df(capsys.readouterr().out)) == 1


@pytest.mark.usefixtures("use_tmpdir")
def test_that_case_and_restart_columns_are_merged_when_they_differ(capsys):
    create_summary(case="RESTART", summary_keys=("FOPR",), times=[0.5])
    create_summary(restart="RESTART", summary_keys=("FWPT",), times=[1.0])

    run(["summary.x", "TEST", "*"])
    df = output_as_df(capsys.readouterr().out)
    assert set(df.columns) == {"Days", "dd/mm/yyyy", "FOPR", "FWPT"}
    assert df["FOPR"].to_list() == pytest.approx([5.6299e16, -99.0])
    assert df["FWPT"].to_list() == pytest.approx([-99.0, 5.6299e16])


@pytest.mark.usefixtures("use_tmpdir")
def test_that_missing_restart_warns(capsys, caplog):
    create_summary(restart="RESTART", summary_keys=("FWPT",))

    run(["summary.x", "TEST", "*"])
    capture = capsys.readouterr()
    df = output_as_df(capture.out)
    assert set(df.columns) == {"Days", "dd/mm/yyyy", "FWPT"}

    assert any("could not open restart case" in r.message for r in caplog.records)


@pytest.mark.usefixtures("use_tmpdir")
def test_that_unopenable_restart_warns(capsys, caplog):
    try:
        create_summary(case="RESTART", summary_keys=("FOPR",))
        Path("RESTART.UNSMRY").chmod(0x000)

        create_summary(restart="RESTART", summary_keys=("FWPT",))

        run(["summary.x", "TEST", "*"])
        capture = capsys.readouterr()
        df = output_as_df(capture.out)
        assert set(df.columns) == {"Days", "dd/mm/yyyy", "FWPT"}

        assert any(
            "Error while reading restart case" in r.message for r in caplog.records
        )
    finally:
        with suppress(FileNotFoundError):
            Path("RESTART.UNSMRY").chmod(0x777)


@pytest.mark.usefixtures("use_tmpdir")
def test_that_unknown_time_unit_gives_informative_error_message(caplog):
    create_summary(time_units="YEARS")
    run(["summary.x", "--no-restart", "TEST", "*"])
    assert any("Unknown date unit " in r.message for r in caplog.records)


@pytest.mark.usefixtures("use_tmpdir")
@given(summary=summaries())
@settings(suppress_health_check=[HealthCheck.function_scoped_fixture])
def test_that_missing_time_keyword_in_smspec_gives_informative_error_message(
    caplog, summary
):
    caplog.clear()  # Ensure that captured output is empty at the start
    smspec, unsmry = summary
    smspec.keywords = smspec.keywords[1:]  # remove TIME
    smspec.to_file("TEST.SMSPEC")
    unsmry.to_file("TEST.UNSMRY")
    with pytest.warns(match="number of keywords given in DIMENS"):
        run(["summary.x", "TEST", "*"])
    assert any("did not contain TIME" in r.message for r in caplog.records)


@pytest.mark.usefixtures("use_tmpdir")
def test_that_invalid_start_date_is_reported(caplog):
    create_summary(
        start_date=Date(
            day=32,  # impossible date
            month=13,
            year=2014,
            hour=0,
            minutes=0,
            micro_seconds=0,
        )
    )
    run(["summary.x", "TEST", "*"])
    assert any("contains invalid STARTDAT" in r.message for r in caplog.records)


@pytest.mark.usefixtures("use_tmpdir")
def test_that_invalid_variables_are_ignored(capsys, caplog):
    caplog.set_level(logging.INFO)
    create_summary(summary_keys=("FOPR", "WOPR"), names=[":+:+:+:+"])
    run(["summary.x", "TEST", "*"])
    capture = capsys.readouterr()

    assert keys_in_header(capture.out) == ["FOPR"]
    assert any(
        "TEST.SMSPEC contains invalid keyword 'WOPR': well keyword without name"
        in r.message
        for r in caplog.records
    )


@pytest.mark.usefixtures("use_tmpdir")
@pytest.mark.parametrize("invalid_name", (":+:+:+:+", "        "))
@pytest.mark.parametrize(
    "named_keyword,keyword_type",
    [
        ("WOPR", "well"),
        ("GGLR", "group"),
        ("LWOPR", "local well"),
        ("LCOPR", "local completion"),
        ("COPT", "completion"),
        ("SOFR", "segment"),
    ],
)
def test_that_named_keywords_with_invalid_name_are_invalid(
    capsys, caplog, named_keyword, keyword_type, invalid_name
):
    caplog.set_level(logging.INFO)
    create_summary(summary_keys=("FOPR", named_keyword), names=[invalid_name] * 3)
    run(["summary.x", "-v", "TEST", "*"])
    capture = capsys.readouterr()

    assert keys_in_header(capture.out) == ["FOPR"]
    assert any(
        f"TEST.SMSPEC contains invalid keyword '{named_keyword}': "
        f"{keyword_type} keyword given invalid name '{invalid_name.strip()}'"
        in r.message
        for r in caplog.records
    )


@pytest.mark.usefixtures("use_tmpdir")
@pytest.mark.parametrize(
    "numbered_keyword,keyword_type",
    [
        ("COPT", "completion"),
        ("SOFR", "segment"),
        ("ROPV", "region"),
        ("RGFR", "inter region"),
        ("BOPR", "block"),
        ("AAQR", "aquifer"),
    ],
)
def test_that_numbered_keywords_with_negative_number_are_invalid(
    capsys, caplog, numbered_keyword, keyword_type
):
    caplog.set_level(logging.INFO)
    create_summary(summary_keys=("FOPR", numbered_keyword), numbers=[-1] * 3)
    run(["summary.x", "-v", "TEST", "*"])
    capture = capsys.readouterr()

    assert keys_in_header(capture.out) == ["FOPR"]
    print(caplog.records)
    assert any(
        f"TEST.SMSPEC contains invalid keyword '{numbered_keyword}': "
        f"{keyword_type} keyword given negative number -1" in r.message
        for r in caplog.records
    )


def create_split_case(
    case="TEST", summary_keys=("FOPR",), formatted="", times=(1, 2, 3, 4)
):
    smspec = Smspec(
        nx=2,
        ny=2,
        nz=2,
        restarted_from_step=0,
        num_keywords=1 + len(summary_keys),
        restart="        ",
        keywords=["TIME    ", *summary_keys],
        well_names=([":+:+:+:+", *(["A_NAME  "] * len(summary_keys))]),
        region_numbers=([-32676, *([0] * len(summary_keys))]),
        units=["DAYS    ", *(["SM3"] * len(summary_keys))],
        start_date=Date(day=1, month=1, year=2014, hour=0, minutes=0, micro_seconds=0),
        intehead=SmspecIntehead(
            unit=UnitSystem.METRIC,
            simulator=Simulator.ECLIPSE_100,
        ),
    )
    format = resfo.Format.FORMATTED if formatted == "F" else resfo.Format.UNFORMATTED
    smspec.to_file(f"{case}.{formatted}SMSPEC", file_format=format)
    for i, t in enumerate(times):
        smry = Unsmry(
            steps=[
                SummaryStep(
                    seqnum=i,
                    ministeps=[
                        SummaryMiniStep(
                            mini_step=0,
                            params=[float(t)] + [4.0] * len(summary_keys),
                        ),
                    ],
                )
            ]
        )
        smry.to_file(f"{case}.{'A' if formatted else 'S'}{i:04d}", file_format=format)


@pytest.mark.usefixtures("use_tmpdir")
@pytest.mark.parametrize("formatted", ("F", ""))
def test_that_case_name_can_refer_to_a_non_unified_summary(capsys, formatted):
    create_split_case(summary_keys=("FOPR", "FGIT"), formatted=formatted)

    capsys.readouterr()  # Ensure empty capture
    run(["summary.x", "-v", "TEST", "*"])
    df = output_as_df(capsys.readouterr().out)
    assert df.to_csv() == dedent("""\
            ,Days,dd/mm/yyyy,FGIT,FOPR
            0,1.0,02/01/2014,4.0,4.0
            1,2.0,03/01/2014,4.0,4.0
            2,3.0,04/01/2014,4.0,4.0
            3,4.0,05/01/2014,4.0,4.0
            """)


@pytest.mark.usefixtures("use_tmpdir")
def test_that_unformatted_unified_is_chosen_over_unformatted_split(capsys):
    create_summary(summary_keys=("FOPR",))
    create_split_case(summary_keys=("FOPR",))

    capsys.readouterr()  # Ensure empty capture
    with pytest.warns(match="More than one type of summary file"):
        run(["summary.x", "-v", "TEST", "*"])
    df = output_as_df(capsys.readouterr().out)
    assert df.to_csv() == dedent("""\
            ,Days,dd/mm/yyyy,FOPR
            0,0.0,01/01/2014,5.6299e+16
            """)


@pytest.mark.usefixtures("use_tmpdir")
def test_that_split_unformatted_is_chosen_over_unified_formatted(capsys):
    create_summary(summary_keys=("WOPR"), formatted="F")
    create_split_case(summary_keys=("FGIT", "FOPR"))

    capsys.readouterr()  # Ensure empty capture
    with pytest.warns(match="More than one type of summary file"):
        run(["summary.x", "-v", "TEST", "*"])
    df = output_as_df(capsys.readouterr().out)
    assert df.to_csv() == dedent("""\
            ,Days,dd/mm/yyyy,FGIT,FOPR
            0,1.0,02/01/2014,4.0,4.0
            1,2.0,03/01/2014,4.0,4.0
            2,3.0,04/01/2014,4.0,4.0
            3,4.0,05/01/2014,4.0,4.0
            """)


@pytest.mark.usefixtures("use_tmpdir")
def test_that_formatted_unified_is_chosen_over_unformatted_split(capsys):
    create_summary(summary_keys=("FOPR",), formatted="F")
    create_split_case(summary_keys=("FOPR",), formatted="F")

    capsys.readouterr()  # Ensure empty capture
    with pytest.warns(match="More than one type of summary file"):
        run(["summary.x", "-v", "TEST", "*"])
    df = output_as_df(capsys.readouterr().out)
    assert df.to_csv() == dedent("""\
            ,Days,dd/mm/yyyy,FOPR
            0,0.0,01/01/2014,5.6299e+16
            """)


@pytest.mark.usefixtures("use_tmpdir")
def test_that_files_can_be_in_a_different_directory(monkeypatch, capsys):
    dir = Path("subdir")
    dir.mkdir()
    monkeypatch.chdir(dir)
    create_summary(summary_keys=("FOPR",))
    monkeypatch.chdir("..")

    capsys.readouterr()  # Ensure empty capture
    run(["summary.x", "-v", str(dir / "TEST"), "*"])
    df = output_as_df(capsys.readouterr().out)
    assert df.to_csv() == dedent("""\
            ,Days,dd/mm/yyyy,FOPR
            0,0.0,01/01/2014,5.6299e+16
            """)


@pytest.mark.usefixtures("use_tmpdir")
def test_that_empty_extension_is_ignored(capsys):
    create_summary(summary_keys=("FOPR",))

    capsys.readouterr()  # Ensure empty capture
    run(["summary.x", "-v", "TEST.", "*"])
    df = output_as_df(capsys.readouterr().out)
    assert df.to_csv() == dedent("""\
            ,Days,dd/mm/yyyy,FOPR
            0,0.0,01/01/2014,5.6299e+16
            """)


@pytest.mark.usefixtures("use_tmpdir")
def test_that_relative_path_restart_is_relative_to_base_case(monkeypatch, capsys):
    dir = Path("subdir")
    dir.mkdir()
    monkeypatch.chdir(dir)
    create_summary(case="RESTART", summary_keys=("FOPT",))
    create_summary(restart="RESTART", summary_keys=("FOPR", "FGIP"))
    monkeypatch.chdir("..")

    capsys.readouterr()  # Ensure empty capture
    run(["summary.x", "-v", str(dir / "TEST"), "*"])
    assert keys_in_header(capsys.readouterr().out) == ["FGIP", "FOPR", "FOPT"]


@pytest.mark.usefixtures("use_tmpdir")
def test_that_restart_and_base_times_are_concated(capsys):
    create_split_case(
        case="RESTART", summary_keys=("FOPT",), times=[1.0, 2.0, 3.0, 4.0]
    )
    create_summary(
        restart="RESTART", summary_keys=("FOPR", "FGIP"), times=[2.0, 3.0, 4.0, 5.0]
    )

    capsys.readouterr()  # Ensure empty capture
    run(["summary.x", "-v", "TEST", "*"])
    assert output_as_df(capsys.readouterr().out).to_csv() == dedent("""\
        ,Days,dd/mm/yyyy,FGIP,FOPR,FOPT
        0,1.0,02/01/2014,-99.0,-99.0,4.0
        1,2.0,03/01/2014,5.6299e+16,5.6299e+16,-99.0
        2,3.0,04/01/2014,5.6299e+16,5.6299e+16,-99.0
        3,4.0,05/01/2014,5.6299e+16,5.6299e+16,-99.0
        4,5.0,06/01/2014,5.6299e+16,5.6299e+16,-99.0
        """)


@pytest.mark.usefixtures("use_tmpdir")
@pytest.mark.timeout(10)
def test_performance_with_many_keys(monkeypatch, benchmark):
    dir = Path("subdir")
    dir.mkdir()
    monkeypatch.chdir(dir)
    create_summary(case="TEST", summary_keys=[f"WWIT:N-{i}" for i in range(5000)])
    monkeypatch.chdir("..")

    def bench():
        run(["summary.x", "-v", str(dir / "TEST"), "*"])

    benchmark(bench)
