import datetime
import re

from resdata import ResdataPrototype
from resdata.util.util import VectorTemplate, CTime


class TimeVector(VectorTemplate):
    TYPE_NAME = "time_t_vector"
    default_format = "%d"

    _alloc = ResdataPrototype("void*   time_t_vector_alloc(int, time_t )", bind=False)
    _alloc_copy = ResdataPrototype(
        "time_t_vector_obj time_t_vector_alloc_copy(time_t_vector )"
    )
    _strided_copy = ResdataPrototype(
        "time_t_vector_obj time_t_vector_alloc_strided_copy(time_t_vector , int , int , int)"
    )
    _free = ResdataPrototype("void   time_t_vector_free( time_t_vector )")
    _iget = ResdataPrototype("time_t   time_t_vector_iget( time_t_vector , int )")
    _safe_iget = ResdataPrototype(
        "time_t   time_t_vector_safe_iget( time_t_vector , int )"
    )
    _iset = ResdataPrototype(
        "time_t   time_t_vector_iset( time_t_vector , int , time_t)"
    )
    _size = ResdataPrototype("int      time_t_vector_size( time_t_vector )")
    _append = ResdataPrototype(
        "void     time_t_vector_append( time_t_vector , time_t )"
    )
    _idel_block = ResdataPrototype(
        "void     time_t_vector_idel_block( time_t_vector , int , int )"
    )
    _idel = ResdataPrototype("void     time_t_vector_idel( time_t_vector , int )")
    _pop = ResdataPrototype("time_t   time_t_vector_pop( time_t_vector )")
    _lshift = ResdataPrototype("void     time_t_vector_lshift( time_t_vector , int )")
    _rshift = ResdataPrototype("void     time_t_vector_rshift( time_t_vector , int )")
    _insert = ResdataPrototype(
        "void     time_t_vector_insert( time_t_vector , int , time_t)"
    )
    _fprintf = ResdataPrototype(
        "void     time_t_vector_fprintf( time_t_vector , FILE , char* , char*)"
    )
    _sort = ResdataPrototype("void     time_t_vector_sort( time_t_vector )")
    _rsort = ResdataPrototype("void     time_t_vector_rsort( time_t_vector )")
    _reset = ResdataPrototype("void     time_t_vector_reset( time_t_vector )")
    _set_read_only = ResdataPrototype(
        "void     time_t_vector_set_read_only( time_t_vector , bool )"
    )
    _get_read_only = ResdataPrototype(
        "bool     time_t_vector_get_read_only( time_t_vector )"
    )
    _get_max = ResdataPrototype("time_t   time_t_vector_get_max( time_t_vector )")
    _get_min = ResdataPrototype("time_t   time_t_vector_get_min( time_t_vector )")
    _get_max_index = ResdataPrototype(
        "int      time_t_vector_get_max_index( time_t_vector , bool)"
    )
    _get_min_index = ResdataPrototype(
        "int      time_t_vector_get_min_index( time_t_vector , bool)"
    )
    _shift = ResdataPrototype("void     time_t_vector_shift( time_t_vector , time_t )")
    _scale = ResdataPrototype("void     time_t_vector_scale( time_t_vector , time_t )")
    _div = ResdataPrototype("void     time_t_vector_div( time_t_vector , time_t )")
    _inplace_add = ResdataPrototype(
        "void     time_t_vector_inplace_add( time_t_vector , time_t_vector )"
    )
    _inplace_mul = ResdataPrototype(
        "void     time_t_vector_inplace_mul( time_t_vector , time_t_vector )"
    )
    _assign = ResdataPrototype(
        "void     time_t_vector_set_all( time_t_vector , time_t)"
    )
    _memcpy = ResdataPrototype(
        "void     time_t_vector_memcpy(time_t_vector , time_t_vector )"
    )
    _set_default = ResdataPrototype(
        "void     time_t_vector_set_default( time_t_vector , time_t)"
    )
    _get_default = ResdataPrototype(
        "time_t   time_t_vector_get_default( time_t_vector )"
    )
    _element_size = ResdataPrototype(
        "int      time_t_vector_element_size( time_t_vector )"
    )

    _permute = ResdataPrototype(
        "void time_t_vector_permute(time_t_vector, permutation_vector)"
    )
    _sort_perm = ResdataPrototype(
        "permutation_vector_obj time_t_vector_alloc_sort_perm(time_t_vector)"
    )
    _rsort_perm = ResdataPrototype(
        "permutation_vector_obj time_t_vector_alloc_rsort_perm(time_t_vector)"
    )
    _contains = ResdataPrototype("bool time_t_vector_contains(time_t_vector, time_t)")
    _select_unique = ResdataPrototype("void time_t_vector_select_unique(time_t_vector)")
    _element_sum = ResdataPrototype("time_t time_t_vector_sum(time_t_vector)")
    _count_equal = ResdataPrototype(
        "int time_t_vector_count_equal(time_t_vector, time_t)"
    )
    _init_range = ResdataPrototype(
        "void time_t_vector_init_range(time_t_vector, time_t , time_t , time_t)"
    )
    _init_linear = ResdataPrototype(
        "bool time_t_vector_init_linear(time_t_vector, time_t, time_t, int)"
    )
    _equal = ResdataPrototype("bool time_t_vector_equal(time_t_vector, time_t_vector)")
    _first_eq = ResdataPrototype(
        "int time_t_vector_first_equal(time_t_vector, time_t_vector, int)"
    )
    _first_neq = ResdataPrototype(
        "int time_t_vector_first_not_equal(time_t_vector, time_t_vector, int)"
    )

    def __init__(self, default_value=None, initial_size=0):
        if default_value is None:
            super(TimeVector, self).__init__(CTime(0), initial_size)
        else:
            try:
                default = CTime(default_value)
            except:
                raise ValueError(
                    "default value invalid - must be type ctime() or date/datetime"
                )

            super(TimeVector, self).__init__(default, initial_size)

    @classmethod
    def parseTimeUnit(cls, deltaString):
        deltaRegexp = re.compile("(?P<num>\d*)(?P<unit>[dmy])", re.IGNORECASE)
        matchObj = deltaRegexp.match(deltaString)
        if matchObj:
            try:
                num = int(matchObj.group("num"))
            except:
                num = 1

            timeUnit = matchObj.group("unit").lower()
            return num, timeUnit
        else:
            raise TypeError(
                "The delta string must be on form '1d', '2m', 'Y' for one day, two months or one year respectively"
            )

    def __str__(self):
        """
        Returns string representantion of vector.
        """
        string_list = []
        for d in self:
            string_list.append("%s" % d)

        return str(string_list)

    def append(self, value):
        self._append(CTime(value))

    def __contains__(self, value):
        return self._contains(CTime(value))

    def nextTime(self, num, timeUnit):
        currentTime = self[-1].datetime()
        hour = currentTime.hour
        minute = currentTime.minute
        second = currentTime.second

        if timeUnit == "d":
            td = datetime.timedelta(days=num)
            currentTime += td
        else:
            day = currentTime.day
            month = currentTime.month
            year = currentTime.year

            if timeUnit == "y":
                year += num
            else:
                month += num - 1
                (deltaYear, newMonth) = divmod(month, 12)
                month = newMonth + 1
                year += deltaYear
            currentTime = datetime.datetime(year, month, day, hour, minute, second)

        return currentTime

    def appendTime(self, num, timeUnit):
        next = self.nextTime(num, timeUnit)
        self.append(CTime(next))

    @classmethod
    def createRegular(cls, start, end, deltaString):
        """
        The last element in the vector will be <= end; i.e. if the
        question of whether the range is closed in the upper end
        depends on the commensurability of the [start,end] interval
        and the delta:

        createRegular(0 , 10 , delta=3) => [0,3,6,9]
        createRegular(0 , 10 , delta=2) => [0,2,4,6,8,10]
        """
        start = CTime(start)
        end = CTime(end)
        if start > end:
            raise ValueError("The time interval is invalid start is after end")

        (num, timeUnit) = cls.parseTimeUnit(deltaString)

        timeVector = TimeVector()
        currentTime = start
        while currentTime <= end:
            ct = CTime(currentTime)
            timeVector.append(ct)
            currentTime = timeVector.nextTime(num, timeUnit)

        return timeVector

    def getDataPtr(self):
        raise NotImplementedError(
            "The getDataPtr() function is not implemented for time_t vectors"
        )
