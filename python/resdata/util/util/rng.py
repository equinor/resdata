import os.path

from cwrap import BaseCClass

import resdata.util.util._rng as _rng
from resdata.util.enums import RngAlgTypeEnum, RngInitModeEnum


class RandomNumberGenerator(BaseCClass):
    TYPE_NAME = "rd_rng"

    def __init__(
        self, alg_type=RngAlgTypeEnum.MZRAN, init_mode=RngInitModeEnum.INIT_CLOCK
    ):
        assert isinstance(alg_type, RngAlgTypeEnum)
        assert isinstance(init_mode, RngInitModeEnum)

        c_ptr = _rng._rng_alloc(int(alg_type), int(init_mode))
        super().__init__(c_ptr)

    def stateSize(self):
        return _rng._state_size(self)

    def setState(self, seed_string):
        state_size = self.stateSize()
        if len(seed_string) < state_size:
            raise ValueError(
                "The seed string must be at least %d characters long" % self.stateSize()
            )
        _rng._set_state(self, seed_string)

    def getDouble(self) -> float:
        return _rng._get_double(self)

    def getInt(self, maximum: int | None = None) -> float:
        if maximum is None:
            maximum = _rng._get_max_int(self)

        return _rng._get_int(self, maximum)

    def forward(self):
        return _rng._forward(self)

    def free(self):
        _rng._free(self)

    def loadState(self, seed_file):
        """
        Will seed the RNG from the file @seed_file.
        """
        if os.path.isfile(seed_file):
            _rng._load_state(self, seed_file)
        else:
            raise OSError("No such file: %s" % seed_file)

    def saveState(self, seed_file):
        """
        Will save the state of the rng to @seed_file
        """
        _rng._save_state(self, seed_file)
