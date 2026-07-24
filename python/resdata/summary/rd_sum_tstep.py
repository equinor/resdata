from cwrap import BaseCClass

import resdata.summary._rd_sum_tstep as _rd_sum_tstep
from resdata.util.util import CTime


class SummaryTStep(BaseCClass):
    TYPE_NAME = "rd_sum_tstep"

    def __init__(
        self, report_step: int, mini_step: int, sim_days: float, smspec
    ) -> None:
        raise NotImplementedError()

    def get_sim_days(self) -> float:
        return _rd_sum_tstep._get_sim_days(self)

    def get_report(self) -> int:
        return _rd_sum_tstep._get_report(self)

    def get_mini_step(self) -> int:
        return _rd_sum_tstep._get_ministep(self)

    def get_sim_time(self) -> CTime:
        return CTime(_rd_sum_tstep._get_sim_time(self))

    def __getitem__(self, key: str) -> float:
        if key not in self:
            raise KeyError("Key '%s' is not available." % key)

        return _rd_sum_tstep._get_from_key(self, key)

    def __setitem__(self, key: str, value: float) -> None:
        if key not in self:
            raise KeyError("Key '%s' is not available." % key)

        _rd_sum_tstep._set_from_key(self, key, value)

    def __contains__(self, key: str) -> bool:
        return _rd_sum_tstep._has_key(self, key)

    def free(self) -> None:
        _rd_sum_tstep._free(self)

    def __repr__(self) -> str:
        d = self.get_sim_days()
        t = self.get_sim_time()
        r = self.get_report()
        m = self.get_mini_step()
        cnt = "sim_days={}, sim_time={}, report={}, ministep={}"
        return self._create_repr(cnt.format(d, t, r, m))
