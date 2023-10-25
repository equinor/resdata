"""
Calculate dynamic change in gravitational strength.

The rd_grav module contains functionality to load time-lapse ECLIPSE
results and calculate the change in gravitational strength between the
different surveys. The implementation is a thin wrapper around the
rd_grav.c implementation in the resdata library.
"""

from typing import Optional

from cwrap import BaseCClass
from resdata import Phase, ResdataPrototype
from resdata.grid import Grid, ResdataRegion
from resdata.resfile import ResdataFile, ResdataFileView
from resdata.util.util import monkey_the_camel


class ResdataGrav(BaseCClass):
    """
    Holding ECLIPSE results for calculating gravity changes.

    The ResdataGrav class is a collection class holding the results from
    ECLIPSE forward modelling of gravity surveys. Observe that the
    class is focused on the ECLIPSE side of things, and does not have
    any notion of observed values or measurement locations; that
    should be handled by the scope using the ResdataGrav class.

    Typical use of the ResdataGrav class involves the following steps:

      1. Create the ResdataGrav instance.
      2. Add surveys with the add_survey_XXXX() methods.
      3. Evalute the gravitational response with the eval() method.
    """

    TYPE_NAME = "rd_grav"
    _grav_alloc = ResdataPrototype("void* rd_grav_alloc(rd_grid, rd_file)", bind=False)
    _free = ResdataPrototype("void rd_grav_free(rd_grav)")
    _add_survey_RPORV = ResdataPrototype(
        "void*  rd_grav_add_survey_RPORV(rd_grav, char*, rd_file_view)"
    )
    _add_survey_PORMOD = ResdataPrototype(
        "void*  rd_grav_add_survey_PORMOD(rd_grav, char*, rd_file_view)"
    )
    _add_survey_FIP = ResdataPrototype(
        "void*  rd_grav_add_survey_FIP(rd_grav, char*, rd_file_view)"
    )
    _add_survey_RFIP = ResdataPrototype(
        "void*  rd_grav_add_survey_RFIP(rd_grav, char*, rd_file_view)"
    )
    _new_std_density = ResdataPrototype(
        "void rd_grav_new_std_density(rd_grav, int, double)"
    )
    _add_std_density = ResdataPrototype(
        "void rd_grav_add_std_density(rd_grav, int, int, double)"
    )
    _eval = ResdataPrototype(
        "double rd_grav_eval(rd_grav, char*, char*, rd_region, double, double, double, int)"
    )

    def __init__(self, grid: Grid, init_file: ResdataFile):
        """
        Creates a new ResdataGrav instance.

        The input arguments @grid and @init_file should be instances
        of Grid and ResdataFile respectively.
        """
        self.init_file = init_file  # Inhibit premature garbage collection of init_file

        c_ptr = self._grav_alloc(grid, init_file)
        super(ResdataGrav, self).__init__(c_ptr)

        self.dispatch = {
            "FIP": self.add_survey_FIP,
            "RFIP": self.add_survey_RFIP,
            "PORMOD": self.add_survey_PORMOD,
            "RPORV": self.add_survey_RPORV,
        }

    def add_survey_RPORV(self, survey_name: str, restart_view: ResdataFileView):
        """
        Add new survey based on RPORV keyword.

        Add a new survey; in this context a survey is the state of
        reservoir, i.e. a restart file. The @survey_name
        input argument will be used when refering to this survey at a
        later stage. The @restart_view input argument should be an
        ResdataFile instance with data from one report step. A typical way
        to load the @restart_view argument is:

           import datetime
           from resdata.resfil import ResdataRestartFile
           ...
           ...
           date = datetime.datetime(year, month, day)
           rst_file = ResdataFile("CASE.UNRST")
           restart_view1 = rst_file.restartView(sim_time=date)
           restart_view2 = rst_file.restartView(report_step=67)

        The pore volume of each cell will be calculated based on the
        RPORV keyword from the restart files. The methods
        add_survey_PORMOD() and add_survey_FIP() are alternatives
        which are based on other keywords.
        """
        self._add_survey_RPORV(survey_name, restart_view)

    def add_survey_PORMOD(self, survey_name: str, restart_view: ResdataFileView):
        """
        Add new survey based on PORMOD keyword.

        The pore volum is calculated from the initial pore volume and
        the PORV_MOD keyword from the restart file; see
        add_survey_RPORV() for further details.
        """
        self._add_survey_PORMOD(survey_name, restart_view)

    def add_survey_FIP(self, survey_name: str, restart_view: ResdataFileView):
        """
        Add new survey based on FIP keywords.

        This method adds a survey as add_survey_RPORV() and
        add_survey_PORMOD; but the mass content in each cell is
        calculated based on the FIPxxx keyword along with the mass
        density at standard conditions of the respective phases.

        The mass density at standard conditions must be specified with
        the new_std_density() (and possibly also add_std_density())
        method before calling the add_survey_FIP() method.
        """
        self._add_survey_FIP(survey_name, restart_view)

    def add_survey_RFIP(self, survey_name: str, restart_view: ResdataFileView):
        """
        Add new survey based on RFIP keywords.

        This method adds a survey as add_survey_RPORV() and
        add_survey_PORMOD; but the mass content in each cell is
        calculated based on the RFIPxxx keyword along with the
        per-cell mass density of the respective phases.
        """
        self._add_survey_RFIP(survey_name, restart_view)

    def add_survey(self, name: str, restart_view: ResdataFileView, method):
        method = self.dispatch[method]
        return method(name, restart_view)

    def eval(
        self,
        base_survey,
        monitor_survey,
        pos,
        region: Optional[ResdataRegion] = None,
        phase_mask=Phase.OIL + Phase.GAS + Phase.WATER,
    ):
        """
        Calculates the gravity change between two surveys.

        This is the method everything is leading up to; will calculate
        the change in gravitational strength, in units of micro Gal,
        between the two surveys named @base_survey and
        @monitor_survey.

        The monitor survey can be 'None' - the resulting answer has
        nothing whatsovever to do with gravitation, but can be
        interesting to determine the numerical size of the quantities
        which are subtracted in a 4D study.

        The @pos argument should be a tuple of three elements with the
        (utm_x, utm_y, depth) position where we want to evaluate the
        change in gravitational strength.

        If supplied the optional argument @region should be an
        ResdataRegion() instance; this region will be used to limit the
        part of the reserviour included in the gravity calculations.

        The optional argument @phase_mask is an integer flag to
        indicate which phases you are interested in. It should be a
        sum of the relevant integer constants 'OIL',
        'GAS' and 'WATER'.
        """
        return self._eval(
            base_survey, monitor_survey, region, pos[0], pos[1], pos[2], phase_mask
        )

    def new_std_density(self, phase_enum, default_density):
        """
        Adds a new phase with a corresponding density.

        @phase_enum is one of the integer constants OIL,
        GAS or WATER, all available in the
        rd_util and also ecl modules.

        @default_density is the density, at standard conditions, for
        this particular phase. By default @default_density will be
        used for all the cells in the model; by using the
        add_std_density() method you can specify different densities
        for different PVT regions.

        The new_std_density() and add_std_density() methods must be
        used before you use the add_survey_FIP() method to add a
        survey based on the FIP keyword.
        """
        self._new_std_density(phase_enum, default_density)

    def add_std_density(self, phase_enum, pvtnum, density):
        """
        Add standard conditions density for PVT region @pvtnum.

        The new_std_density() method will add a standard conditions
        density which applies to all cells in the model. Using the
        add_std_density() method it is possible to add standard
        conditions densities on a per PVT region basis. You can add
        densities for as many PVT regions as you like, and then fall
        back to the default density for the others.

        The new_std_density() method must be called before calling the
        add_std_density() method.

        The new_std_density() and add_std_density() methods must be
        used before you use the add_survey_FIP() method to add a
        survey based on the FIP keyword.
        """
        self._add_std_density(phase_enum, pvtnum, density)

    def free(self):
        self._free()


monkey_the_camel(ResdataGrav, "addSurvey", ResdataGrav.add_survey)
