"""
Package with utility classes, used by other ERT classes.

The libutil library implements many utility functions and classes of
things like hash table and vector; these classes are extensively used
by the other ert libraries. The present wrapping here is to facilitate
use and interaction with various ert classes, in a pure python context
you are probably better served by using a plain python solution;
either based on built in python objects or well established third
party packages.
"""

import warnings

from cwrap import Prototype

import resdata

from .ctime import CTime, TimeLike
from .install_abort_signals import installAbortSignals, updateAbortSignals
from .int_vector import IntVector
from .permutation_vector import PermutationVector
from .vector_template import VectorTemplate
