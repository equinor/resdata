import ctypes

from cwrap import BaseCClass
from resdata import ResdataPrototype

import weakref


class CThreadPool(BaseCClass):
    TYPE_NAME = "rd_thread_pool"

    _alloc = ResdataPrototype("void* thread_pool_alloc(int, bool)", bind=False)
    _free = ResdataPrototype("void thread_pool_free(rd_thread_pool)")
    _add_job = ResdataPrototype(
        "void thread_pool_add_job(rd_thread_pool, void*, void*)"
    )
    _join = ResdataPrototype("void thread_pool_join(rd_thread_pool)")

    def __init__(self, pool_size, start=True):
        c_ptr = self._alloc(pool_size, start)
        super(CThreadPool, self).__init__(c_ptr)
        self.arg_list = []

    def addTaskFunction(self, name, lib, c_function_name):
        function = CThreadPool.lookupCFunction(lib, c_function_name)
        self_ref = weakref.ref(self)  # avoid circular dependencies

        def wrappedFunction(arg):
            return self_ref().addTask(function, arg)

        setattr(self, name, wrappedFunction)

    def addTask(self, cfunc, arg):
        """
        The function should come from CThreadPool.lookupCFunction().
        """
        if isinstance(arg, BaseCClass):
            arg_ptr = BaseCClass.from_param(arg)
        else:
            arg_ptr = arg

        self.arg_list.append(arg)
        self._add_job(cfunc, arg_ptr)

    def join(self):
        self._join()

    def free(self):
        self.join()
        self._free()

    @staticmethod
    def lookupCFunction(lib, name):
        if isinstance(lib, ctypes.CDLL):
            func = getattr(lib, name)
            return func
        else:
            raise TypeError("The lib argument must be of type ctypes.CDLL")


class CThreadPoolContextManager(object):
    def __init__(self, tp):
        self.__tp = tp

    def __enter__(self):
        return self.__tp

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.__tp.join()
        return False


def startCThreadPool(size):
    return CThreadPoolContextManager(CThreadPool(size, start=True))
