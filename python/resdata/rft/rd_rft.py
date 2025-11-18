"""Module for loading RFT files."""

from resdata.rft import ResdataPLTCell, ResdataRFTCell
from resdata.util.util import monkey_the_camel
from os import PathLike
from typing import Any
from collections.abc import Mapping
import datetime
import numpy.typing as npt
import numpy as np
from resfo_utilities import RFTReader
import fnmatch
import warnings


def to_float_or_none(value: np.float32 | None) -> float | None:
    return float(value) if value is not None else None


class ResdataRFT:
    """Contains the information for *one* RFT.

    The RFT file can contain three different types of RFT data. ResdataRFT
    is a container of these. The types are:

       RFT: Contains measurements of saturations for each of the completed cells.

       PLT: This contains production and flow rates for each phase in each cell.

       SEGMENT: Not implemented.

    In addition to the measurements specific for RFT, PLT and SEGMENT each cell
    has coordinates, pressure and depth.
    """

    def __init__(
        self,
        name: str,
        type_string: str,
        date: datetime.date,
        days: float,
        connections: np.ndarray[tuple[int, int], np.dtype[np.int32]] | None = None,
        values: Mapping[str, npt.NDArray[np.float32]] | None = None,
        is_msw: bool = False,
    ) -> None:
        self._name = name
        self._type_string = type_string
        self._date = date
        self._days = days
        self._is_msw = is_msw
        connections_ = connections if connections is not None else []
        empty_values = [None] * len(connections_)
        zeros = np.zeros(len(connections_), np.float32)
        values_ = values or {}
        self._cells: list[ResdataRFTCell | ResdataPLTCell] = []
        match type_string:
            case "RFT":
                depth = values_.get("DEPTH", empty_values)
                pressure = values_.get("PRESSURE", empty_values)
                swat = values_.get("SWAT", empty_values)
                sgas = values_.get("SGAS", empty_values)
                for index, grid_index in (
                    enumerate(connections) if connections is not None else []
                ):
                    i, j, k = grid_index - 1
                    # Type cast cell properties for backwards compatibility
                    self._cells.append(
                        ResdataRFTCell(
                            int(i),
                            int(j),
                            int(k),
                            to_float_or_none(depth[index]),
                            to_float_or_none(pressure[index]),
                            to_float_or_none(swat[index]),
                            to_float_or_none(sgas[index]),
                        )
                    )
            case "PLT":
                condepth = values_.get("CONDEPTH", empty_values)
                conpres = values_.get("CONPRES", zeros)
                if sum(conpres) <= 0:
                    warnings.warn("Got all-zero CONPRES.")
                    if "PRESSURE" in values_:
                        conpres = values_["PRESSURE"]
                    else:
                        warnings.warn("No PRESSURE values to replace all-zero CONPRES.")
                else:
                    conpres = values_["CONPRES"]
                orat = values_.get("CONORAT", empty_values)
                grat = values_.get("CONGRAT", empty_values)
                wrat = values_.get("CONWRAT", empty_values)
                # conn is 0 when missing for backwards compatibility
                conn_start = values_.get("CONLENST", zeros)
                conn_end = values_.get("CONLENEN", zeros)
                flowrate = values_.get("CONVTUB", empty_values)
                oil_flowrate = values_.get("CONOTUB", empty_values)
                gas_flowrate = values_.get("CONGTUB", empty_values)
                water_flowrate = values_.get("CONWTUB", empty_values)
                for index, grid_index in enumerate(connections_):
                    i, j, k = grid_index - 1
                    # Type cast cell properties for backwards compatibility
                    self._cells.append(
                        ResdataPLTCell(
                            int(i),
                            int(j),
                            int(k),
                            to_float_or_none(condepth[index]),
                            to_float_or_none(conpres[index]),
                            to_float_or_none(orat[index]),
                            to_float_or_none(grat[index]),
                            to_float_or_none(wrat[index]),
                            float(conn_start[index]),
                            float(conn_end[index]),
                            to_float_or_none(flowrate[index]),
                            to_float_or_none(oil_flowrate[index]),
                            to_float_or_none(gas_flowrate[index]),
                            to_float_or_none(water_flowrate[index]),
                        )
                    )
                if self._is_msw:
                    self._cells.sort(key=lambda x: x.conn_start)
            case default:
                warnings.warn(f"Encountered unimplemented RFT data type {default}")

    def __repr__(self) -> str:
        rs = []
        rs.append("completed_cells = %d" % len(self))
        rs.append("date = %s" % self.get_date())
        if self.is_RFT():
            rs.append("RFT")
        if self.is_PLT():
            rs.append("PLT")
        if self.is_SEGMENT():
            rs.append("SEGMENT")
        if self.is_MSW():
            rs.append("MSW")
        rstr = ", ".join(rs)
        return f"{self.__class__.__name__}({rstr}) {id(self)}"

    def __len__(self) -> int:
        """The number of cells in this RFT."""
        return len(self._cells)

    def is_RFT(self) -> bool:
        """Whether the cells will be ResdataRFTCell instances."""
        return self._type_string == "RFT"

    def is_PLT(self) -> bool:
        """Whether the cells will be ResdataPLTCell instances."""
        return self._type_string == "PLT"

    def is_SEGMENT(self) -> bool:
        """Is this a SEGMENT - not implemented."""
        return self._type_string == "SEGMENT"

    def is_MSW(self) -> bool:
        """Is this well a multi-segment well."""
        return self._is_msw

    def get_well_name(self) -> str:
        """The name of the well we are considering."""
        return self._name

    def get_date(self) -> datetime.date:
        """The date when this RFT/PLT/... was recorded."""
        return self._date

    def assert_cell_index(self, index: Any) -> None:
        if isinstance(index, int):
            length = self.__len__()
            if index < 0 or index >= length:
                raise IndexError()
        else:
            raise TypeError("Index should be integer type")

    def __getitem__(self, index: int) -> ResdataRFTCell | ResdataPLTCell:
        """
        For MSW wells the cells will come in sorted order along the wellpath,
        for other well types the cells will come sorted in input order.
        """
        return self._cells[index]

    def iget(self, index):
        return self[index]

    def ijkget(
        self, ijk: tuple[int, int, int]
    ) -> ResdataRFTCell | ResdataPLTCell | None:
        """Look up the cell based on (i,j,k).

        If the cell (i,j,k) is not part of this RFT/PLT None will be
        returned. The (i,j,k) input values should be zero offset,
        i.e. you must subtract 1 from the (i,j,k) values given in the RFT file.
        """
        for c in self._cells:
            if c.get_ijk() == ijk:
                return c
        return None


