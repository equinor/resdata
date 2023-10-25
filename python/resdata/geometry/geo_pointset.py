from cwrap import BaseCClass
from resdata import ResdataPrototype


class GeoPointset(BaseCClass):
    TYPE_NAME = "geo_pointset"

    _alloc = ResdataPrototype("void*   geo_pointset_alloc(bool)", bind=False)
    _free = ResdataPrototype("void    geo_pointset_free(geo_pointset)")
    # _add_xyz    = ResdataPrototype("void    geo_pointset_add_xyz(geo_pointset, double, double, double)")
    _get_size = ResdataPrototype("int     geo_pointset_get_size(geo_pointset)")
    # _iget_xy    = ResdataPrototype("void    geo_pointset_iget_xy(geo_pointset, int, double*, double*)")
    # _get_zcoord = ResdataPrototype("double* geo_pointset_get_zcoord(geo_pointset)")
    _equal = ResdataPrototype("bool    geo_pointset_equal(geo_pointset, geo_pointset)")
    _iget_z = ResdataPrototype("double  geo_pointset_iget_z(geo_pointset, int)")
    # _iset_z     = ResdataPrototype("void    geo_pointset_iset_z(geo_pointset, int, double)")
    # _memcpy     = ResdataPrototype("void    geo_pointset_memcpy(geo_pointset, geo_pointset, bool)")
    # _shift_z    = ResdataPrototype("void    geo_pointset_shift_z(geo_pointset, double)")
    # _assign_z   = ResdataPrototype("void    geo_pointset_assign_z(geo_pointset, double)")
    # _scale_z    = ResdataPrototype("void    geo_pointset_scale_z(geo_pointset, double)")
    # _imul       = ResdataPrototype("void    geo_pointset_imul(geo_pointset, geo_pointset)")
    # _iadd       = ResdataPrototype("void    geo_pointset_iadd(geo_pointset, geo_pointset)")
    # _isub       = ResdataPrototype("void    geo_pointset_isub(geo_pointset, geo_pointset)")
    # _isqrt      = ResdataPrototype("void    geo_pointset_isqrt(geo_pointset)")

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
