import os
import sys


def main():
    prog = os.path.basename(sys.argv[0])
    path = os.path.abspath(os.path.join(os.path.dirname(__file__), ".bin", prog))
    os.execl(path, path, *sys.argv[1:])
