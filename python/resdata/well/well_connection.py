from cwrap import BaseCClass

import resdata.well._well_connection as _well_connection

from .well_connection_direction_enum import WellConnectionDirection


class WellConnection(BaseCClass):
    TYPE_NAME = "rd_well_connect"

    def __init__(self):
        raise NotImplementedError("Class can not be instantiated directly")

    def isOpen(self) -> bool:
        return _well_connection._is_open(self)

    def ijk(self) -> tuple[int, int, int]:
        i = _well_connection._i(self)
        j = _well_connection._j(self)
        k = _well_connection._k(self)
        return i, j, k

    def direction(self) -> WellConnectionDirection:
        return WellConnectionDirection(_well_connection._get_dir(self))

    def segmentId(self) -> int:
        return _well_connection._segment_id(self)

    def isFractureConnection(self) -> bool:
        return _well_connection._fracture_connection(self)

    def isMatrixConnection(self) -> bool:
        return _well_connection._matrix_connection(self)

    def connectionFactor(self) -> float:
        return _well_connection._connection_factor(self)

    def __eq__(self, other) -> bool:
        return _well_connection._equal(self, other)

    def __hash__(self) -> int:
        return id(self)

    def __ne__(self, other) -> bool:
        return not self == other

    def free(self):
        pass

    def isMultiSegmentWell(self) -> bool:
        return _well_connection._is_msw(self)

    def __repr__(self) -> str:
        ijk = str(self.ijk())
        frac = "fracture " if self.isFractureConnection() else ""
        open_ = "open " if self.isOpen() else "shut "
        msw = " (multi segment)" if self.isMultiSegmentWell() else ""
        direction = WellConnectionDirection(self.direction())
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
                direction,
                addr,
            )
        )

    def gasRate(self) -> float:
        return _well_connection._gas_rate(self)

    def waterRate(self) -> float:
        return _well_connection._water_rate(self)

    def oilRate(self) -> float:
        return _well_connection._oil_rate(self)

    def volumeRate(self) -> float:
        return _well_connection._volume_rate(self)

    def gasRateSI(self) -> float:
        return _well_connection._gas_rate_si(self)

    def waterRateSI(self) -> float:
        return _well_connection._water_rate_si(self)

    def oilRateSI(self) -> float:
        return _well_connection._oil_rate_si(self)

    def volumeRateSI(self) -> float:
        return _well_connection._volume_rate_si(self)
