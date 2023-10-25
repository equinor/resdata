import re

from resdata.util.util import monkey_the_camel
from resdata.grid import Grid
from .fault import Fault

comment_regexp = re.compile("--.*")


def dequote(s):
    if s[0] in ["'", '"']:
        if s[0] == s[-1]:
            return s[1:-1]
        else:
            raise ValueError("s[0] != s[-1], s[0]={}, s[-1]={}".format(s[0], s[-1]))
    else:
        return s


class FaultCollection(object):
    def __init__(self, grid=None, *file_list):
        self.__fault_list = []
        self.__fault_map = {}
        self.__grid = grid

        if self.__grid is not None:
            if not isinstance(self.__grid, Grid):
                raise ValueError(
                    "When supplying a list of files to load - you must have a grid"
                )
            for file in file_list:
                self.load(self.__grid, file)

    def __contains__(self, fault_name):
        return fault_name in self.__fault_map

    def __len__(self):
        return len(self.__fault_list)

    def __getitem__(self, index):
        if isinstance(index, str):
            return self.__fault_map[index]
        elif isinstance(index, int):
            return self.__fault_list[index]
        else:
            raise TypeError("Argument must be fault name or number")

    def __iter__(self):
        return iter(self.__fault_list)

    def get_grid(self):
        return self.__grid

    def get_fault(self, name):
        return self[name]

    def has_fault(self, fault_name):
        return fault_name in self

    def add_fault(self, fault):
        self.__fault_map[fault.getName()] = fault
        self.__fault_list.append(fault)

    def split_line(self, line):
        tmp = line.split()
        if not tmp[-1] == "/":
            raise ValueError("Line:%s does not end with /" % line)

        if len(tmp) != 9:
            raise ValueError("Line:%s not correct number of items" % line)

        fault_name = dequote(tmp[0])
        I1 = int(tmp[1]) - 1
        I2 = int(tmp[2]) - 1
        J1 = int(tmp[3]) - 1
        J2 = int(tmp[4]) - 1
        K1 = int(tmp[5]) - 1
        K2 = int(tmp[6]) - 1
        face = dequote(tmp[7])

        return (fault_name, I1, I2, J1, J2, K1, K2, face)

    def load_faults(self, grid, fileH):
        for line in fileH:
            line = comment_regexp.sub("", line)
            line = line.strip()
            if line == "/":
                break

            if line:
                (name, I1, I2, J1, J2, K1, K2, face) = self.splitLine(line)
                if not self.hasFault(name):
                    fault = Fault(grid, name)
                    self.addFault(fault)
                else:
                    fault = self.getFault(name)

                fault.addRecord(I1, I2, J1, J2, K1, K2, face)

    def load(self, grid, file_name):
        with open(file_name) as fileH:
            for line in fileH:
                if line.startswith("FAULTS"):
                    self.loadFaults(grid, fileH)


monkey_the_camel(FaultCollection, "getGrid", FaultCollection.get_grid)
monkey_the_camel(FaultCollection, "getFault", FaultCollection.get_fault)
monkey_the_camel(FaultCollection, "hasFault", FaultCollection.has_fault)
monkey_the_camel(FaultCollection, "addFault", FaultCollection.add_fault)
monkey_the_camel(FaultCollection, "splitLine", FaultCollection.split_line)
monkey_the_camel(FaultCollection, "loadFaults", FaultCollection.load_faults)
