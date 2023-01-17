import os.path

from cwrap import BaseCClass
from ecl import EclPrototype
from ecl.util.enums import RngInitModeEnum, RngAlgTypeEnum


class RandomNumberGenerator(BaseCClass):
    TYPE_NAME = "rng"

    _rng_alloc = EclPrototype(
        "void* rng_alloc(rng_alg_type_enum, rng_init_mode_enum)", bind=False
    )
    _free = EclPrototype("void rng_free(rng)")
    _get_double = EclPrototype("double rng_get_double(rng)")
    _get_int = EclPrototype("int rng_get_int(rng, int)")
    _forward = EclPrototype("uint rng_forward(rng)")
    _get_max_int = EclPrototype("uint rng_get_max_int(rng)")
    _state_size = EclPrototype("int rng_state_size(rng)")
    _set_state = EclPrototype("void rng_set_state(rng , char*)")
    _load_state = EclPrototype("void rng_load_state(rng , char*)")
    _save_state = EclPrototype("void rng_save_state(rng , char*)")

    def __init__(
        self, alg_type=RngAlgTypeEnum.MZRAN, init_mode=RngInitModeEnum.INIT_CLOCK
    ):
        assert isinstance(alg_type, RngAlgTypeEnum)
        assert isinstance(init_mode, RngInitModeEnum)

        c_ptr = self._rng_alloc(alg_type, init_mode)
        super(RandomNumberGenerator, self).__init__(c_ptr)

    def stateSize(self):
        return self._state_size()

    def setState(self, seed_string):
        state_size = self.stateSize()
        if len(seed_string) < state_size:
            raise ValueError(
                "The seed string must be at least %d characters long" % self.stateSize()
            )
        self._set_state(seed_string)

    def getDouble(self):
        """@rtype: float"""
        return self._get_double()

    def getInt(self, max=None):
        """@rtype: float"""
        if max is None:
            max = self._get_max_int()

        return self._get_int(max)

    def forward(self):
        return self._forward()

    def free(self):
        self._free()

    def loadState(self, seed_file):
        """
        Will seed the RNG from the file @seed_file.
        """
        if os.path.isfile(seed_file):
            self._load_state(seed_file)
        else:
            raise IOError("No such file: %s" % seed_file)

    def saveState(self, seed_file):
        """
        Will save the state of the rng to @seed_file
        """
        self._save_state(seed_file)
