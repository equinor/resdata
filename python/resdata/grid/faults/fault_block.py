import ctypes
from cwrap import BaseCClass

from resdata.util.util import monkey_the_camel
from resdata.util.util import DoubleVector, IntVector
from resdata import ResdataPrototype
from resdata.geometry import Polyline, GeometryTools, CPolylineCollection


class FaultBlockCell(object):
    def __init__(self, i, j, k, x, y, z):
        self.i = i
        self.j = j
        self.k = k

        self.x = x
        self.y = y
        self.z = z

    def __str__(self):
        return "(%d,%d)" % (self.i, self.j)


class FaultBlock(BaseCClass):
    TYPE_NAME = "rd_fault_block"

    _get_xc = ResdataPrototype("double fault_block_get_xc(rd_fault_block)")
    _get_yc = ResdataPrototype("double fault_block_get_yc(rd_fault_block)")
    _get_block_id = ResdataPrototype("int fault_block_get_id(rd_fault_block)")
    _get_size = ResdataPrototype("int fault_block_get_size(rd_fault_block)")
    _export_cell = ResdataPrototype(
        "void fault_block_export_cell(rd_fault_block, int, int*, int*, int*, double*, double*, double*)"
    )
    _assign_to_region = ResdataPrototype(
        "void fault_block_assign_to_region(rd_fault_block, int)"
    )
    _get_region_list = ResdataPrototype(
        "rd_int_vector_ref fault_block_get_region_list(rd_fault_block)"
    )
    _add_cell = ResdataPrototype("void fault_block_add_cell(rd_fault_block,  int, int)")
    _get_global_index_list = ResdataPrototype(
        "rd_int_vector_ref fault_block_get_global_index_list(rd_fault_block)"
    )
    _trace_edge = ResdataPrototype(
        "void fault_block_trace_edge(rd_fault_block, rd_double_vector, rd_double_vector, rd_int_vector)"
    )
    _get_neighbours = ResdataPrototype(
        "void fault_block_list_neighbours(rd_fault_block, bool, rd_geo_polygon_collection, rd_int_vector)"
    )
    _free = ResdataPrototype("void fault_block_free__(rd_fault_block)")

    def __init__(self, *args, **kwargs):
        raise NotImplementedError("Class can not be instantiated directly!")

    def __getitem__(self, index):
        if isinstance(index, int):
            if index < 0:
                index += len(self)

            if 0 <= index < len(self):
                x = ctypes.c_double()
                y = ctypes.c_double()
                z = ctypes.c_double()

                i = ctypes.c_int()
                j = ctypes.c_int()
                k = ctypes.c_int()

                self._export_cell(
                    index,
                    ctypes.byref(i),
                    ctypes.byref(j),
                    ctypes.byref(k),
                    ctypes.byref(x),
                    ctypes.byref(y),
                    ctypes.byref(z),
                )
                return FaultBlockCell(
                    i.value, j.value, k.value, x.value, y.value, z.value
                )
            else:
                raise IndexError("Index:%d out of range: [0,%d)" % (index, len(self)))
        else:
            raise TypeError("Index:%s wrong type - integer expected")

    def __str__(self):
        return "Block ID: %d" % self.getBlockID()

    def __len__(self):
        return self._get_size()

    def free(self):
        self._free()

    def get_centroid(self):
        xc = self._get_xc()
        yc = self._get_yc()
        return (xc, yc)

    def count_inside(self, polygon):
        """
        Will count the number of points in block which are inside polygon.
        """
        inside = 0
        for p in self:
            if GeometryTools.pointInPolygon((p.x, p.y), polygon):
                inside += 1

        return inside

    def get_block_id(self):
        return self._get_block_id()

    def assign_to_region(self, region_id):
        self._assign_to_region(region_id)

    def get_region_list(self):
        regionList = self._get_region_list()
        return regionList.copy()

    def add_cell(self, i, j):
        self._add_cell(i, j)

    def get_global_index_list(self):
        return self._get_global_index_list()

    def get_edge_polygon(self):
        x_list = DoubleVector()
        y_list = DoubleVector()
        cell_list = IntVector()

        self._trace_edge(x_list, y_list, cell_list)
        p = Polyline()
        for x, y in zip(x_list, y_list):
            p.addPoint(x, y)
        return p

    def contains_polyline(self, polyline):
        """
        Will return true if at least one point from the polyline is inside the block.
        """
        edge_polyline = self.getEdgePolygon()
        for p in polyline:
            if GeometryTools.pointInPolygon(p, edge_polyline):
                return True
        else:
            edge_polyline.assertClosed()
            return GeometryTools.polylinesIntersect(edge_polyline, polyline)

    def get_neighbours(self, polylines=None, connected_only=True):
        """
        Will return a list of FaultBlock instances which are in direct
        contact with this block.
        """
        neighbour_id_list = IntVector()
        if polylines is None:
            polylines = CPolylineCollection()

        self._get_neighbours(connected_only, polylines, neighbour_id_list)

        parent_layer = self.getParentLayer()
        neighbour_list = []
        for id in neighbour_id_list:
            neighbour_list.append(parent_layer.getBlock(id))
        return neighbour_list

    def get_parent_layer(self):
        return self.parent()


monkey_the_camel(FaultBlock, "getCentroid", FaultBlock.get_centroid)
monkey_the_camel(FaultBlock, "countInside", FaultBlock.count_inside)
monkey_the_camel(FaultBlock, "getBlockID", FaultBlock.get_block_id)
monkey_the_camel(FaultBlock, "assignToRegion", FaultBlock.assign_to_region)
monkey_the_camel(FaultBlock, "getRegionList", FaultBlock.get_region_list)
monkey_the_camel(FaultBlock, "addCell", FaultBlock.add_cell)
monkey_the_camel(FaultBlock, "getGlobalIndexList", FaultBlock.get_global_index_list)
monkey_the_camel(FaultBlock, "getEdgePolygon", FaultBlock.get_edge_polygon)
monkey_the_camel(FaultBlock, "containsPolyline", FaultBlock.contains_polyline)
monkey_the_camel(FaultBlock, "getNeighbours", FaultBlock.get_neighbours)
monkey_the_camel(FaultBlock, "getParentLayer", FaultBlock.get_parent_layer)
