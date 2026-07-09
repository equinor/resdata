import datetime

import pytest
from resdata import ResDataType
from resdata.gravimetry import ResdataGrav
from resdata.grid import GridGenerator
from resdata.resfile import FortIO, ResdataFile, ResdataKW, openFortIO

from tests import ResdataTest


def write_kws(filename, kws):
    with openFortIO(filename + ".UNRST", mode=FortIO.WRITE_MODE) as f:
        seq_hdr = ResdataKW("SEQNUM", 1, ResDataType.RD_INT)
        seq_hdr[0] = 10

        header = ResdataKW("INTEHEAD", 67, ResDataType.RD_INT)
        header[64] = 1
        header[65] = 1
        header[66] = 2000

        seq_hdr.fwrite(f)
        header.fwrite(f)
        for kw in kws:
            kw.fwrite(f)

        seq_hdr[0] = 20
        header[66] = 2009

        seq_hdr.fwrite(f)
        header.fwrite(f)
        for kw in kws:
            kw.fwrite(f)

        seq_hdr[0] = 20
        header[66] = 2010

        seq_hdr.fwrite(f)
        header.fwrite(f)
        for kw in kws:
            kw.fwrite(f)

        header = ResdataKW("INTEHEAD", 95, ResDataType.RD_INT)
        header[14] = 1  # sets phase to oil
        header[94] = 100  # E100
        with openFortIO(filename + ".INIT", mode=FortIO.WRITE_MODE) as f:
            header.fwrite(f)
            for kw in kws:
                kw.fwrite(f)


def write_grav_case(filename, kws, phases=1, version=100, init_kws=None):
    with openFortIO(filename + ".UNRST", mode=FortIO.WRITE_MODE) as f:
        seq_hdr = ResdataKW("SEQNUM", 1, ResDataType.RD_INT)
        header = ResdataKW("INTEHEAD", 67, ResDataType.RD_INT)
        header[64] = 1
        header[65] = 1
        for seqnum, year in [(10, 2000), (20, 2009), (20, 2010)]:
            seq_hdr[0] = seqnum
            header[66] = year
            seq_hdr.fwrite(f)
            header.fwrite(f)
            for kw in kws:
                kw.fwrite(f)

    header = ResdataKW("INTEHEAD", 95, ResDataType.RD_INT)
    header[14] = phases
    header[94] = version
    with openFortIO(filename + ".INIT", mode=FortIO.WRITE_MODE) as f:
        header.fwrite(f)
        for kw in kws:
            kw.fwrite(f)
        for kw in init_kws or []:
            kw.fwrite(f)


