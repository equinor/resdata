from cwrap import BaseCClass

import resdata.geometry._geo_pointset as _geo_pointset


class GeoPointset(BaseCClass):
    TYPE_NAME = "rd_geo_points"

    def __init__(self, external_z=False):
        c_ptr = _geo_pointset._alloc(external_z)
        if c_ptr:
            super().__init__(c_ptr)
        else:
            ext = "external" if external_z else "internal"
            raise ValueError("Failed to construct GeoPointset with %s_z." % ext)

    @staticmethod
    def fromSurface(surface):
        return surface.getPointset()

    def __eq__(self, other):
        if isinstance(other, GeoPointset):
            return _geo_pointset._equal(self, other)
        return NotImplemented

    def __getitem__(self, key):
        size = len(self)
        if isinstance(key, int):
            idx = key
            if idx < 0:
                idx += size
            if 0 <= idx < size:
                return _geo_pointset._iget_z(self, idx)
            else:
                raise IndexError(
                    "Invalid index, must be in [0, %d), was: %d." % (size, key)
                )
        else:
            # TODO implement slicing?
            raise ValueError("Index must be int, not %s." % type(key))

    def __len__(self):
        return _geo_pointset._get_size(self)

    def __repr__(self):
        return self._create_repr("len=%d" % len(self))

    def free(self):
        _geo_pointset._free(self)
