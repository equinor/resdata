from cwrap import BaseCClass
from resdata import FileMode, FileType, ResdataPrototype
from resdata.resfile import Resdata3DFile, ResdataFile
from resdata.util.util import CTime, monkey_the_camel


class ResdataRestartHead(BaseCClass):
    TYPE_NAME = "rd_rsthead"
    _alloc = ResdataPrototype(
        "void*  rd_rsthead_alloc(rd_file_view , int )", bind=False
    )
    _alloc_from_kw = ResdataPrototype(
        "void*  rd_rsthead_alloc_from_kw(int , rd_kw , rd_kw , rd_kw )", bind=False
    )
    _free = ResdataPrototype("void   rd_rsthead_free(rd_rsthead)")
    _get_report_step = ResdataPrototype("int    rd_rsthead_get_report_step(rd_rsthead)")
    _get_sim_time = ResdataPrototype("rd_time_t rd_rsthead_get_sim_time(rd_rsthead)")
    _get_sim_days = ResdataPrototype("double rd_rsthead_get_sim_days(rd_rsthead)")
    _get_nxconz = ResdataPrototype("int   rd_rsthead_get_nxconz(rd_rsthead)")
    _get_ncwmax = ResdataPrototype("int   rd_rsthead_get_ncwmax(rd_rsthead)")

    def __init__(self, kw_arg=None, rst_view=None):
        if kw_arg is None and rst_view is None:
            raise ValueError(
                "Cannot construct ResdataRestartHead without one of kw_arg and rst_view, both were None!"
            )

        if not kw_arg is None:
            report_step, intehead_kw, doubhead_kw, logihead_kw = kw_arg
            c_ptr = self._alloc_from_kw(
                report_step, intehead_kw, doubhead_kw, logihead_kw
            )
        else:
            c_ptr = self._alloc(rst_view, -1)

        super(ResdataRestartHead, self).__init__(c_ptr)

    def free(self):
        self._free()

    def get_report_step(self):
        return self._get_report_step()

    def get_sim_date(self):
        ct = CTime(self._get_sim_time())
        return ct.datetime()

    def get_sim_days(self):
        return self._get_sim_days()

    def well_details(self):
        return {"NXCONZ": self._get_nxconz(), "NCWMAX": self._get_ncwmax()}


class ResdataRestartFile(Resdata3DFile):
    def __init__(self, grid, filename, flags=FileMode.DEFAULT):
        """Will open an Eclipse restart file.

        The ResdataRestartFile class will open an eclipse restart file, in
        unified or non unified format. The constructor will infer the
        file type based on the filename, and will raise a ValueError
        exception if the file type is not RESTART or
        UNIFIED_RESTART.

        The ResdataRestartFile will use a grid reference to create Resdata3DKw
        instances for all the keyword elements which have either
        'nactive' or 'nx*ny*nz' elements.
        """

        file_type, report_step, fmt_file = ResdataFile.getFileType(filename)
        if not file_type in [
            FileType.RESTART,
            FileType.UNIFIED_RESTART,
        ]:
            raise ValueError(
                'The input filename "%s" does not correspond to a restart file.  Please follow the Eclipse naming conventions'
                % filename
            )

        super(ResdataRestartFile, self).__init__(grid, filename, flags)
        self.rst_headers = None
        if file_type == FileType.RESTART:
            self.is_unified = False
            self.report_step = report_step
        else:
            self.is_unified = True

    def unified(self):
        """
        Will return True if the file we have opened is unified.
        """
        return self.is_unified

    def assert_headers(self):
        if self.rst_headers is None:
            self.rst_headers = []
            if self.unified():
                for index in range(self.num_named_kw("SEQNUM")):
                    self.rst_headers.append(
                        ResdataRestartHead(
                            rst_view=self.restartView(seqnum_index=index)
                        )
                    )
            else:
                intehead_kw = self["INTEHEAD"][0]
                doubhead_kw = self["DOUBHEAD"][0]
                if "LOGIHEAD" in self:
                    logihead_kw = self["LOGIHEAD"][0]
                else:
                    logihead_kw = None

                self.rst_headers.append(
                    ResdataRestartHead(
                        kw_arg=(self.report_step, intehead_kw, doubhead_kw, logihead_kw)
                    )
                )

    def time_list(self):
        """Will return a list of report_step, simulation time and days.

        The return value will be a list tuples. For a unified restart
        file with the three report steps {10,15,20} it can look like:

           [  (10, datetime.datetime( 2010 , 1 , 1 , 0 , 0 , 0 ) , 100.0),
              (15, datetime.datetime( 2010 , 3 , 1 , 0 , 0 , 0 ) , 160.0),
              (20, datetime.datetime( 2010 , 5 , 1 , 0 , 0 , 0 ) , 220.0) ]

        For a non-unified restart file the list will have only one element.
        """

        self.assertHeaders()
        time_list = []
        for header in self.rst_headers:
            time_list.append(
                (header.getReportStep(), header.getSimDate(), header.getSimDays())
            )

        return time_list

    def headers(self):
        self.assertHeaders()
        return self.rst_headers

    def get_header(self, index):
        self.assertHeaders()
        return self.rst_headers[index]


monkey_the_camel(
    ResdataRestartHead, "getReportStep", ResdataRestartHead.get_report_step
)
monkey_the_camel(ResdataRestartHead, "getSimDate", ResdataRestartHead.get_sim_date)
monkey_the_camel(ResdataRestartHead, "getSimDays", ResdataRestartHead.get_sim_days)

monkey_the_camel(ResdataRestartFile, "assertHeaders", ResdataRestartFile.assert_headers)
monkey_the_camel(ResdataRestartFile, "timeList", ResdataRestartFile.time_list)
