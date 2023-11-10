"""
Module for loading RFT files.
"""

from cwrap import BaseCClass
from resdata import ResdataPrototype
from resdata.rft import ResdataPLTCell, ResdataRFTCell
from resdata.util.util import CTime, monkey_the_camel


class ResdataRFT(BaseCClass):
    """The ResdataRFT class contains the information for *one* RFT.

    The RFT file can contain three different types of RFT like
    objects which are lumped together; the ResdataRFTClass is a container
    for such objects. The three different object types which can be
    found in an RFT file are:

       RFT: This is old-fashioned RFT which contains measurements of
            saturations for each of the completed cells.

       PLT: This contains production and flow rates for each phase in
            each cell.

       SEGMENT: Not implemented.

    In addition to the measurements specific for RFT and PLT each cell
    has coordinates, pressure and depth.
    """

    TYPE_NAME = "rd_rft"
    _alloc = ResdataPrototype(
        "void* rd_rft_node_alloc_new( char* , char* , rd_time_t , double)", bind=False
    )
    _free = ResdataPrototype("void  rd_rft_node_free( rd_rft )")
    _get_type = ResdataPrototype("int    rd_rft_node_get_type( rd_rft )")
    _get_well = ResdataPrototype("char*  rd_rft_node_get_well_name( rd_rft )")
    _get_date = ResdataPrototype("rd_time_t rd_rft_node_get_date( rd_rft )")
    _get_size = ResdataPrototype("int rd_rft_node_get_size( rd_rft )")
    _iget_cell = ResdataPrototype("void* rd_rft_node_iget_cell( rd_rft )")
    _iget_depth = ResdataPrototype("double rd_rft_node_iget_depth( rd_rft )")
    _iget_pressure = ResdataPrototype("double rd_rft_node_iget_pressure(rd_rft)")
    _iget_ijk = ResdataPrototype(
        "void rd_rft_node_iget_ijk( rd_rft , int , int*, int*, int*)"
    )
    _iget_swat = ResdataPrototype("double rd_rft_node_iget_swat(rd_rft)")
    _iget_sgas = ResdataPrototype("double rd_rft_node_iget_sgas(rd_rft)")
    _iget_orat = ResdataPrototype("double rd_rft_node_iget_orat(rd_rft)")
    _iget_wrat = ResdataPrototype("double rd_rft_node_iget_wrat(rd_rft)")
    _iget_grat = ResdataPrototype("double rd_rft_node_iget_grat(rd_rft)")
    _lookup_ijk = ResdataPrototype(
        "void* rd_rft_node_lookup_ijk( rd_rft , int , int , int)"
    )
    _is_RFT = ResdataPrototype("bool   rd_rft_node_is_RFT( rd_rft )")
    _is_PLT = ResdataPrototype("bool   rd_rft_node_is_PLT( rd_rft )")
    _is_SEGMENT = ResdataPrototype("bool   rd_rft_node_is_SEGMENT( rd_rft )")
    _is_MSW = ResdataPrototype("bool   rd_rft_node_is_MSW( rd_rft )")

    def __init__(self, name, type_string, date, days):
        c_ptr = self._alloc(name, type_string, CTime(date), days)
        super(ResdataRFT, self).__init__(c_ptr)

    def free(self):
        self._free()

    def __repr__(self):
        rs = []
        rs.append("completed_cells = %d" % len(self))
        rs.append("date = %s" % self.getDate())
        if self.is_RFT():
            rs.append("RFT")
        if self.is_PLT():
            rs.append("PLT")
        if self.is_SEGMENT():
            rs.append("SEGMENT")
        if self.is_MSW():
            rs.append("MSW")
        rstr = ", ".join(rs)
        return self._create_repr(rstr)

    def __len__(self):
        """
        The number of completed cells in this RFT.
        """
        return self._get_size()

    def is_RFT(self):
        """
        Is instance an RFT; in that case all the cells will be ResdataRFTCell instances.
        """
        return self._is_RFT()

    def is_PLT(self):
        """
        Is instance a PLT; in that case all the cells will be ResdataPLTCell instances.
        """
        return self._is_PLT()

    def is_SEGMENT(self):
        """
        Is this a SEGMENT - not implemented.
        """
        return self._is_SEGMENT()

    def is_MSW(self):
        """
        Is this well a MSW well. Observe that the test ONLY applies to PLTs.
        """
        return self._is_MSW()

    def get_well_name(self):
        """
        The name of the well we are considering.
        """
        return self._get_well()

    def get_date(self):
        """
        The date when this RFT/PLT/... was recorded.
        """
        ct = CTime(self._get_date())
        return ct.date()

    def __cell_ref(self, cell_ptr):
        if self.is_RFT():
            return ResdataRFTCell.createCReference(cell_ptr, self)
        elif self.is_PLT():
            return ResdataPLTCell.createCReference(cell_ptr, self)
        else:
            raise NotImplementedError("Only RFT and PLT cells are implemented")

    def assert_cell_index(self, index):
        if isinstance(index, int):
            length = self.__len__()
            if index < 0 or index >= length:
                raise IndexError
        else:
            raise TypeError("Index should be integer type")

    def __getitem__(self, index):
        """Implements the [] operator to return the cells.

        To get the object related to cell nr 5:

           cell = rft[4]

        The return value from the __getitem__() method is either an
        ResdataRFTCell instance or a ResdataPLTCell instance, depending on the
        type of this particular RFT object.

        For MSW wells the cells will come in sorted order along the wellpath,
        for other well types the cells will come sorted in input order.
        """
        self.assert_cell_index(index)
        cell_ptr = self._iget_cell(index)
        return self.__cell_ref(cell_ptr)

    def iget(self, index):
        return self[index]

    # ijk are zero offset
    def ijkget(self, ijk):
        """
        Will look up the cell based on (i,j,k).

        If the cell (i,j,k) is not part of this RFT/PLT None will be
        returned. The (i,j,k) input values should be zero offset,
        i.e. you must subtract 1 from the (i,j,k) values given in the ECLIPSE input.
        """
        cell_ptr = self._lookup_ijk(ijk[0], ijk[1], ijk[2])
        if cell_ptr:
            return self.__cell_ref(cell_ptr)
        else:
            return None


