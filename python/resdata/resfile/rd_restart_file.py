from resdata import FileMode, FileType
from resdata.resfile import Resdata3DFile, ResdataFile
from resdata.util.util import monkey_the_camel

from ._rsthead import ResdataRestartHead


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

        file_type, report_step, fmt_file = ResdataFile.get_filetype(filename)
        if file_type not in [
            FileType.RESTART,
            FileType.UNIFIED_RESTART,
        ]:
            raise ValueError(
                'The input filename "%s" does not correspond to a restart file.  Please follow the Eclipse naming conventions'
                % filename
            )

        super().__init__(grid, filename, flags)
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
                            rst_view=self.restart_view(seqnum_index=index)
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

        self.assert_headers()
        time_list = []
        for header in self.rst_headers:
            time_list.append(
                (header.get_report_step(), header.get_sim_date(), header.get_sim_days())
            )

        return time_list

    def headers(self):
        self.assert_headers()
        return self.rst_headers

    def get_header(self, index):
        self.assert_headers()
        return self.rst_headers[index]


monkey_the_camel(
    ResdataRestartHead, "getReportStep", ResdataRestartHead.get_report_step
)
monkey_the_camel(ResdataRestartHead, "getSimDate", ResdataRestartHead.get_sim_date)
monkey_the_camel(ResdataRestartHead, "getSimDays", ResdataRestartHead.get_sim_days)

monkey_the_camel(ResdataRestartFile, "assertHeaders", ResdataRestartFile.assert_headers)
monkey_the_camel(ResdataRestartFile, "timeList", ResdataRestartFile.time_list)
