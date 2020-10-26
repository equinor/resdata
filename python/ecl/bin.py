import os
import sys


def _exec(prog):
    prog_path = os.path.abspath(os.path.join(os.path.dirname(__file__), ".bin", prog))
    os.execl(prog_path, prog_path, *sys.argv[1:])


def ecl_pack():
    _exec("ecl_pack.x")


def ecl_unpack():
    _exec("ecl_unpack.x")