class ResdataRFTFile(BaseCClass):
    TYPE_NAME = "rd_rft_file"
    _load = ResdataPrototype("void* rd_rft_file_alloc_case( char* )", bind=False)
    _iget = ResdataPrototype("rd_rft_ref rd_rft_file_iget_node( rd_rft_file , int )")
    _get_rft = ResdataPrototype(
        "rd_rft_ref rd_rft_file_get_well_time_rft( rd_rft_file , char* , rd_time_t)"
    )
    _has_rft = ResdataPrototype("bool rd_rft_file_case_has_rft( char* )", bind=False)
    _free = ResdataPrototype("void rd_rft_file_free( rd_rft_file )")
    _get_size = ResdataPrototype(
        "int rd_rft_file_get_size__( rd_rft_file , char* , rd_time_t)"
    )
    _get_num_wells = ResdataPrototype("int  rd_rft_file_get_num_wells( rd_rft_file )")

    """
    The ResdataRFTFile class is used to load an RFT file.

    The ResdataRFTFile serves as a container which can load and hold the
    content of an RFT file. The RFT files will in general
    contain data for several wells and several times in one large
    container. The ResdataRFTClass class contains methods get the the RFT
    results for a specific time and/or well.

    The ResdataRFTFile class can in general contain a mix of RFT and PLT
    measurements. The class does not really differentiate between
    these.
    """

    def __init__(self, case):
        c_ptr = self._load(case)
        super(ResdataRFTFile, self).__init__(c_ptr)

    def __len__(self):
        return self._get_size(None, CTime(-1))

    def __getitem__(self, index):
        if isinstance(index, int):
            if 0 <= index < len(self):
                rft = self._iget(index)
                rft.setParent(self)
                return rft
            else:
                raise IndexError(
                    "Index '%d' must be in range: [0, %d]" % (index, len(self) - 1)
                )
        else:
            raise TypeError("Index must be integer type")

    def size(self, well=None, date=None):
        """
        The number of elements in ResdataRFTFile container.

        By default the size() method will return the total number of
        RFTs/PLTs in the container, but by specifying the optional
        arguments date and/or well the function will only count the
        number of well measurements matching that time or well
        name. The well argument can contain wildcards.

           >>> rftFile = resdata.ResdataRFTFile( "CASE.RFT" )
           >>> print "Total number of RFTs : %d" % rftFile.size( )
           >>> print "RFTs matching OP*    : %d" % rftFile.size( well = "OP*" )
           >>> print "RFTs at 01/01/2010   : %d" % rftFile.size( date = datetime.date( 2010 , 1 , 1 ))

        """
        if date:
            cdate = CTime(date)
        else:
            cdate = CTime(-1)

        return self._get_size(well, cdate)

    def get_num_wells(self):
        """
        Returns the total number of distinct wells in the RFT file.
        """
        return self._get_num_wells()

    def get_headers(self):
        """
        Returns a list of two tuples (well_name , date) for the whole file.
        """
        header_list = []
        for i in range(self._get_size(None, CTime(-1))):
            rft = self.iget(i)
            header_list.append((rft.getWellName(), rft.getDate()))
        return header_list

    def iget(self, index):
        """
        Will lookup RFT @index - equivalent to [@index].
        """
        return self[index]

    def get(self, well_name, date):
        """
        Will look up the RFT object corresponding to @well and @date.

        Raise Exception if not found.
        """
        if self.size(well=well_name, date=date) == 0:
            raise KeyError("No RFT for well:%s at %s" % (well_name, date))

        rft = self._get_rft(well_name, CTime(date))
        rft.setParent(self)
        return rft

    def free(self):
        self._free()

    def __repr__(self):
        w = len(self)
        return self._create_repr("wells = %d" % w)


monkey_the_camel(ResdataRFT, "getWellName", ResdataRFT.get_well_name)
monkey_the_camel(ResdataRFT, "getDate", ResdataRFT.get_date)

monkey_the_camel(ResdataRFTFile, "getNumWells", ResdataRFTFile.get_num_wells)
monkey_the_camel(ResdataRFTFile, "getHeaders", ResdataRFTFile.get_headers)
