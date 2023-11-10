from cwrap import BaseCClass
from resdata import ResdataPrototype
from resdata.well import WellConnectionDirection


class WellConnection(BaseCClass):
    TYPE_NAME = "rd_well_connect"

    _i = ResdataPrototype("int well_conn_get_i(rd_well_connect)")
    _j = ResdataPrototype("int well_conn_get_j(rd_well_connect)")
    _k = ResdataPrototype("int well_conn_get_k(rd_well_connect)")
    _segment_id = ResdataPrototype("int well_conn_get_segment_id(rd_well_connect)")
    _is_open = ResdataPrototype("bool well_conn_open(rd_well_connect)")
    _is_msw = ResdataPrototype("bool well_conn_MSW(rd_well_connect)")
    _fracture_connection = ResdataPrototype(
        "bool well_conn_fracture_connection(rd_well_connect)"
    )
    _matrix_connection = ResdataPrototype(
        "bool well_conn_matrix_connection(rd_well_connect)"
    )
    _connection_factor = ResdataPrototype(
        "double well_conn_get_connection_factor(rd_well_connect)"
    )
    _equal = ResdataPrototype("bool well_conn_equal(rd_well_connect, rd_well_connect)")
    _get_dir = ResdataPrototype(
        "rd_well_connection_dir well_conn_get_dir(rd_well_connect)"
    )
    _oil_rate = ResdataPrototype("double well_conn_get_oil_rate(rd_well_connect)")
    _gas_rate = ResdataPrototype("double well_conn_get_gas_rate(rd_well_connect)")
    _water_rate = ResdataPrototype("double well_conn_get_water_rate(rd_well_connect)")
    _volume_rate = ResdataPrototype("double well_conn_get_volume_rate(rd_well_connect)")

    _oil_rate_si = ResdataPrototype("double well_conn_get_oil_rate_si(rd_well_connect)")
    _gas_rate_si = ResdataPrototype("double well_conn_get_gas_rate_si(rd_well_connect)")
    _water_rate_si = ResdataPrototype(
        "double well_conn_get_water_rate_si(rd_well_connect)"
    )
    _volume_rate_si = ResdataPrototype(
        "double well_conn_get_volume_rate_si(rd_well_connect)"
    )

    def __init__(self):
        raise NotImplementedError("Class can not be instantiated directly")

    def isOpen(self):
        """@rtype: bool"""
        return self._is_open()

    def ijk(self):
        """@rtype: tuple of (int, int, int)"""
        i = self._i()
        j = self._j()
        k = self._k()
        return i, j, k

    def direction(self):
        """@rtype: WellConnectionDirection"""
        return self._get_dir()

    def segmentId(self):
        """@rtype: int"""
        return self._segment_id()

    def isFractureConnection(self):
        """@rtype: bool"""
        return self._fracture_connection()

    def isMatrixConnection(self):
        """@rtype: bool"""
        return self._matrix_connection()

    def connectionFactor(self):
        """@rtype: float"""
        return self._connection_factor()

    def __eq__(self, other):
        return self._equal(other)

    def __hash__(self):
        return id(self)

    def __ne__(self, other):
        return not self == other

    def free(self):
        pass

    def isMultiSegmentWell(self):
        """@rtype: bool"""
        return self._is_msw()

    def __repr__(self):
        ijk = str(self.ijk())
        frac = "fracture " if self.isFractureConnection() else ""
        open_ = "open " if self.isOpen() else "shut "
        msw = " (multi segment)" if self.isMultiSegmentWell() else ""
        dir = WellConnectionDirection(self.direction())
        addr = self._address()
        return (
            "WellConnection(%s %s%s%s, rates = (O:%s,G:%s,W:%s), direction = %s) at 0x%x"
            % (
                ijk,
                frac,
                open_,
                msw,
                self.oilRate(),
                self.gasRate(),
                self.waterRate(),
                dir,
                addr,
            )
        )

    def gasRate(self):
        return self._gas_rate()

    def waterRate(self):
        return self._water_rate()

    def oilRate(self):
        return self._oil_rate()

    def volumeRate(self):
        return self._volume_rate()

    def gasRateSI(self):
        return self._gas_rate_si()

    def waterRateSI(self):
        return self._water_rate_si()

    def oilRateSI(self):
        return self._oil_rate_si()

    def volumeRateSI(self):
        return self._volume_rate_si()
