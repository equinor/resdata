import os.path

from cwrap import BaseCClass
from resdata import ResdataPrototype
from resdata.util.enums import RngInitModeEnum, RngAlgTypeEnum


class RandomNumberGenerator(BaseCClass):
    TYPE_NAME = "rd_rng"

    _rng_alloc = ResdataPrototype(
        "void* rng_alloc(rd_rng_alg_type_enum, rd_rng_init_mode)", bind=False
    )
    _free = ResdataPrototype("void rng_free(rd_rng)")
    _get_double = ResdataPrototype("double rng_get_double(rd_rng)")
    _get_int = ResdataPrototype("int rng_get_int(rd_rng, int)")
    _forward = ResdataPrototype("uint rng_forward(rd_rng)")
    _get_max_int = ResdataPrototype("uint rng_get_max_int(rd_rng)")
    _state_size = ResdataPrototype("int rng_state_size(rd_rng)")
    _set_state = ResdataPrototype("void rng_set_state(rd_rng , char*)")
    _load_state = ResdataPrototype("void rng_load_state(rd_rng , char*)")
    _save_state = ResdataPrototype("void rng_save_state(rd_rng , char*)")

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
