from cwrap import BaseCClass
from resdata import ResdataPrototype


class GeoPointset(BaseCClass):
    TYPE_NAME = "rd_geo_points"

    _alloc = ResdataPrototype("void* geo_pointset_alloc(bool)", bind=False)
    _free = ResdataPrototype("void geo_pointset_free(rd_geo_points)")
    _get_size = ResdataPrototype("int geo_pointset_get_size(rd_geo_points)")
    _equal = ResdataPrototype("bool geo_pointset_equal(rd_geo_points, rd_geo_points)")
    _iget_z = ResdataPrototype("double geo_pointset_iget_z(rd_geo_points, int)")

    def __init__(self, external_z=False):
        c_ptr = self._alloc(external_z)
        if c_ptr:
            super(GeoPointset, self).__init__(c_ptr)
        else:
            ext = "external" if external_z else "internal"
            raise ValueError("Failed to construct GeoPointset with %s_z." % ext)

    @staticmethod
    def fromSurface(surface):
        return surface.getPointset()

    def __eq__(self, other):
        if isinstance(other, GeoPointset):
            return self._equal(other)
        return NotImplemented

    def __getitem__(self, key):
        size = len(self)
        if isinstance(key, int):
            idx = key
            if idx < 0:
                idx += size
            if 0 <= idx < size:
                return self._iget_z(idx)
            else:
                raise IndexError(
                    "Invalid index, must be in [0, %d), was: %d." % (size, key)
                )
        else:
            # TODO implement slicing?
            raise ValueError("Index must be int, not %s." % type(key))

    def __len__(self):
        return self._get_size()

    def __repr__(self):
        return self._create_repr("len=%d" % len(self))

    def free(self):
        self._free()
