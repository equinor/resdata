from cwrap import BaseCClass

import resdata.well._well_state as _well_state
from resdata.util.util import CTime

from .well_connection import WellConnection
from .well_segment import WellSegment
from .well_type_enum import WellType


class WellState(BaseCClass):
    TYPE_NAME = "rd_well_state"

    def __init__(self):
        raise NotImplementedError("Class can not be instantiated directly")

    def name(self) -> str:
        return _well_state._name(self)

    def isOpen(self) -> bool:
        return _well_state._is_open(self)

    def free(self) -> None:
        pass

    def wellHead(self) -> WellConnection | None:
        return _well_state._well_head(self)

    def wellNumber(self) -> int:
        return _well_state._well_number(self)

    def reportNumber(self) -> int:
        return _well_state._report_number(self)

    def simulationTime(self) -> CTime:
        return CTime(_well_state._sim_time(self))

    def wellType(self) -> WellType:
        return WellType(_well_state._well_type(self))

    def hasGlobalConnections(self) -> bool:
        return _well_state._has_global_connections(self)

    def globalConnections(self) -> list[WellConnection]:
        """The list of well connections for the global grid.

        Note: Constructs a new list of references to the well connections.
        """
        return _well_state.global_connections(self)

    def __len__(self) -> int:
        return self.numSegments()

    def __getitem__(self, idx):
        return self.igetSegment(idx)

    def numSegments(self) -> int:
        return _well_state._num_segments(self)

    def segments(self) -> list[WellSegment]:
        """The list of segments in the well.

        Note: Constructs a new list of references to the well segments.
        """
        return _well_state._segments(self)

    def igetSegment(self, seg_idx: int) -> WellSegment:
        if seg_idx < 0:
            seg_idx += len(self)

        if not 0 <= seg_idx < self.numSegments():
            raise IndexError(
                "Invalid index:%d - valid range [0,%d)" % (seg_idx, len(self))
            )
        return _well_state._iget_segment(self, seg_idx)

    def isMultiSegmentWell(self) -> bool:
        return _well_state._is_msw(self)

    def hasSegmentData(self) -> bool:
        return _well_state._has_segment_data(self)

    def __repr__(self) -> str:
        name = self.name()
        if name:
            name = "%s" % name
        else:
            name = "[no name]"
        msw = " (multi segment)" if self.isMultiSegmentWell() else ""
        wn = str(self.wellNumber())
        type_ = self.wellType()
        open_ = "open" if self.isOpen() else "shut"
        cnt = '%s%s, number = %s, type = "%s", state = %s' % (
            name,
            msw,
            wn,
            type_,
            open_,
        )
        return self._create_repr(cnt)

    def gasRate(self) -> float:
        """The gas rate, as stored in the restart file.

        The physical unit depends on the file's unit system: sm3/day (metric),
        Mscf/day (field) or cm3/hour (lab). Use :meth:`gasRateSI` to get the
        value converted to SI units.
        """
        return _well_state._gas_rate(self)

    def waterRate(self) -> float:
        """The water rate, as stored in the restart file.

        The physical unit depends on the file's unit system: sm3/day (metric),
        stb/day (field) or cm3/hour (lab). Use :meth:`waterRateSI` to get the
        value converted to SI units.
        """
        return _well_state._water_rate(self)

    def oilRate(self) -> float:
        """The oil rate, as stored in the restart file.

        The physical unit depends on the file's unit system: sm3/day (metric),
        stb/day (field) or cm3/hour (lab). Use :meth:`oilRateSI` to get the
        value converted to SI units.
        """
        return _well_state._oil_rate(self)

    def volumeRate(self) -> float:
        """The volume rate, at reservoir conditions."""
        return _well_state._volume_rate(self)

    def gasRateSI(self) -> float:
        """The gas rate converted to SI units (m3/s).

        This is the raw :meth:`gasRate` multiplied by a unit-system dependent
        conversion factor.
        """
        return _well_state._gas_rate_si(self)

    def waterRateSI(self) -> float:
        """The water rate converted to SI units (m3/s).

        This is the raw :meth:`waterRate` multiplied by a unit-system dependent
        conversion factor.
        """
        return _well_state._water_rate_si(self)

    def oilRateSI(self) -> float:
        """The oil rate converted to SI units (m3/s).

        This is the raw :meth:`oilRate` multiplied by a unit-system dependent
        conversion factor.
        """
        return _well_state._oil_rate_si(self)

    def volumeRateSI(self) -> float:
        """The volume rate, at reservoir conditions, converted to SI units (m3/s).

        This is the raw :meth:`volumeRate` multiplied by a unit-system dependent
        conversion factor.
        """
        return _well_state._volume_rate_si(self)
