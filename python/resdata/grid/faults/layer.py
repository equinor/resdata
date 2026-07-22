from cwrap import BaseCClass

import resdata.grid.faults._layer as _layer
from resdata.grid import Grid
from resdata.util.util import IntVector


class Layer(BaseCClass):
    TYPE_NAME = "rd_layer"

    def __init__(self, nx, ny):
        c_ptr = _layer._alloc(nx, ny)
        if c_ptr:
            super().__init__(c_ptr)
        else:
            raise ValueError("Invalid input - no Layer object created")

    def _assert_ij(self, i, j):
        if i < 0 or i >= self.get_nx():
            raise ValueError("Invalid layer i:%d" % i)

        if j < 0 or j >= self.get_ny():
            raise ValueError("Invalid layer j:%d" % j)

    def __unpack_index(self, index):
        try:
            i, j = index
        except TypeError:
            raise ValueError(
                "Index:%s is invalid - must have two integers" % str(index)
            )

        self._assert_ij(i, j)

        return (i, j)

    def __setitem__(self, index, value):
        i, j = self.__unpack_index(index)
        _layer._set_cell(self, i, j, value)

    def active_cell(self, i, j):
        self._assert_ij(i, j)
        return _layer._active_cell(self, i, j)

    def update_active(self, grid: Grid, k):
        if grid.get_nx() != self.get_nx():
            raise ValueError(
                "NX dimension mismatch. Grid:%d  layer:%d"
                % (grid.get_nx(), self.get_nx())
            )

        if grid.get_ny() != self.get_ny():
            raise ValueError(
                "NY dimension mismatch. Grid:%d  layer:%d"
                % (grid.get_ny(), self.get_ny())
            )

        if not (0 <= k < grid.get_nz()):
            raise ValueError("K value invalid: Grid range [0,%d)" % grid.get_nz())

        _layer._update_active(self, grid, k)

    def __getitem__(self, index):
        i, j = self.__unpack_index(index)
        return _layer._get_cell(self, i, j)

    def bottom_barrier(self, i, j):
        self._assert_ij(i, j)
        return _layer._get_bottom_barrier(self, i, j)

    def left_barrier(self, i, j):
        self._assert_ij(i, j)
        return _layer._get_left_barrier(self, i, j)

    def get_nx(self):
        return _layer._get_nx(self)

    @property
    def nx(self):
        return _layer._get_nx(self)

    def get_ny(self):
        return _layer._get_ny(self)

    @property
    def ny(self):
        return _layer._get_ny(self)

    def free(self):
        _layer._free(self)

    def cell_contact(self, p1, p2):
        i1, j1 = p1
        i2, j2 = p2

        if not 0 <= i1 < self.get_nx():
            raise IndexError(f"Invalid i1:{i1}")

        if not 0 <= i2 < self.get_nx():
            raise IndexError(f"Invalid i2:{i2}")

        if not 0 <= j1 < self.get_ny():
            raise IndexError(f"Invalid j1:{j1}")

        if not 0 <= j2 < self.get_ny():
            raise IndexError(f"Invalid j2:{j2}")

        return _layer._cell_contact(self, i1, j1, i2, j2)

    def add_interp_barrier(self, c1, c2):
        _layer._add_interp_barrier(self, c1, c2)

    def add_polyline_barrier(self, polyline, grid, k):
        if len(polyline) > 1:
            for i in range(len(polyline) - 1):
                x1, y1 = polyline[i]
                x2, y2 = polyline[i + 1]

                c1 = grid.find_cell_corner_xy(x1, y1, k)
                c2 = grid.find_cell_corner_xy(x2, y2, k)

                self.add_interp_barrier(c1, c2)

    def add_fault_barrier(self, fault, K, link_segments=True):
        fault_layer = fault[K]
        num_lines = len(fault_layer)
        for index, fault_line in enumerate(fault_layer):
            for segment in fault_line:
                c1, c2 = segment.get_corners()
                _layer._add_barrier(self, c1, c2)

            if index < num_lines - 1:
                next_line = fault_layer[index + 1]
                next_segment = next_line[0]
                next_c1, next_c2 = next_segment.get_corners()

                if link_segments:
                    self.add_interp_barrier(c2, next_c1)

    def add_ij_barrier(self, ij_list):
        if len(ij_list) < 2:
            raise ValueError("Must have at least two (i,j) points")

        nx = self.get_nx()
        ny = self.get_ny()
        p1 = ij_list[0]
        i1, j1 = p1
        for p2 in ij_list[1:]:
            i2, j2 = p2
            if i1 == i2 or j1 == j2:
                if not 0 <= i1 <= nx:
                    raise ValueError(f"i value:{i1} invalid. Valid range: [0,{nx}]")

                if not 0 <= i2 <= nx:
                    raise ValueError(f"i value:{i2} invalid. Valid range: [0,{nx}]")

                if not 0 <= j1 <= ny:
                    raise ValueError(f"j value:{j1} invalid. Valid range: [0,{ny}]")

                if not 0 <= j2 <= ny:
                    raise ValueError(f"j value:{j2} invalid. Valid range: [0,{ny}]")

                _layer._add_ijbarrier(self, i1, j1, i2, j2)
                p1 = p2
                i1, j1 = p1
            else:
                raise ValueError("Must have i1 == i2 or j1 == j2")

    def cell_sum(self):
        return _layer._cell_sum(self)

    def clear_cells(self):
        """
        Will reset all cell and edge values to zero. Barriers will be left
        unchanged.
        """
        _layer._clear_cells(self)

    def assign(self, value):
        """
        Will set the cell value to @value in all cells. Barriers will not be changed
        """
        _layer._assign(self, value)

    def update_connected(self, ij, new_value, org_value=None):
        """
        Will update cell value of all cells in contact with cell ij to the
        value @new_value. If org_value is not supplied, the current
        value in cell ij is used.
        """
        if org_value is None:
            org_value = self[ij]

        if self[ij] == org_value:
            _layer._update_connected(self, ij[0], ij[1], org_value, new_value)
        else:
            raise ValueError("Cell %s is not equal to %d \n" % (ij, org_value))

    def cells_equal(self, value):
        """
        Will return a list [(i1,j1),(i2,j2), ...(in,jn)] of all cells with value @value.
        """
        i_list = IntVector()
        j_list = IntVector()
        _layer._cells_equal(self, value, i_list, j_list)
        ij_list = []
        for i, j in zip(i_list, j_list):
            ij_list.append((i, j))
        return ij_list

    def count_equal(self, value):
        return _layer._count_equal(self, value)
