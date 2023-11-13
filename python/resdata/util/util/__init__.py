"""
Package with utility classes, used by other ERT classes.

The libutil library implements many utility functions and classes of
things like hash table and vector; these classes are extensively used
by the other ert libraries. The present wrapping here is to facilitate
use and interaction with various ert classes, in a pure python context
you are probably better served by using a plain python solution;
either based on built in python objects or well established third
party packages.

The modules included in the util package are:

  tvector.py: This module implements the classes IntVector,
     DoubleVector and BoolVector. This is a quite normal
     implementation of a typed growable vector; but with a special
     twist regarding default values.

  util_func.py: This module wraps a couple of stateless (i.e. there is
     no class involved) functions from the util.c file.

"""

from __future__ import absolute_import, division, print_function, unicode_literals

from .bool_vector import BoolVector
from .ctime import CTime
from .cwd_context import CWDContext
from .double_vector import DoubleVector
from .hash import DoubleHash, Hash, IntegerHash, StringHash
from .int_vector import IntVector
from .lookup_table import LookupTable
from .permutation_vector import PermutationVector
from .rng import RandomNumberGenerator
from .stringlist import StringList
from .thread_pool import ThreadPool
from .time_vector import TimeVector
from .vector_template import VectorTemplate

__all__ = [
    "BoolVector",
    "CTime",
    "CWDContext",
    "DoubleVector",
    "DoubleHash",
    "Hash",
    "IntegerHash",
    "StringHash",
    "installAbortSignals",
    "IntVector",
    "LookupTable",
    "PermutationVector",
    "RandomNumberGenerator",
    "StringList",
    "ThreadPool",
    "TimeVector",
    "VectorTemplate",
]
