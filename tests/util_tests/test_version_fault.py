import re

from resdata.util.util import ResdataVersion

EXPECTED_MAJOR = 7
EXPECTED_MINOR = 0
EXPECTED_MICRO = "0b1.dev4+g5931a7fbe"
EXPECTED_VERSION = "7.0.0b1.dev4+g5931a7fbe"
EXPECTED_COMMIT = "5931a7fbe17df09d03934930cb5834933f773369"
EXPECTED_BUILD_TIME = "Unknown build time"


def test_version_major_is_int_and_expected_value():
    version = ResdataVersion()

    assert type(version.major) is int
    assert version.major == EXPECTED_MAJOR


def test_version_minor_is_int_and_expected_value():
    version = ResdataVersion()

    assert type(version.minor) is int
    assert version.minor == EXPECTED_MINOR


def test_version_micro_is_string_and_expected_value():
    version = ResdataVersion()

    assert type(version.micro) is str
    assert version.micro == EXPECTED_MICRO


def test_version_tuple_has_expected_types_length_and_contents():
    version = ResdataVersion()

    version_tuple = version.versionTuple()
    assert type(version_tuple) is tuple
    assert len(version_tuple) == 3
    assert tuple(type(part) for part in version_tuple) == (int, int, str)
    assert version_tuple == (EXPECTED_MAJOR, EXPECTED_MINOR, EXPECTED_MICRO)


def test_version_string_matches_expected_format_and_value():
    version = ResdataVersion()

    version_string = version.versionString()
    assert type(version_string) is str
    assert version_string == EXPECTED_VERSION
    assert re.fullmatch(r"\d+\.\d+\.0b1\.dev4\+g[0-9a-f]{9}", version_string)


def test_str_uses_the_same_public_version_string():
    version = ResdataVersion()

    assert str(version) == version.versionString()
    assert str(version) == EXPECTED_VERSION


def test_development_status_is_bool_true_for_non_numeric_micro():
    version = ResdataVersion()

    is_devel = version.isDevelVersion()
    assert type(is_devel) is bool
    assert is_devel is True
    assert version.micro_int == -1


def test_build_time_is_expected_string():
    version = ResdataVersion()

    build_time = version.getBuildTime()
    assert type(build_time) is str
    assert build_time == EXPECTED_BUILD_TIME
    assert version.build_time == EXPECTED_BUILD_TIME


def test_git_commit_is_full_expected_hash():
    version = ResdataVersion()

    git_commit = version.getGitCommit()
    assert type(git_commit) is str
    assert git_commit == EXPECTED_COMMIT
    assert len(git_commit) == 40
    assert re.fullmatch(r"[0-9a-f]{40}", git_commit)


def test_short_git_commit_and_repr_use_eight_character_hash():
    version = ResdataVersion()

    short_commit = version.getGitCommit(short=True)
    assert type(short_commit) is str
    assert short_commit == EXPECTED_COMMIT[:8]
    assert repr(version) == (
        'Version(major=7, minor=0, micro="0b1.dev4+g5931a7fbe", '
        'commit="5931a7fb", status="development")'
    )
