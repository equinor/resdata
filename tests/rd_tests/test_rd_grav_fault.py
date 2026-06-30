import datetime
import shutil
from pathlib import Path

import pytest
from resdata import Phase, ResDataType
from resdata.gravimetry import ResdataGrav
from resdata.grid import GridGenerator
from resdata.resfile import FortIO, ResdataFile, ResdataKW, openFortIO


def fillKeyword(name, dataType, values):
    keyword = ResdataKW(name, len(values), dataType)
    for index, value in enumerate(values):
        keyword[index] = value
    return keyword


def writeCase(casePrefix):
    initHeader = ResdataKW("INTEHEAD", 95, ResDataType.RD_INT)
    initHeader[14] = 1
    initHeader[94] = 100

    with openFortIO(casePrefix + ".INIT", mode=FortIO.WRITE_MODE) as fileH:
        initHeader.fwrite(fileH)
        fillKeyword("PORO", ResDataType.RD_FLOAT, [0.25, 0.25]).fwrite(fileH)
        fillKeyword("PORV", ResDataType.RD_FLOAT, [250000.0, 250000.0]).fwrite(fileH)
        fillKeyword("PVTNUM", ResDataType.RD_INT, [0, 1]).fwrite(fileH)

    seqHeader = ResdataKW("SEQNUM", 1, ResDataType.RD_INT)
    intHeader = ResdataKW("INTEHEAD", 67, ResDataType.RD_INT)
    intHeader[64] = 1
    intHeader[65] = 1
    surveys = [
        (
            10,
            2000,
            [250000.0, 240000.0],
            [1.0, 0.96],
            [100.0, 120.0],
            [90.0, 110.0],
            [800.0, 810.0],
        ),
        (
            20,
            2010,
            [230000.0, 220000.0],
            [0.92, 0.88],
            [130.0, 160.0],
            [120.0, 150.0],
            [820.0, 830.0],
        ),
    ]

    with openFortIO(casePrefix + ".UNRST", mode=FortIO.WRITE_MODE) as fileH:
        for seqNum, year, rporv, pormod, fip, rfip, density in surveys:
            seqHeader[0] = seqNum
            intHeader[66] = year
            seqHeader.fwrite(fileH)
            intHeader.fwrite(fileH)
            fillKeyword("RPORV", ResDataType.RD_FLOAT, rporv).fwrite(fileH)
            fillKeyword("PORV_MOD", ResDataType.RD_FLOAT, pormod).fwrite(fileH)
            fillKeyword("FIPOIL", ResDataType.RD_FLOAT, fip).fwrite(fileH)
            fillKeyword("RFIPOIL", ResDataType.RD_FLOAT, rfip).fwrite(fileH)
            fillKeyword("SWAT", ResDataType.RD_FLOAT, [0.2, 0.3]).fwrite(fileH)
            fillKeyword("OIL_DEN", ResDataType.RD_FLOAT, density).fwrite(fileH)


@pytest.fixture
def gravCase(request):
    workspace = (
        Path.cwd()
        / ".rd_grav_fault_workspace"
        / f"{request.node.name}_{id(request.node)}"
    )
    shutil.rmtree(workspace, ignore_errors=True)
    workspace.mkdir(parents=True)
    casePrefix = str(workspace / "CASE")
    writeCase(casePrefix)

    grid = GridGenerator.create_rectangular((2, 1, 1), (100, 100, 100))
    initFile = ResdataFile(casePrefix + ".INIT")
    restartFile = ResdataFile(casePrefix + ".UNRST")
    view2000 = restartFile.restart_view(sim_time=datetime.date(2000, 1, 1))
    view2010 = restartFile.restart_view(sim_time=datetime.date(2010, 1, 1))

    yield grid, initFile, restartFile, view2000, view2010

    shutil.rmtree(workspace, ignore_errors=True)


