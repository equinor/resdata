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

import resdata
from cwrap import Prototype


from .version import Version, ResdataVersion

from resdata.util.enums import RngAlgTypeEnum, RngInitModeEnum

from .ctime import CTime

from .permutation_vector import PermutationVector
from .vector_template import VectorTemplate
from .double_vector import DoubleVector
from .int_vector import IntVector
from .bool_vector import BoolVector
from .time_vector import TimeVector
from .stringlist import StringList
from .rng import RandomNumberGenerator
from .lookup_table import LookupTable
from .hash import Hash, StringHash, DoubleHash, IntegerHash
from .thread_pool import ThreadPool
from .install_abort_signals import installAbortSignals, updateAbortSignals
from .cwd_context import CWDContext


###
###  monkey_the_camel is a function temporarily added to resdata while we are in
###  the process of changing camelCase function names to snake_case function
###  names.
###
###  See https://github.com/Equinor/resdata/issues/142 for a discussion and for
###  usage.
###

import os
import warnings

__cc = os.environ.get("RDWARNING", None)  # __cc in (None, 'user', 'dev', 'hard')


def __silencio(msg):
    pass


def __user_warning(msg):
    print("User warning: " + msg)


def __dev_warning(msg):
    warnings.warn(msg, DeprecationWarning)


def __hard_warning(msg):
    raise UserWarning("CamelCase exception: " + msg)


__rd_camel_case_warning = __silencio
if __cc == "user":
    __rd_camel_case_warning = __user_warning
elif __cc == "dev":
    __rd_camel_case_warning = __dev_warning
elif __cc == "hard":
    __rd_camel_case_warning = __hard_warning


def monkey_the_camel(class_, camel, method_, method_type=None):
    """Creates a method "class_.camel" in class_ which prints a warning and forwards
    to method_.  method_type should be one of (None, classmethod, staticmethod),
    and generates new methods accordingly.
    """

    def shift(*args):
        return args if (method_type != classmethod) else args[1:]

    def warned_method(*args, **kwargs):
        __rd_camel_case_warning(
            "Warning, %s is deprecated, use %s" % (camel, str(method_))
        )
        return method_(*shift(*args), **kwargs)

    if method_type == staticmethod:
        warned_method = staticmethod(warned_method)
    elif method_type == classmethod:
        warned_method = classmethod(warned_method)
    setattr(class_, camel, warned_method)
