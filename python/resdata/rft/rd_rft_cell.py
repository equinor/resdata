from cwrap import BaseCClass
from resdata import ResdataPrototype


class RFTCell(BaseCClass):
    """The RFTCell is a base class for the cells which are part of an RFT/PLT.

    The RFTCell class contains the elements which are common to both
    RFT and PLT. The list of common elements include the corrdinates
    (i,j,k) the pressure and the depth of the cell. Actual user access
    should be based on the derived classes ResdataRFTCell and ResdataPLTCell.

    Observe that from june 2013 the properties i,j and k which return
    offset 1 coordinate values are deprecated, and you should rather
    use the methods get_i(), get_j() and get_k() which return offset 0
    coordinate values.
    """

    TYPE_NAME = "rd_rft_cell_base"
    _free = ResdataPrototype("void rd_rft_cell_free(rd_rft_cell_base)")
    _get_pressure = ResdataPrototype(
        "double rd_rft_cell_get_pressure(rd_rft_cell_base)"
    )
    _get_depth = ResdataPrototype("double rd_rft_cell_get_depth(rd_rft_cell_base)")
    _get_i = ResdataPrototype("int rd_rft_cell_get_i(rd_rft_cell_base)")
    _get_j = ResdataPrototype("int rd_rft_cell_get_j(rd_rft_cell_base)")
    _get_k = ResdataPrototype("int rd_rft_cell_get_k(rd_rft_cell_base)")

    def free(self):
        self._free()

    def get_i(self):
        return self._get_i()

    def get_j(self):
        return self._get_j()

    def get_k(self):
        return self._get_k()

    def get_ijk(self):
        return (self.get_i(), self.get_j(), self.get_k())

    @property
    def pressure(self):
        return self._get_pressure()

    @property
    def depth(self):
        return self._get_depth()


#################################################################


class ResdataRFTCell(RFTCell):
    TYPE_NAME = "rd_rft_cell"
    _alloc_RFT = ResdataPrototype(
        "void* rd_rft_cell_alloc_RFT(int, int, int, double, double, double, double)",
        bind=False,
    )
    _get_swat = ResdataPrototype("double rd_rft_cell_get_swat(rd_rft_cell)")
    _get_soil = ResdataPrototype("double rd_rft_cell_get_soil(rd_rft_cell)")
    _get_sgas = ResdataPrototype("double rd_rft_cell_get_sgas(rd_rft_cell)")

    def __init__(self, i, j, k, depth, pressure, swat, sgas):
        c_ptr = self._alloc_RFT(i, j, k, depth, pressure, swat, sgas)
        super(ResdataRFTCell, self).__init__(c_ptr)

    @property
    def swat(self):
        return self._get_swat()

    @property
    def sgas(self):
        return self._get_sgas()

    @property
    def soil(self):
        return 1 - (self._get_sgas() + self._get_swat())


#################################################################


class ResdataPLTCell(RFTCell):
    TYPE_NAME = "rd_plt_cell"
    _alloc_PLT = ResdataPrototype(
        "void* rd_rft_cell_alloc_PLT(int, int, int, double, double, double, double, double, double, double, double, double, double, double)",
        bind=False,
    )
    _get_orat = ResdataPrototype("double rd_rft_cell_get_orat(rd_plt_cell)")
    _get_grat = ResdataPrototype("double rd_rft_cell_get_grat(rd_plt_cell)")
    _get_wrat = ResdataPrototype("double rd_rft_cell_get_wrat(rd_plt_cell)")

    _get_flowrate = ResdataPrototype("double rd_rft_cell_get_flowrate(rd_plt_cell)")
    _get_oil_flowrate = ResdataPrototype(
        "double rd_rft_cell_get_oil_flowrate(rd_plt_cell)"
    )
    _get_gas_flowrate = ResdataPrototype(
        "double rd_rft_cell_get_gas_flowrate(rd_plt_cell)"
    )
    _get_water_flowrate = ResdataPrototype(
        "double rd_rft_cell_get_water_flowrate(rd_plt_cell)"
    )

    _get_conn_start = ResdataPrototype(
        "double rd_rft_cell_get_connection_start(rd_plt_cell)"
    )
    _get_conn_end = ResdataPrototype(
        "double rd_rft_cell_get_connection_end(rd_plt_cell)"
    )

    def __init__(
        self,
        i,
        j,
        k,
        depth,
        pressure,
        orat,
        grat,
        wrat,
        conn_start,
        conn_end,
        flowrate,
        oil_flowrate,
        gas_flowrate,
        water_flowrate,
    ):
        c_ptr = self._alloc_PLT(
            i,
            j,
            k,
            depth,
            pressure,
            orat,
            grat,
            wrat,
            conn_start,
            conn_end,
            flowrate,
            oil_flowrate,
            gas_flowrate,
            water_flowrate,
        )
        super(ResdataPLTCell, self).__init__(c_ptr)

    @property
    def orat(self):
        return self._get_orat()

    @property
    def grat(self):
        return self._get_grat()

    @property
    def wrat(self):
        return self._get_wrat()

    @property
    def conn_start(self):
        """Will return the length from wellhead(?) to connection.

        For MSW wells this property will return the distance from a
        fixed point (wellhead) to the current connection. This value
        will be used to sort the completed cells along the well
        path. In the case of non MSW wells this will just return a
        fixed default value.
        """
        return self._get_conn_start()

    @property
    def conn_end(self):
        """Will return the length from wellhead(?) to connection end.

        For MSW wells this property will return the distance from a
        fixed point (wellhead) to the current connection end. This value
        will be used to sort the completed cells along the well
        path. In the case of non MSW wells this will just return a
        fixed default value.
        """
        return self._get_conn_end()

    @property
    def flowrate(self):
        return self._get_flowrate()

    @property
    def oil_flowrate(self):
        return self._get_oil_flowrate()

    @property
    def gas_flowrate(self):
        return self._get_gas_flowrate()

    @property
    def water_flowrate(self):
        return self._get_water_flowrate()
