"""Calculate subsidence change.

The rd_subsidence module contains functionality to load time-lapse ECLIPSE
results and calculate the change in seafloor subsidence between the
different surveys.
"""

from cwrap import BaseCClass

from resdata import ResdataPrototype
from resdata.grid import Grid, ResdataRegion
from resdata.resfile import ResdataFile, ResdataFileView
from resdata.util.util import monkey_the_camel


class ResdataSubsidence(BaseCClass):
    """ECLIPSE results for calculating subsidence changes.

    The ResdataSubsidence class is a collection class holding the results from
    ECLIPSE forward modelling of subsidence surveys. Observe that the
    class is focused on the ECLIPSE side of things, and does not have
    any notion of observed values or measurement locations; that
    should be handled by the scope using the ResdataSubsidence class.

    Typical use of the ResdataSubsidence class involves the following steps:

      1. Create the ResdataSubsidence instance.
      2. Add surveys with the add_survey_XXXX() methods.
      3. Evaluate the subsidence response with the eval() method.
    """

    TYPE_NAME = "rd_subsidence"
    _alloc = ResdataPrototype(
        "void* rd_subsidence_alloc( rd_grid , rd_file )", bind=False
    )
    _free = ResdataPrototype("void rd_subsidence_free( rd_subsidence )")
    _add_survey_PRESSURE = ResdataPrototype(
        "void*  rd_subsidence_add_survey_PRESSURE( rd_subsidence , char* , rd_file_view )"
    )
    _eval = ResdataPrototype(
        "double rd_subsidence_eval( rd_subsidence , char* , char* , rd_region , double , double , double, double, double)"
    )
    _eval_geertsma = ResdataPrototype(
        "double rd_subsidence_eval_geertsma( rd_subsidence , char* , char* , rd_region , double , double , double, double, double, double)"
    )
    _eval_geertsma_rporv = ResdataPrototype(
        "double rd_subsidence_eval_geertsma_rporv( rd_subsidence , char* , char* , rd_region , double , double , double, double, double, double)"
    )
    _has_survey = ResdataPrototype(
        "bool  rd_subsidence_has_survey( rd_subsidence , char*)"
    )

    def __init__(self, grid: Grid, init_file: ResdataFile):
        self.init_file = init_file  # Inhibit premature garbage collection of init_file
        c_ptr = self._alloc(grid, init_file)
        super().__init__(c_ptr)

    def __contains__(self, survey_name):
        return self._has_survey(survey_name)

    def add_survey_PRESSURE(
        self, survey_name: str, restart_file: ResdataFileView
    ) -> None:
        """Add new survey based on PRESSURE keyword.

        Add a new survey; in this context a survey is the state of
        reservoir, i.e. a restart file. The @survey_name
        input argument will be used when referring to this survey at a
        later stage. The @restart_file input argument should be an
        ResdataFileView instance with data from one report step. A typical way
        to load the @restart_file argument is:

           >>> date = datetime.datetime(year, month, day)
           >>> restart = ResdataFile("CASE.UNRST")
           >>> init = ResdataFile("CASE.INIT")
           >>> grid = Grid("CASE.EGRID")
           >>>
           >>> subsidence = ResdataSubsidence(grid, init)
           >>> subsidence.add_survey_PRESSURE("Survey1", restart.restart_view(sim_time=date))
           >>> subsidence.add_survey_PRESSURE("Survey2", restart.restart_view(report_step=67))

        The pore volume is calculated from the initial pore volume and
        the PRESSURE keyword from the restart file.
        """
        self._add_survey_PRESSURE(survey_name, restart_file)

    def eval_geertsma(
        self,
        base_survey,
        monitor_survey,
        pos,
        youngs_modulus,
        poisson_ratio,
        seabed,
        region=None,
    ):
        if base_survey not in self:
            raise KeyError("No such survey: %s" % base_survey)

        if monitor_survey is not None:
            if monitor_survey not in self:
                raise KeyError("No such survey: %s" % monitor_survey)

        return self._eval_geertsma(
            base_survey,
            monitor_survey,
            region,
            pos[0],
            pos[1],
            pos[2],
            youngs_modulus,
            poisson_ratio,
            seabed,
        )

    def eval_geertsma_rporv(
        self,
        base_survey,
        monitor_survey,
        pos,
        youngs_modulus,
        poisson_ratio,
        seabed,
        region=None,
    ):
        if base_survey not in self:
            raise KeyError("No such survey: %s" % base_survey)

        if monitor_survey is not None:
            if monitor_survey not in self:
                raise KeyError("No such survey: %s" % monitor_survey)

        return self._eval_geertsma_rporv(
            base_survey,
            monitor_survey,
            region,
            pos[0],
            pos[1],
            pos[2],
            youngs_modulus,
            poisson_ratio,
            seabed,
        )

    def eval(
        self,
        base_survey: str,
        monitor_survey: str | None,
        pos: tuple[float, float, float],
        compressibility: float,
        poisson_ratio: float,
        region: ResdataRegion | None = None,
    ) -> float:
        """Calculate the subsidence change between two surveys.

        This is the method everything is leading up to; will calculate
        the change in subsidence, in centimeters,
        between the two surveys named @base_survey and
        @monitor_survey.

        The monitor survey can be 'None' - the resulting answer has
        nothing whatsoever to do with subsidence, but can be
        interesting to determine the numerical size of the quantities
        which are subtracted in a 4D study.

        The @pos argument should be a tuple of three elements with the
        (utm_x , utm_y , depth) position where we want to evaluate the
        change in subsidence.

        If supplied the optional argument @region should be an
        ResdataRegion() instance; this region will be used to limit the
        part of the reservoir included in the subsidence calculations.

        The argument @compressibility is the total reservoir compressibility.
        """
        if base_survey not in self:
            raise KeyError("No such survey: %s" % base_survey)

        if monitor_survey is not None and monitor_survey not in self:
            raise KeyError("No such survey: %s" % monitor_survey)

        return self._eval(
            base_survey,
            monitor_survey,
            region,
            pos[0],
            pos[1],
            pos[2],
            compressibility,
            poisson_ratio,
        )

    def free(self):
        self._free()


monkey_the_camel(ResdataSubsidence, "evalGeertsma", ResdataSubsidence.eval_geertsma)
