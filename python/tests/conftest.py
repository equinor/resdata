from hypothesis import HealthCheck, settings
import pytest

# Timeout settings are unreliable both on CI and
# when running pytest with xdist so we disable it
settings.register_profile(
    "no_timeouts",
    deadline=None,
    suppress_health_check=[HealthCheck.too_slow],
    print_blob=True,
)
settings.load_profile("no_timeouts")


@pytest.fixture
def use_tmpdir(tmpdir):
    with tmpdir.as_cwd():
        yield
