from os.path import isfile
from cwrap import BaseCClass
from resdata.grid import Grid
from resdata.resfile.rd_file import ResdataFile
from resdata.well import WellTimeLine
from resdata import ResdataPrototype


class WellInfo(BaseCClass):
    TYPE_NAME = "rd_well_info"

    _alloc = ResdataPrototype("void* well_info_alloc(rd_grid)", bind=False)
    _free = ResdataPrototype("void well_info_free(rd_well_info)")
    _load_rstfile = ResdataPrototype(
        "void well_info_load_rstfile(rd_well_info, char*, bool)"
    )
    _load_rst_resfile = ResdataPrototype(
        "void well_info_load_rst_resfile(rd_well_info, rd_file, bool)"
    )
    _get_well_count = ResdataPrototype("int well_info_get_num_wells(rd_well_info)")
    _iget_well_name = ResdataPrototype(
        "char* well_info_iget_well_name(rd_well_info, int)"
    )
    _has_well = ResdataPrototype("bool well_info_has_well(rd_well_info, char*)")
    _get_ts = ResdataPrototype(
        "rd_well_time_line_ref well_info_get_ts(rd_well_info, char*)"
    )

    def __init__(self, grid, rst_file=None, load_segment_information=True):
        """
        @type grid: Grid
        @type rst_file: str or ResdataFile or list of str or list of ResdataFile
        """
        c_ptr = self._alloc(grid)
        super(WellInfo, self).__init__(c_ptr)
        if not c_ptr:
            raise ValueError("Unable to construct WellInfo from grid %s." % str(grid))

        if rst_file is not None:
            if isinstance(rst_file, list):
                for item in rst_file:
                    self.addWellFile(item, load_segment_information)
            else:
                self.addWellFile(rst_file, load_segment_information)

    def __repr__(self):
        return "WellInfo(well_count = %d) at 0x%x" % (len(self), self._address())

    def __len__(self):
        """@rtype: int"""
        return self._get_well_count()

    def __getitem__(self, item):
        """
        @type item: int or str
        @rtype: WellTimeLine
        """

        if isinstance(item, str):
            if not item in self:
                raise KeyError("The well '%s' is not in this set." % item)
            well_name = item

        elif isinstance(item, int):
            if not 0 <= item < len(self):
                raise IndexError(
                    "Index must be in range 0 <= %d < %d" % (item, len(self))
                )
            well_name = self._iget_well_name(item)

        return self._get_ts(well_name).setParent(self)

    def __iter__(self):
        """@rtype: iterator of WellTimeLine"""
        index = 0

        while index < len(self):
            yield self[index]
            index += 1

    def allWellNames(self):
        """@rtype: list of str"""
        return [self._iget_well_name(index) for index in range(0, len(self))]

    def __contains__(self, item):
        """
        @type item: str
        @rtype: bool
        """
        return self._has_well(item)

    def _assert_file_exists(self, rst_file):
        if not isfile(rst_file):
            raise IOError("No such file %s" % rst_file)

    def addWellFile(self, rst_file, load_segment_information):
        """@type rstfile: str or ResdataFile"""
        if isinstance(rst_file, str):
            self._assert_file_exists(rst_file)
            self._load_rstfile(rst_file, load_segment_information)
        elif isinstance(rst_file, ResdataFile):
            self._load_rst_resfile(rst_file, load_segment_information)
        else:
            raise TypeError(
                "Expected the RST file to be a filename or an ResdataFile instance."
            )

    def hasWell(self, well_name):
        return well_name in self

    def free(self):
        self._free()
