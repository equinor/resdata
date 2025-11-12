class RFTCell:
    """The RFTCell is a base class for the cells which are part of an RFT/PLT.

    The RFTCell class contains the elements which are common to both
    RFT and PLT. The list of common elements include the coordinates
    (i,j,k) the pressure and the depth of the cell. Actual user access
    should be based on the derived classes ResdataRFTCell and ResdataPLTCell.
    """

    def __init__(
        self,
        i: int,
        j: int,
        k: int,
        pressure: float | None,
        depth: float | None,
    ) -> None:
        self._i = i
        self._j = j
        self._k = k
        self._pressure = pressure
        self._depth = depth

    def get_i(self) -> int:
        return self._i

    def get_j(self) -> int:
        return self._j

    def get_k(self) -> int:
        return self._k

    def get_ijk(self) -> tuple[int, int, int]:
        return (self.get_i(), self.get_j(), self.get_k())

    @property
    def pressure(self) -> float | None:
        return self._pressure

    @property
    def depth(self) -> float | None:
        return self._depth


class ResdataRFTCell(RFTCell):
    def __init__(
        self,
        i: int,
        j: int,
        k: int,
        depth: float | None,
        pressure: float | None,
        swat: float | None,
        sgas: float | None,
    ) -> None:
        super().__init__(i, j, k, pressure, depth)
        self._swat = swat
        self._sgas = sgas

    @property
    def swat(self) -> float | None:
        return self._swat

    @property
    def sgas(self) -> float | None:
        return self._sgas

    @property
    def soil(self) -> float | None:
        sgas = self.sgas
        swat = self.swat
        if sgas is None:
            return None
        if swat is None:
            return None
        return 1 - (sgas + swat)


class ResdataPLTCell(RFTCell):
    def __init__(
        self,
        i: int,
        j: int,
        k: int,
        depth: float | None,
        pressure: float | None,
        orat: float | None,
        grat: float | None,
        wrat: float | None,
        conn_start: float,
        conn_end: float,
        flowrate: float | None,
        oil_flowrate: float | None,
        gas_flowrate: float | None,
        water_flowrate: float | None,
    ) -> None:
        super().__init__(i, j, k, pressure, depth)
        self._orat = orat
        self._grat = grat
        self._wrat = wrat
        self._conn_start = conn_start
        self._conn_end = conn_end
        self._flowrate = flowrate
        self._oil_flowrate = oil_flowrate
        self._gas_flowrate = gas_flowrate
        self._water_flowrate = water_flowrate

    @property
    def orat(self) -> float | None:
        return self._orat

    @property
    def grat(self) -> float | None:
        return self._grat

    @property
    def wrat(self) -> float | None:
        return self._wrat

    @property
    def conn_start(self) -> float:
        """Will return the length from wellhead(?) to connection.

        For multi-segment wells (MSW) this property will return the distance from a
        fixed point (wellhead) to the current connection. This value
        will be used to sort the completed cells along the well
        path. In the case of non MSW wells this will just return 0.
        """
        return self._conn_start

    @property
    def conn_end(self) -> float:
        """Will return the length from wellhead(?) to connection end.

        For multi-segment wells (MSW) this property will return the distance from a
        fixed point (wellhead) to the current connection end. This value
        will be used to sort the completed cells along the well
        path. In the case of non MSW wells this will just return 0.
        """
        return self._conn_end

    @property
    def flowrate(self) -> float | None:
        return self._flowrate

    @property
    def oil_flowrate(self) -> float | None:
        return self._oil_flowrate

    @property
    def gas_flowrate(self) -> float | None:
        return self._gas_flowrate

    @property
    def water_flowrate(self) -> float | None:
        return self._water_flowrate
