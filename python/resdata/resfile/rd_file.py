"""
The rd_file module contains functionality to load a file
in 'restart format'. Files of 'restart format' include restart files,
init files, grid files, summary files and RFT files.

The rd_file implementation is agnostic[1] to the content and
structure of the file; more specialized classes like Summary and
Grid use the ResdataFile functionality for low level file loading.

The typical usage involves loading a complete file, and then
subsequently querying for various keywords. In the example below we
load a restart file, and ask for the SWAT keyword:

   file = ResdataFile( "CASE.X0067" )
   swat_kw = file.iget_named_kw( "SWAT" , 0 )

The rd_file module is a thin wrapper around the rd_file.c
implementation from the resdata library.

[1]: In particular for restart files, which do not have a special
     RestartFile class, there is some specialized functionality.
"""

import datetime
import re

from cwrap import BaseCClass

import resdata.resfile._file as _file
from resdata.rd_util import FileMode, FileType, ResdataUtil
from resdata.util.util import CTime

from .rd_file_view import ResdataFileView
from .rd_kw import ResdataKW


class ResdataFile(BaseCClass):
    TYPE_NAME = "rd_file"

    @staticmethod
    def get_filetype(filename: str) -> tuple[FileType, bool | None, int | None]:
        file_type, fmt_file, report_step = ResdataUtil.inspect_extension(filename)
        if file_type not in [
            FileType.RESTART,
            FileType.SUMMARY,
        ]:
            report_step = None

        if file_type in [FileType.OTHER, FileType.DATA]:
            fmt_file = None

        return (file_type, report_step, fmt_file)

    @classmethod
    def contains_report_step(cls, filename, report_step):
        """
        Will check if the @filename contains @report_step.

        This classmethod works by opening the file @filename and
        searching through linearly to see if an rd_kw with value
        corresponding to @report_step can be found.

        If you have already loaded the file into an ResdataFile instance
        you should use the has_report_step() method instead.
        """
        obj = ResdataFile(filename)
        return obj.has_report_step(report_step)

    @classmethod
    def contains_sim_time(cls, filename, dtime):
        """
        Will check if the @filename contains simulation at @dtime.

        This classmethod works by opening the file @filename and
        searching through linearly to see if a result block at the
        time corresponding to @dtime can be found.

        If you have already loaded the file into an ResdataFile instance
        you should use the has_sim_time() method instead.
        """
        obj = ResdataFile(filename)
        return obj.has_sim_time(dtime)

    @property
    def report_list(self):
        report_steps = []
        try:
            seqnum_list = self["SEQNUM"]
            for s in seqnum_list:
                report_steps.append(s[0])
        except KeyError:
            # OK - we did not have seqnum; that might be because this
            # a non-unified restart file; or because this is not a
            # restart file at all.
            fname = self.get_filename()
            matchObj = re.search(r"\.[XF](\d{4})$", fname)
            if matchObj:
                report_steps.append(int(matchObj.group(1)))
            else:
                raise TypeError(
                    'Tried get list of report steps from file "%s" - which is not a restart file'
                    % fname
                )

        return report_steps

    @classmethod
    def file_report_list(cls, filename):
        """
        Will identify the available report_steps from @filename.
        """

        file = ResdataFile(filename)
        return file.report_list

    def __repr__(self):
        fn = self.get_filename()
        wr = ", read/write" if _file._writable(self) else ""
        return self._create_repr('"%s"%s' % (fn, wr))

    def __init__(self, filename, flags=FileMode.DEFAULT, index_filename=None):
        """
        Loads the complete file @filename.

        Will create a new ResdataFile instance with the content of file
        @filename. The file @filename must be in 'restart format' -
        otherwise it will be crash and burn.

        The optional argument flags can be an or'ed combination of the
        flags:

           WRITABLE : It is possible to update the
              content of the keywords in the file.

           CLOSE_STREAM : The underlying FILE * is closed
              when not used; to save number of open file descriptors
              in cases where a high number of ResdataFile instances are
              open concurrently.

        When the file has been loaded the ResdataFile instance can be used
        to query for and get reference to the ResdataKW instances
        constituting the file, like e.g. SWAT from a restart file or
        FIPNUM from an INIT file.
        """
        if index_filename is None:
            c_ptr = _file._open(filename, flags)
        else:
            c_ptr = _file._fast_open(filename, index_filename, flags)

        super().__init__(c_ptr)
        self.global_view: ResdataFileView = _file._get_global_view(self)

    def save_kw(self, kw):
        """
        Will write the @kw back to file.

        This function should typically be used in situations like this:

          1. Create an ResdataFile instance around a restart format file.
          2. Extract a keyword of interest and modify it.
          3. Call this method to save the modifications to disk.

        There are several restrictions to the use of this function:

          1. The ResdataFile instance must have been created with the
             optional read_only flag set to False.

          2. You can only modify the content of the keyword; if you
             try to modify the header in any way (i.e. size, datatype
             or name) the function will fail.

          3. The keyword you are trying to save must be exactly the
             keyword you got from this ResdataFile instance, otherwise the
             function will fail.
        """
        if _file._writable(self):
            _file._save_kw(self, kw)
        else:
            raise OSError(
                'save_kw: the file "%s" has been opened read only.'
                % self.get_filename()
            )

    def __len__(self):
        return len(self.global_view)

    def close(self):
        """Closes the file handle used to read data.

        There are two caveats:

          1. Stale cached ResdataKW instances: A ResdataKW that was already
             read before ``close()`` keeps the data it held at that time. It is
             a snapshot: it is not refreshed and will not reflect any later
             modifications made to the file.

          2. Lazy re-opening of ResdataFileView: Keyword data is loaded lazily,
             so a keyword that was *not* yet read when ``close()`` was called is
             read on first access. Accessing such a keyword through a
             ResdataFileView (or through this ResdataFile)
             re-opens the file on disk to read it.
        """
        _file._close(self)

    def free(self):
        _file._free(self)

    def block_view(self, kw: str, kw_index: int) -> ResdataFileView:
        """A view of the keyword block delimited by ``kw``.

        The returned :class:`ResdataFileView` contains the keywords from the
        ``kw_index``'th occurrence of ``kw`` (inclusive) up to, but not
        including, the next occurrence of ``kw``. In other words the file is
        treated as a sequence of blocks that each start with ``kw``, and this
        method returns one such block::

            # File: HEADER DATA1 DATA2 HEADER DATA1 DATA2
            view = rd_file.block_view("HEADER", 1)
            # view contains: HEADER DATA1 DATA2   (the second block)

        The last block extends to the end of the file.

        ``kw_index`` selects which occurrence of ``kw`` starts the block. A
        negative index counts from the end, so ``-1`` is the last occurrence.

        :param kw: The keyword that delimits the blocks.
        :param kw_index: Which occurrence of ``kw`` to start the block at.
        :raises KeyError: If ``kw`` is not present in the file.
        :raises IndexError: If ``kw_index`` is out of range.
        """
        if kw not in self:
            raise KeyError('No such keyword "%s".' % kw)
        ls = self.global_view.num_keywords(kw)
        idx = kw_index
        if idx < 0:
            idx += ls
        if 0 <= idx < ls:
            return self.global_view.block_view(kw, idx)
        raise IndexError(
            "Index out of range, must be in [0, %d), was %d." % (ls, kw_index)
        )

    def block_view2(
        self, start_kw: str | None, stop_kw: str | None, start_index: int
    ) -> ResdataFileView:
        """Return a view of the keywords between ``start_kw`` and ``stop_kw``.

        The returned :class:`ResdataFileView` starts at the ``start_index``'th
        occurrence of ``start_kw`` (inclusive) and ends just before the first
        occurrence of ``stop_kw`` that follows it (exclusive)::

            # File: SEQNUM PRESSURE SWAT PRESSURE SWAT
            view = rd_file.block_view2("SEQNUM", "PRESSURE", 0)
            # view contains: SEQNUM   (up to, but not including, PRESSURE)

        ``start_kw`` and ``stop_kw`` may be ``None``:

        * If ``start_kw`` is ``None`` the view starts at the first keyword in
          the file and ``start_index`` is ignored.
        * If ``stop_kw`` is ``None`` the view extends to the end of the file.

        So ``block_view2(None, None, 0)`` returns a view containing every
        keyword in the file, in order::

            # File: SEQNUM PRESSURE SWAT PRESSURE SWAT
            view = rd_file.block_view2(None, None, 0)
            # view contains: SEQNUM PRESSURE SWAT PRESSURE SWAT

        ``start_index`` selects which occurrence of ``start_kw`` to start at. A
        negative index counts from the end, so ``-1`` is the last occurrence.

        :param start_kw: The keyword to start at, or ``None`` for the start of
            the file.
        :param stop_kw: The keyword to stop before, or ``None`` for the end of
            the file.
        :param start_index: Which occurrence of ``start_kw`` to start at.
        :raises KeyError: If ``start_kw`` or ``stop_kw`` is given but not
            present in the file.
        :raises IndexError: If ``start_index`` is out of range.
        """
        return self.global_view.block_view2(start_kw, stop_kw, start_index)

    def restart_view(
        self, seqnum_index=None, report_step=None, sim_time=None, sim_days=None
    ):
        return self.global_view.restart_view(
            seqnum_index, report_step, sim_time, sim_days
        )

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
                return self.global_view[idx]
            else:
                raise IndexError("Index must be in [0, %d), was: %d." % (ls, index))
        return self.global_view[index]

    def iget_kw(self, index, copy=False):
        """
        Will return ResdataKW instance nr @index.

        In the files loaded with the ResdataFile implementation the
        keywords come sequentially in a long series, an INIT
        file might have the following keywords:

          INTEHEAD
          LOGIHEAD
          DOUBHEAD
          PORV
          DX
          DY
          DZ
          PERMX
          PERMY
          PERMZ
          MULTX
          MULTY
          .....

        The iget_kw() method will give you a ResdataKW reference to
        keyword nr @index. This functionality is also available
        through the index operator []:

           file = ResdataFile( "CASE.INIT" )
           permx = file.iget_kw( 7 )
           permz = file[ 9 ]

        Observe that the returned ResdataKW instance is only a reference
        to the data owned by the ResdataFile instance.

        The method iget_named_kw() which lets you specify the name of
        the keyword you are interested in is in general more useful
        than this method.
        """
        kw = self[index]
        if copy:
            return ResdataKW.copy(kw)
        else:
            return kw

    def iget_named_kw(self, kw_name, index, copy=False):
        return self.global_view.iget_named_kw(kw_name, index)

    def restart_get_kw(self, kw_name, dtime, copy=False):
        """Will return ResdataKW @kw_name from restart file at time @dtime.

        This function assumes that the current ResdataFile instance
        represents a restart file. It will then look for keyword
        @kw_name exactly at the time @dtime; @dtime is a datetime
        instance:

            file = ResdataFile( "CASE.UNRST" )
            swat2010 = file.restart_get_kw( "SWAT" , datetime.datetime( 2000 , 1 , 1 ))

        By default the returned kw instance is a reference to the
        rd_kw still contained in the ResdataFile instance; i.e. the kw
        will become a dangling reference if the ResdataFile instance goes
        out of scope. If the optional argument @copy is True the
        returned kw will be a true copy.

        If the file does not have the keyword at the specified time
        the function will raise IndexError(); if the file does not
        have the keyword at all - KeyError will be raised.
        """
        index = _file._get_restart_index(self, CTime(dtime).value())
        if index >= 0:
            if self.num_named_kw(kw_name) > index:
                kw = self.iget_named_kw(kw_name, index)
                if copy:
                    return ResdataKW.copy(kw)
                else:
                    return kw
            else:
                if self.has_kw(kw_name):
                    raise IndexError(
                        'Does not have keyword "%s" at time:%s.' % (kw_name, dtime)
                    )
                else:
                    raise KeyError('Keyword "%s" not recognized.' % kw_name)
        else:
            raise IndexError(
                'Does not have keyword "%s" at time:%s.' % (kw_name, dtime)
            )

    @property
    def size(self):
        """
        The number of keywords in the current ResdataFile object.
        """
        return len(self)

    @property
    def unique_size(self):
        """
        The number of unique keyword (names) in the current ResdataFile object.
        """
        return self.global_view.unique_size()

    def keys(self):
        """
        Will return a list of unique kw names - like keys() on a dict.
        """
        header_dict = {}
        for index in range(len(self)):
            kw = self[index]
            header_dict[kw.get_name()] = True
        return header_dict.keys()

    @property
    def headers(self):
        """
        Will return a list of the headers of all the keywords.
        """
        header_list = []
        for index in range(self.size):
            kw = self[index]
            header_list.append(kw.header)
        return header_list

    @property
    def report_steps(self):
        """
        Will return a list of all report steps.

        The method works by iterating through the whole restart file
        looking for 'SEQNUM' keywords; if the current ResdataFile instance
        is not a restart file it will not contain any 'SEQNUM'
        keywords and the method will simply return an empty list.
        """
        steps = []
        for kw in self["SEQNUM"]:
            steps.append(kw[0])
        return steps

    @property
    def report_dates(self):
        """
        Will return a list of the dates for all report steps.

        The method works by iterating through the whole restart file
        looking for 'SEQNUM/INTEHEAD' keywords; the method can
        probably be tricked by other file types also containing an
        INTEHEAD keyword.
        """
        if self.has_kw("SEQNUM"):
            dates = []
            for index in range(self.num_named_kw("SEQNUM")):
                dates.append(self.iget_restart_sim_time(index))
            return dates
        elif "INTEHEAD" in self:
            # This is a workaround; consider exporting the rd_rsthead
            # object as a ctypes structure.
            intehead = self["INTEHEAD"][0]
            year = intehead[66]
            month = intehead[65]
            day = intehead[64]
            date = datetime.datetime(year, month, day)
            return [date]
        return None

    @property
    def dates(self):
        """
        Will return a list of the dates for all report steps.
        """
        return self.report_dates

    def num_named_kw(self, kw):
        """
        The number of keywords with name == @kw in the current ResdataFile object.
        """
        return self.global_view.num_keywords(kw)

    def has_kw(self, kw, num=0):
        """
        Check if current ResdataFile instance has a keyword @kw.

        If the optional argument @num is given it will check if the
        ResdataFile has at least @num occurences of @kw.
        """

        return self.num_named_kw(kw) > num

    def __contains__(self, kw):
        """
        Check if the current file contains keyword @kw.
        """
        return self.has_kw(kw)

    def has_report_step(self, report_step):
        """
        Checks if the current ResdataFile has report step @report_step.

        If the ResdataFile in question is not a restart file, you will
        just get False. If you want to check if the file contains the
        actual report_step before loading the file, you should use the
        classmethod contains_report_step() instead.
        """
        return _file._has_report_step(self, report_step)

    def num_report_steps(self):
        """
        Returns the total number of report steps in the restart file.

        Works by counting the number of 'SEQNUM' instances, and will
        happily return 0 for a non-restart file. Observe that the
        report_steps present in a unified restart file are in general
        not consecutive, i.e. the last report step will typically be
        much higher than the return value from this function.
        """
        return len(self["SEQNUM"])

    def has_sim_time(self, dtime):
        """
        Checks if the current ResdataFile has data for time @dtime.

        The implementation goes through all the INTEHEAD headers in the
        ResdataFile, i.e. it can fail if the ResdataFile instance in question
        has INTEHEAD keyword(s), but is still not a restart file. The @dtime
        argument should be a normal python datetime instance.
        """
        return _file._has_sim_time(self, CTime(dtime).value())

    def iget_restart_sim_time(self, index):
        """
        Will locate restart block nr @index and return the true time
        as a datetime instance.
        """
        ct = CTime(_file._iget_restart_time(self, index))
        return ct.datetime()

    def iget_restart_sim_days(self, index):
        """
        Will locate restart block nr @index and return the number of days
        (in METRIC at least ...) since the simulation started.

        """
        return _file._iget_restart_days(self, index)

    def get_filename(self):
        """
        Name of the file currently loaded.
        """
        fn = _file._get_src_file(self)
        return str(fn) if fn else ""

    def fwrite(self, fortio):
        """
        Will write current ResdataFile instance to fortio stream.

        ECLIPSE is written in Fortran; and a "special" handle for
        Fortran IO must be used when reading and writing these files.
        This method will write the current ResdataFile instance to a
        FortIO stream already opened for writing:

           >>> from resdata.resfile import FortIO

           >>> fortio = FortIO( "FILE.XX" )
           >>> file.fwrite( fortio )
           >>> fortio.close()

        """
        _file._fwrite(self, fortio)

    def write_index(self, index_file_name):
        if not self or not _file._write_index(self, index_file_name):
            raise OSError("Failed to write index file:%s" % index_file_name)
