from __future__ import absolute_import, division, print_function, unicode_literals

try:
    from StringIO import StringIO
except ImportError:
    from io import StringIO

import cProfile
import pstats
import sys

from ecl.util import monkey_the_camel

class Profiler(object):

    __profiler = None
    """ :type: Profile """

    @classmethod
    def start_profiler(cls, subcalls=True, builtins=True):
        cls.__profiler = cProfile.Profile()
        cls.__profiler.enable(subcalls=subcalls, builtins=builtins)

    @classmethod
    def stop_profiler(cls, sort_method="cumulative"):
        if cls.__profiler is not None:
            cls.__profiler.disable()
            stream = StringIO()
            stats_printer = pstats.Stats(cls.__profiler, stream=stream).sort_stats(sort_method)
            stats_printer.print_stats()
            cls.__profiler = None
            print(stream.getvalue())
        else:
            sys.stderr.write("WARNING: Profiler has not been started!\n")

monkey_the_camel(Profiler, 'startProfiler', Profiler.start_profiler, classmethod)
monkey_the_camel(Profiler, 'stopProfiler', Profiler.stop_profiler, classmethod)
