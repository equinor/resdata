import warnings

warnings.filterwarnings(
    action='always',
    category=DeprecationWarning,
    module=r'ecl|ert|res',
)

try:
    from .local import *
except ImportError:
    pass
