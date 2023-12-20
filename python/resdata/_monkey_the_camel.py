import os
import warnings

###
###  monkey_the_camel is a function temporarily added to resdata while we are in
###  the process of changing camelCase function names to snake_case function
###  names.
###
###  See https://github.com/Equinor/resdata/issues/142 for a discussion and for
###  usage.
###


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