class ResdataGravTest(ResdataTest):
    def setUp(self):
        self.grid = GridGenerator.create_rectangular((10, 10, 10), (1, 1, 1))

    def _float_kw(self, name, value=0.5):
        kw = ResdataKW(name, self.grid.get_global_size(), ResDataType.RD_FLOAT)
        for i in range(self.grid.get_global_size()):
            kw[i] = value
        return kw

    def _int_kw(self, name, value=0):
        kw = ResdataKW(name, self.grid.get_global_size(), ResDataType.RD_INT)
        for i in range(self.grid.get_global_size()):
            kw[i] = value
        return kw

    def _all_phase_grav(self, tmpdir, mp, kws, version=100, init_kws=None):
        mp.chdir(tmpdir)
        write_grav_case("TEST", kws, phases=7, version=version, init_kws=init_kws)
        init = ResdataFile("TEST.INIT")
        grav = ResdataGrav(self.grid, init)
        restart_file = ResdataFile("TEST.UNRST")
        restart_view = restart_file.restart_view(sim_time=datetime.date(2000, 1, 1))
        for phase in (1, 2, 4):  # oil, gas, water
            grav.new_std_density(phase, 0.5)
            grav.add_std_density(phase, 0, 0.5)
        return grav, restart_view

    def test_all_phases_rporv_e100(self):
        kws = [
            self._float_kw(name)
            for name in [
                "PORO",
                "PORV",
                "RPORV",
                "SWAT",
                "SGAS",
                "OIL_DEN",
                "GAS_DEN",
                "WAT_DEN",
            ]
        ]
        tmpdir = self.tmp_path_factory.mktemp("grav_all_rporv", numbered=True)
        with self.monkeypatch.context() as mp:
            grav, restart_view = self._all_phase_grav(tmpdir, mp, kws)
            grav.add_survey_RPORV("rporv", restart_view)
            response = grav.eval("rporv", None, (0, 0, 0), phase_mask=7)
            assert response == pytest.approx(-0.015846654333235285, rel=1e-9)

    def test_all_phases_rfip_e100(self):
        kws = [
            self._float_kw(name)
            for name in [
                "PORO",
                "PORV",
                "OIL_DEN",
                "GAS_DEN",
                "WAT_DEN",
                "RFIPOIL",
                "RFIPGAS",
                "RFIPWAT",
            ]
        ]
        tmpdir = self.tmp_path_factory.mktemp("grav_all_rfip", numbered=True)
        with self.monkeypatch.context() as mp:
            grav, restart_view = self._all_phase_grav(tmpdir, mp, kws)
            grav.add_survey_RFIP("rfip", restart_view)
            response = grav.eval("rfip", None, (0, 0, 0), phase_mask=7)
            assert response == pytest.approx(-0.04753996299970585, rel=1e-9)

    def test_all_phases_fip_e100(self):
        kws = [
            self._float_kw(name)
            for name in ["PORO", "PORV", "FIPOIL", "FIPGAS", "FIPWAT"]
        ]
        tmpdir = self.tmp_path_factory.mktemp("grav_all_fip", numbered=True)
        with self.monkeypatch.context() as mp:
            grav, restart_view = self._all_phase_grav(
                tmpdir, mp, kws, init_kws=[self._int_kw("PVTNUM")]
            )
            grav.add_survey_FIP("fip", restart_view)
            response = grav.eval("fip", None, (0, 0, 0), phase_mask=7)
            assert response == pytest.approx(-0.04753996299970585, rel=1e-9)

    def test_e300_density_keywords(self):
        kws = [
            self._float_kw(name)
            for name in [
                "PORO",
                "PORV",
                "RPORV",
                "SWAT",
                "SGAS",
                "DENO",
                "DENG",
                "DENW",
            ]
        ]
        tmpdir = self.tmp_path_factory.mktemp("grav_e300", numbered=True)
        with self.monkeypatch.context() as mp:
            grav, restart_view = self._all_phase_grav(tmpdir, mp, kws, version=300)
            grav.add_survey_RPORV("rporv", restart_view)
            response = grav.eval("rporv", None, (0, 0, 0), phase_mask=7)
            assert response == pytest.approx(-0.015846654333235285, rel=1e-9)

    def test_unrecognized_simulator_id_raises(self):
        kws = [
            self._float_kw(name)
            for name in ["PORO", "PORV", "SWAT", "OIL_DEN", "RPORV"]
        ]
        tmpdir = self.tmp_path_factory.mktemp("grav_intersect", numbered=True)
        with self.monkeypatch.context() as mp:
            mp.chdir(tmpdir)
            write_grav_case("TEST", kws, phases=1, version=700)  # INTERSECT
            init = ResdataFile("TEST.INIT")
            grav = ResdataGrav(self.grid, init)
            restart_file = ResdataFile("TEST.UNRST")
            restart_view = restart_file.restart_view(sim_time=datetime.date(2000, 1, 1))
            grav.new_std_density(1, 0.5)
            grav.add_std_density(1, 0, 0.5)
            with pytest.raises(ValueError, match=r"unrecognized simulator id"):
                grav.add_survey_RPORV("rporv", restart_view)

    def test_create(self):
        kws = [
            ResdataKW(kw, self.grid.get_global_size(), ResDataType.RD_FLOAT)
            for kw in [
                "PORO",
                "PORV",
                "PRESSURE",
                "SWAT",
                "OIL_DEN",
                "RPORV",
                "PORV_MOD",
                "FIPOIL",
                "RFIPOIL",
            ]
        ]
        int_kws = [
            ResdataKW(kw, self.grid.get_global_size(), ResDataType.RD_INT)
            for kw in ["FIP_NUM", "PVTNUM"]
        ]

        for kw in kws:
            for i in range(self.grid.get_global_size()):
                kw[i] = 0.5
        for kw in int_kws:
            for i in range(self.grid.get_global_size()):
                kw[i] = 0

        kws += int_kws

        tmpdir = self.tmp_path_factory.mktemp("grav_init", numbered=True)
        with self.monkeypatch.context() as mp:
            mp.chdir(tmpdir)
            write_kws("TEST", kws)
            # The init file created here only contains a PORO field. More
            # properties must be added to this before it can be used for
            # any useful gravity calculations.
            init = ResdataFile("TEST.INIT")

            grav = ResdataGrav(self.grid, init)

            restart_file = ResdataFile("TEST.UNRST")
            restart_view = restart_file.restart_view(sim_time=datetime.date(2000, 1, 1))

            grav.new_std_density(1, 0.5)
            grav.add_std_density(1, 0, 0.5)

            grav.add_survey_RPORV("rporv", restart_view)
            grav.add_survey_PORMOD("pormod", restart_view)
            grav.add_survey_FIP("fip", restart_view)
            grav.add_survey_RFIP("fip", restart_view)

            grav.eval("rporv", "pormod", (0, 0, 0), phase_mask=1)

            # Test that missing std_density raises
            grav = ResdataGrav(self.grid, init)
            with self.assertRaises(ValueError):
                grav.add_survey_FIP("fip", restart_view)

    def test_create_minimal(self):
        kws = [
            ResdataKW(kw, self.grid.get_global_size(), ResDataType.RD_FLOAT)
            for kw in [
                "PORO",
                "PORV",
                "SWAT",
                "OIL_DEN",
                "RPORV",
            ]
        ]

        tmpdir = self.tmp_path_factory.mktemp("grav_init", numbered=True)
        with self.monkeypatch.context() as mp:
            mp.chdir(tmpdir)
            write_kws("TEST", kws)
            init = ResdataFile("TEST.INIT")

            grav = ResdataGrav(self.grid, init)

            restart_file = ResdataFile("TEST.UNRST")
            restart_view = restart_file.restart_view(sim_time=datetime.date(2000, 1, 1))

            grav.new_std_density(1, 0.5)
            grav.add_std_density(1, 0, 0.5)

            grav.add_survey_RPORV("rporv", restart_view)

    def test_missing_rporv_keyword_raises(self):
        kws = [
            ResdataKW(kw, self.grid.get_global_size(), ResDataType.RD_FLOAT)
            for kw in [
                "PORO",
                "PORV",
                "SWAT",
                "OIL_DEN",
            ]
        ]

        tmpdir = self.tmp_path_factory.mktemp("grav_missing_rporv", numbered=True)
        with self.monkeypatch.context() as mp:
            mp.chdir(tmpdir)
            write_kws("TEST", kws)
            init = ResdataFile("TEST.INIT")

            grav = ResdataGrav(self.grid, init)

            restart_file = ResdataFile("TEST.UNRST")
            restart_view = restart_file.restart_view(sim_time=datetime.date(2000, 1, 1))

            grav.new_std_density(1, 0.5)
            grav.add_std_density(1, 0, 0.5)

            with pytest.raises(RuntimeError, match=r"restart file did not contain"):
                grav.add_survey_RPORV("rporv", restart_view)

    def test_invalid_phase_id_raises_python_exception(self):
        kws = [
            ResdataKW(kw, self.grid.get_global_size(), ResDataType.RD_FLOAT)
            for kw in [
                "PORO",
                "PORV",
                "SWAT",
                "OIL_DEN",
                "RPORV",
            ]
        ]

        tmpdir = self.tmp_path_factory.mktemp("grav_invalid_phase", numbered=True)
        with self.monkeypatch.context() as mp:
            mp.chdir(tmpdir)
            write_kws("TEST", kws)
            init = ResdataFile("TEST.INIT")
            grav = ResdataGrav(self.grid, init)

            with pytest.raises(
                ValueError, match=r"phase enum value: 999 not recognized"
            ):
                grav.new_std_density(999, 0.5)

    def test_eval_unknown_survey_raises(self):
        kws = [
            ResdataKW(kw, self.grid.get_global_size(), ResDataType.RD_FLOAT)
            for kw in [
                "PORO",
                "PORV",
                "SWAT",
                "OIL_DEN",
                "RPORV",
            ]
        ]

        tmpdir = self.tmp_path_factory.mktemp("grav_unknown_survey", numbered=True)
        with self.monkeypatch.context() as mp:
            mp.chdir(tmpdir)
            write_kws("TEST", kws)
            init = ResdataFile("TEST.INIT")

            grav = ResdataGrav(self.grid, init)

            restart_file = ResdataFile("TEST.UNRST")
            restart_view = restart_file.restart_view(sim_time=datetime.date(2000, 1, 1))

            grav.new_std_density(1, 0.5)
            grav.add_std_density(1, 0, 0.5)
            grav.add_survey_RPORV("rporv", restart_view)

            with pytest.raises(
                ValueError,
                match=r"Survey name: does_not_exist not registered",
            ):
                grav.eval("does_not_exist", None, (0, 0, 0), phase_mask=1)

    def test_that_rporv_inconsistency_raises(self):
        size = self.grid.get_global_size()
        porv = ResdataKW("PORV", size, ResDataType.RD_FLOAT)
        rporv = ResdataKW("RPORV", size, ResDataType.RD_FLOAT)

        for i in range(size):
            porv[i] = 1.0
            rporv[i] = 1000.0

        kws = [porv, rporv]

        tmpdir = self.tmp_path_factory.mktemp("grav_bad_rporv", numbered=True)
        with self.monkeypatch.context() as mp:
            mp.chdir(tmpdir)
            write_kws("TEST", kws)
            init = ResdataFile("TEST.INIT")

            grav = ResdataGrav(self.grid, init)

            restart_file = ResdataFile("TEST.UNRST")
            restart_view = restart_file.restart_view(sim_time=datetime.date(2000, 1, 1))

            grav.new_std_density(1, 0.5)
            grav.add_std_density(1, 0, 0.5)

            with pytest.raises(
                RuntimeError,
                match=r"substantially different from the initial porv value",
            ):
                grav.add_survey_RPORV("rporv", restart_view)