def addRporvSurveys(gravCase):
    grid, initFile, restartFile, view2000, view2010 = gravCase
    grav = ResdataGrav(grid, initFile)
    grav.add_survey_RPORV("base", view2000)
    grav.add_survey_RPORV("monitor", view2010)
    return grav


def test_default_phase_mask_returns_float(gravCase):
    grav = addRporvSurveys(gravCase)

    response = grav.eval("base", "monitor", (1000.0, 1000.0, 0.0))

    assert type(response) is float


def test_default_phase_mask_rporv_value(gravCase):
    grav = addRporvSurveys(gravCase)

    response = grav.eval("base", "monitor", (1000.0, 1000.0, 0.0))

    assert response == pytest.approx(-0.0025800427500607746)


def test_same_survey_with_default_phase_mask_is_zero(gravCase):
    grav = addRporvSurveys(gravCase)

    response = grav.eval("base", "base", (1000.0, 1000.0, 0.0))

    assert response == pytest.approx(0.0, abs=1e-14)


def test_monitor_none_with_default_phase_mask_returns_value(gravCase):
    grav = addRporvSurveys(gravCase)

    response = grav.eval("base", None, (1000.0, 1000.0, 0.0))

    assert response == pytest.approx(-0.043844308563498906)


def test_explicit_phase_enum_mask_matches_expected_value(gravCase):
    grav = addRporvSurveys(gravCase)

    response = grav.eval("base", "monitor", (1000.0, 1000.0, 0.0), phase_mask=Phase.OIL)

    assert response == pytest.approx(-0.0025800427500607746)


def test_summed_phase_enum_mask_matches_oil_only_case(gravCase):
    grav = addRporvSurveys(gravCase)

    oilOnly = grav.eval("base", "monitor", (1000.0, 1000.0, 0.0), phase_mask=Phase.OIL)
    allPhases = grav.eval(
        "base",
        "monitor",
        (1000.0, 1000.0, 0.0),
        phase_mask=Phase.OIL + Phase.GAS + Phase.WATER,
    )

    assert allPhases == pytest.approx(oilOnly)


def test_positional_phase_enum_mask_is_accepted(gravCase):
    grav = addRporvSurveys(gravCase)

    response = grav.eval("base", "monitor", (1000.0, 1000.0, 0.0), None, Phase.OIL)

    assert response == pytest.approx(-0.0025800427500607746)


def test_pormod_survey_accepts_phase_enum_mask(gravCase):
    grid, initFile, restartFile, view2000, view2010 = gravCase
    grav = ResdataGrav(grid, initFile)
    grav.add_survey_PORMOD("base", view2000)
    grav.add_survey_PORMOD("monitor", view2010)

    response = grav.eval("base", "monitor", (1000.0, 1000.0, 0.0), phase_mask=Phase.OIL)

    assert response == pytest.approx(-0.002580041996635322)


def test_rfip_survey_accepts_phase_enum_mask(gravCase):
    grid, initFile, restartFile, view2000, view2010 = gravCase
    grav = ResdataGrav(grid, initFile)
    grav.add_survey_RFIP("base", view2000)
    grav.add_survey_RFIP("monitor", view2010)

    response = grav.eval("base", "monitor", (1000.0, 1000.0, 0.0), phase_mask=Phase.OIL)

    assert response == pytest.approx(9.315230851413306e-06)


def test_fip_density_methods_accept_phase_enum_values(gravCase):
    grid, initFile, restartFile, view2000, view2010 = gravCase
    grav = ResdataGrav(grid, initFile)
    grav.new_std_density(Phase.OIL, 700.0)
    grav.add_std_density(Phase.OIL, 0, 700.0)
    grav.add_std_density(Phase.OIL, 1, 900.0)
    grav.add_survey_FIP("base", view2000)
    grav.add_survey_FIP("monitor", view2010)

    response = grav.eval("base", "monitor", (1000.0, 1000.0, 0.0), phase_mask=Phase.OIL)

    assert response == pytest.approx(8.670100392906717e-06)
