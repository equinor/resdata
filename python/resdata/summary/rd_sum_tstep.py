from cwrap import BaseCClass

from resdata import ResdataPrototype
from resdata.util.util import CTime


class SummaryTStep(BaseCClass):
    TYPE_NAME = "rd_sum_tstep"
    _alloc = ResdataPrototype(
        "void* rd_sum_tstep_alloc_new(int, int, float, void*)", bind=False
    )
    _free = ResdataPrototype("void rd_sum_tstep_free(rd_sum_tstep)")
    _get_sim_days = ResdataPrototype("double rd_sum_tstep_get_sim_days(rd_sum_tstep)")
    _get_sim_time = ResdataPrototype(
        "rd_time_t rd_sum_tstep_get_sim_time(rd_sum_tstep)"
    )
    _get_report = ResdataPrototype("int rd_sum_tstep_get_report(rd_sum_tstep)")
    _get_ministep = ResdataPrototype("int rd_sum_tstep_get_ministep(rd_sum_tstep)")
    _set_from_key = ResdataPrototype(
        "void rd_sum_tstep_set_from_key(rd_sum_tstep, char*, float)"
    )
    _get_from_key = ResdataPrototype(
        "double rd_sum_tstep_get_from_key(rd_sum_tstep, char*)"
    )
    _has_key = ResdataPrototype("bool rd_sum_tstep_has_key(rd_sum_tstep, char*)")

    def __init__(
        self, report_step: int, mini_step: int, sim_days: float, smspec
    ) -> None:
        sim_seconds = sim_days * 24 * 60 * 60
        c_pointer = self._alloc(report_step, mini_step, sim_seconds, smspec)
        super().__init__(c_pointer)

    def get_sim_days(self) -> float:
        return self._get_sim_days()

    def get_report(self) -> int:
        return self._get_report()

    def get_mini_step(self) -> int:
        return self._get_ministep()

    def get_sim_time(self) -> CTime:
        return self._get_sim_time()

    def __getitem__(self, key: str) -> float:
        if key not in self:
            raise KeyError("Key '%s' is not available." % key)

        return self._get_from_key(key)

    def __setitem__(self, key: str, value: float) -> None:
        if key not in self:
            raise KeyError("Key '%s' is not available." % key)

        self._set_from_key(key, value)

    def __contains__(self, key: str) -> bool:
        return self._has_key(key)

    def free(self) -> None:
        self._free(self)

    def __repr__(self) -> str:
        d = self._get_sim_days()
        t = self._get_sim_time()
        r = self._get_report()
        m = self._get_ministep()
        cnt = "sim_days={}, sim_time={}, report={}, ministep={}"
        return self._create_repr(cnt.format(d, t, r, m))
