from view_summary.__main__ import parse_arguments, run
import resfo
import pytest
from hypothesis import given
from tests.summary_generator import (
    summaries,
    Unsmry,
    SummaryMiniStep,
    SummaryStep,
    Smspec,
    Date,
    SmspecIntehead,
    Simulator,
    UnitSystem,
)


def test_help_string(capsys):
    with pytest.raises(SystemExit) as sysexit:
        parse_arguments(["summary.x", "-h"])
    assert sysexit.value.code == 0
    captured = capsys.readouterr()
    assert (
        captured.out
        == """\
usage: summary.x [-h] [--list] [--restart | --no-restart]
                 [--header | --no-header] [--report-only]
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
  only look for unified files.
"""
    )


def test_that_giving_non_existing_case_is_invalid(tmp_path, capsys):
    try:
        parse_arguments(["summary.x", str(tmp_path / "DOES_NOT_EXIST")])
    except BaseException:
        # will produce exit code 2
        pass
    assert (
        f"No summary data found for case: {tmp_path / 'DOES_NOT_EXIST'}"
        in capsys.readouterr().err
    )


def create_summary(
    summary_keys=("FOPR",), time_units="DAYS", CASE="TEST", formatted=""
):
    num_values = len(summary_keys)
    unsmry = Unsmry(
        steps=[
            SummaryStep(
                seqnum=0,
                ministeps=[
                    SummaryMiniStep(
                        mini_step=0, params=[0.0] + [5.629901e16] * num_values
                    ),
                ],
            )
        ]
    )
    smspec = Smspec(
        nx=2,
        ny=2,
        nz=2,
        restarted_from_step=0,
        num_keywords=1 + len(summary_keys),
        restart="        ",
        keywords=["TIME    ", *summary_keys],
        well_names=[":+:+:+:+", "        " * len(summary_keys)],
        region_numbers=[-32676, 0 * len(summary_keys)],
        units=[time_units.ljust(8), "SM3" * len(summary_keys)],
        start_date=Date(day=1, month=1, year=2014, hour=0, minutes=0, micro_seconds=0),
        intehead=SmspecIntehead(
            unit=UnitSystem.METRIC,
            simulator=Simulator.ECLIPSE_100,
        ),
    )
    format = resfo.Format.FORMATTED if formatted == "F" else resfo.Format.UNFORMATTED
    smspec.to_file(f"{CASE}.{formatted}SMSPEC", file_format=format)
    unsmry.to_file(f"{CASE}.{formatted}UNSMRY", file_format=format)


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
        parse_arguments(["summary.x", str(tmp_path / f"TEST.FSMSPEC")])
    except BaseException:
        # will produce exit code 2
        pass
    assert (
        f"No summary data found for case: {tmp_path / 'TEST.FSMSPEC'}"
        in capsys.readouterr().err
    )


@pytest.mark.usefixtures("use_tmpdir")
def test_that_case_name_with_unformatted_extension_does_not_match_formatted_summaries(
    tmp_path, capsys
):
    try:
        create_summary(
            formatted="F"  # creates file named "TEST.FSMSPEC", not "TEST.SMSPEC"
        )
        parse_arguments(["summary.x", str(tmp_path / f"TEST.SMSPEC")])
    except BaseException:
        # will produce exit code 2
        pass
    assert (
        f"No summary data found for case: {tmp_path / 'TEST.SMSPEC'}"
        in capsys.readouterr().err
    )


@pytest.mark.usefixtures("use_tmpdir")
def test_that_ambiguous_references_to_summary_files_is_invalid(tmp_path, capsys):
    try:
        create_summary()
        create_summary(formatted="F")
        parse_arguments(["summary.x", str(tmp_path / f"TEST")])
    except BaseException:
        # will produce exit code 2
        pass
    assert f"could be any of: TEST.FUNSMRY, TEST.UNSMRY" in capsys.readouterr().err


@pytest.fixture
def run_cli(capsys):
    def inner(cli_args, *args, **kwargs):
        create_summary(*args, **kwargs)
        capsys.readouterr()
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
        "-- Days   dd/mm/yyyy               FGIP             FOPR\n"
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


@pytest.mark.usefixtures("use_tmpdir")
def test_that_for_non_wildcard_keywords_the_order_of_columns_is_as_in_the_input(capsys):
    create_summary(summary_keys=("FGIP", "FOPR", "FWPT", "FOPT"))

    run(["summary.x", "TEST", "FGIP", "FOPR"])
    assert keys_in_header(capsys.readouterr().out) == ["FGIP", "FOPR"]

    run(["summary.x", "TEST", "FOPR", "FGIP"])
    assert keys_in_header(capsys.readouterr().out) == ["FOPR", "FGIP"]


@pytest.mark.usefixtures("use_tmpdir")
def test_that_header_displays_the_time_units_from_spec(run_cli):
    run_cli(cli_args="FGIP", summary_keys=("FGIP",), time_units="HOURS").out.startswith(
        "-- Hours   dd/mm/yyyy"
    )