def category_to_type_str(s: str) -> str | None:
    if "P" in s:
        return "PLT"
    if "R" in s:
        return "RFT"
    warnings.warn(f"Unsupported rft type {s}")
    return None


class ResdataRFTFile:
    """Used to load an RFT file.

    The ResdataRFTFile is a container which can load and hold the
    content of an RFT file. The RFT files will in general contain data for
    several wells and several times. A ResdataRFT has methods to get the
    RFT results for a specific time and well.

    A ResdataRFTFile can contain a mix of RFT and PLT measurements.
    """

    def __init__(self, case: str | PathLike[str]) -> None:
        warnings.warn(
            "ResdataRFTFile is deprecated, see "
            "resfo-utilities.readthedocs.io/en/latest/user_guide.html"
            "#module-resfo_utilities._rft_reader to migrate to resfo-utilities.",
            DeprecationWarning,
        )
        try:
            with RFTReader.open(case) as rft:
                self._entries = [
                    ResdataRFT(
                        e.well,
                        category_to_type_str(e.types_of_data),
                        e.date,
                        e.time_since_start.total_seconds() / (60 * 60 * 24),
                        e.connections,
                        e,
                        "CONLENST" in e,  # done for backwards compatibility
                        # the check e.type_of_well == "MULTSEG" would be more correct
                    )
                    for e in rft
                    if "P" in e.types_of_data or "R" in e.types_of_data
                ]
        except FileNotFoundError as err:
            raise ValueError(str(err)) from err  # For backwards compatibility

    def __len__(self) -> int:
        return len(self._entries)

    def __getitem__(self, index: int) -> ResdataRFT:
        if isinstance(index, int):
            if 0 <= index < len(self):
                return self._entries[index]
            else:
                raise IndexError(
                    "Index '%d' must be in range: [0, %d]" % (index, len(self) - 1)
                )
        else:
            raise TypeError("Index must be integer type")

    def size(self, well: str | None = None, date: datetime.date | None = None) -> int:
        """The number of elements matching the given well pattern and date.

        By default the size() method will return the total number of
        RFTs/PLTs in the container, but by specifying the optional
        arguments date and/or well the function will only count the
        number of well measurements matching that time or well
        name. The well argument can contain wildcards.

        >>> rftFile = resdata.ResdataRFTFile("CASE.RFT")
        >>> print(f"Total number of RFTs: {rftFile.size()}")
        >>> print(f"RFTs matching OP*: {rftFile.size(well="OP*")}")
        >>> print(f"RFTs at 01/01/2010: {rftFile.size(date=datetime.date(2010, 1, 1))}")
        """
        return sum(
            1
            for e in self._entries
            if (well is None or fnmatch.fnmatch(e.get_well_name(), well))
            and (date is None or e.get_date() == date)
        )

    def get_num_wells(self) -> int:
        """The total number of distinct wells in the RFT file."""
        return len(set(e.get_well_name() for e in self._entries))

    def get_headers(self) -> list[tuple[str, datetime.date]]:
        """List of two tuples (well_name , date) for the whole file."""
        return [(rft.get_well_name(), rft.get_date()) for rft in self._entries]

    def iget(self, index: int) -> ResdataRFT:
        """equivalent to __getitem__."""
        return self[index]

    def get(self, well_name: str, date: datetime.date) -> ResdataRFT:
        """look up the RFT corresponding to well and date.

        Raises KeyError if not found.
        """
        for rft in self._entries:
            if rft.get_date() == date and rft.get_well_name() == well_name:
                return rft
        raise KeyError("No RFT for well:%s at %s" % (well_name, date))

    def __repr__(self) -> str:
        return f"{self.__class__.__name__}(wells = {len(self)}) {id(self)}"


monkey_the_camel(ResdataRFT, "getWellName", ResdataRFT.get_well_name)
monkey_the_camel(ResdataRFT, "getDate", ResdataRFT.get_date)

monkey_the_camel(ResdataRFTFile, "getNumWells", ResdataRFTFile.get_num_wells)
monkey_the_camel(ResdataRFTFile, "getHeaders", ResdataRFTFile.get_headers)
