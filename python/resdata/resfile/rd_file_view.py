from cwrap import BaseCClass
from six import string_types

import resdata.resfile._rd_file_view as _rd_file_view
from resdata.util.util import CTime, monkey_the_camel


class ResdataFileView(BaseCClass):
    TYPE_NAME = "rd_file_view"

    def __init__(self):
        raise NotImplementedError("Can not instantiate directly")

    def __iget(self, index):
        return _rd_file_view._iget_kw(self, index)

    def __repr__(self):
        return "ResdataFileView(size=%d) %s" % (len(self), self._ad_str())

    def iget_named_kw(self, kw_name, index):
        if kw_name not in self:
            raise KeyError("No such keyword: %s" % kw_name)

        if index >= self.num_keywords(kw_name):
            raise IndexError("Too large index: %d" % index)

        return _rd_file_view._iget_named_kw(self, kw_name, index)

    def __getitem__(self, index):
        """
        Implements [] operator; index can be integer or key.

        Will look up ResdataKW instances from the current ResdataFile
        instance. The @index argument can either be an integer, in
        which case the method will return ResdataKW number @index, or
        alternatively a keyword string, in which case the method will
        return a list of ResdataKW instances with that keyword:

           restart_file = rd_file.ResdataFile("CASE.UNRST")
           kw9 = restart_file[9]
           swat_list = restart_file["SWAT"]

        The keyword based lookup can be combined with an extra [] to
        get ResdataKW instance nr:

           swat9 = restart_file["SWAT"][9]

        Will return the 10'th SWAT keyword from the restart file. The
        following example will iterate over all the SWAT keywords in a
        restart file:

           restart_file = rd_file.ResdataFile("CASE.UNRST")
           for swat in restart_file["SWAT"]:
               ....
        """

        if isinstance(index, int):
            ls = len(self)
            idx = index
            if idx < 0:
                idx += ls
            if 0 <= idx < ls:
                return self.__iget(idx)
            else:
                raise IndexError("Index must be in [0, %d), was: %d." % (ls, index))

        if isinstance(index, slice):
            indices = index.indices(len(self))
            kw_list = []
            for i in range(*indices):
                kw_list.append(self[i])
            return kw_list
        else:
            if isinstance(index, bytes):
                index = index.decode("ascii")
            if isinstance(index, string_types):
                if index in self:
                    kw_index = index
                    kw_list = []
                    for index in range(self.num_keywords(kw_index)):
                        kw_list.append(self.iget_named_kw(kw_index, index))
                    return kw_list
                else:
                    raise KeyError("Unrecognized keyword:'%s'" % index)
            else:
                raise TypeError("Index must be integer or string (keyword)")

    def __len__(self):
        return _rd_file_view._get_size(self)

    def __contains__(self, kw):
        if self.num_keywords(kw) > 0:
            return True
        else:
            return False

    def num_keywords(self, kw):
        return _rd_file_view._get_num_named_kw(self, kw)

    def unique_size(self):
        return _rd_file_view._get_unique_size(self)

    def unique_kw(self):
        return [
            _rd_file_view._get_unique_kw(self, index)
            for index in range(self.unique_size())
        ]

    def block_view2(self, start_kw, stop_kw, start_index):
        idx = start_index
        if start_kw:
            if start_kw not in self:
                raise KeyError("The keyword:%s is not in file" % start_kw)

            ls = self.num_keywords(start_kw)
            if idx < 0:
                idx += ls
            if not (0 <= idx < ls):
                raise IndexError(
                    "Index must be in [0, %d), was: %d." % (ls, start_index)
                )

        if stop_kw:
            if stop_kw not in self:
                raise KeyError("The keyword:%s is not in file" % stop_kw)

        return _rd_file_view._create_block_view2(self, start_kw, stop_kw, idx)

    def block_view(self, kw, kw_index):
        num = self.num_keywords(kw)

        if num == 0:
            raise KeyError("Unknown keyword: %s" % kw)

        idx = kw_index
        if idx < 0:
            idx += num

        if not (0 <= idx < num):
            raise IndexError("Index must be in [0, %d), was: %d." % (num, kw_index))

        return _rd_file_view._create_block_view(self, kw, kw_index)

    def restart_view(
        self, seqnum_index=None, report_step=None, sim_time=None, sim_days=None
    ):
        if report_step is None:
            report_step = -1

        if sim_time is None:
            sim_time = -1

        if sim_days is None:
            sim_days = -1

        if seqnum_index is None:
            seqnum_index = -1

        view = _rd_file_view._restart_view(
            self, seqnum_index, report_step, CTime(sim_time).value(), sim_days
        )
        if view is None:
            raise ValueError("No such restart block could be identified")

        return view


monkey_the_camel(ResdataFileView, "numKeywords", ResdataFileView.num_keywords)
monkey_the_camel(ResdataFileView, "uniqueSize", ResdataFileView.unique_size)
monkey_the_camel(ResdataFileView, "blockView2", ResdataFileView.block_view2)
monkey_the_camel(ResdataFileView, "blockView", ResdataFileView.block_view)
monkey_the_camel(ResdataFileView, "restartView", ResdataFileView.restart_view)
