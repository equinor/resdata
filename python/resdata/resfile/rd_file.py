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
import ctypes
import datetime
import re
import types

from cwrap import BaseCClass
from resdata import FileMode, FileType, ResdataPrototype
from resdata.resfile import ResdataFileView, ResdataKW
from resdata.util.util import CTime, monkey_the_camel


class ResdataFile(BaseCClass):
    TYPE_NAME = "rd_file"
    _open = ResdataPrototype("void* rd_file_open(char*, rd_file_flag_enum)", bind=False)
    _get_file_type = ResdataPrototype(
        "rd_file_enum rd_get_file_type(char*, bool*, int*)", bind=False
    )
    _writable = ResdataPrototype("bool rd_file_writable(rd_file)")
    _save_kw = ResdataPrototype("void rd_file_save_kw(rd_file, rd_kw)")
    _close = ResdataPrototype("void rd_file_close(rd_file)")
    _iget_restart_time = ResdataPrototype(
        "rd_time_t rd_file_iget_restart_sim_date(rd_file, int)"
    )
    _iget_restart_days = ResdataPrototype(
        "double rd_file_iget_restart_sim_days(rd_file, int)"
    )
    _get_restart_index = ResdataPrototype(
        "int rd_file_get_restart_index(rd_file, rd_time_t)"
    )
    _get_src_file = ResdataPrototype("char* rd_file_get_src_file(rd_file)")
    _replace_kw = ResdataPrototype(
        "void rd_file_replace_kw(rd_file, rd_kw, rd_kw, bool)"
    )
    _fwrite = ResdataPrototype("void rd_file_fwrite_fortio(rd_file, rd_fortio, int)")
    _has_report_step = ResdataPrototype("bool rd_file_has_report_step(rd_file, int)")
    _has_sim_time = ResdataPrototype("bool rd_file_has_sim_time(rd_file, rd_time_t)")
    _get_global_view = ResdataPrototype(
        "rd_file_view_ref rd_file_get_global_view(rd_file)"
    )
    _write_index = ResdataPrototype("bool rd_file_write_index(rd_file, char*)")
    _fast_open = ResdataPrototype(
        "void* rd_file_fast_open(char*, char*, int)", bind=False
    )

    @staticmethod
    def get_filetype(filename):
        fmt_file = ctypes.c_bool()
        report_step = ctypes.c_int()

        file_type = ResdataFile._get_file_type(
            filename, ctypes.byref(fmt_file), ctypes.byref(report_step)
        )
        if file_type in [
            FileType.RESTART,
            FileType.SUMMARY,
        ]:
            report_step = report_step.value
        else:
            report_step = None

        if file_type in [FileType.OTHER, FileType.DATA]:
            fmt_file = None
        else:
            fmt_file = fmt_file.value

        return (file_type, report_step, fmt_file)

    @classmethod
    def restart_block(cls, filename, dtime=None, report_step=None):
        raise NotImplementedError(
            "The restart_block implementation has been removed - open file normally and use ResdataFileView."
        )

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
            fname = self.getFilename()
            matchObj = re.search("\.[XF](\d{4})$", fname)
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
        fn = self.getFilename()
        wr = ", read/write" if self._writable() else ""
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
            c_ptr = self._open(filename, flags)
        else:
            c_ptr = self._fast_open(filename, index_filename, flags)

        if c_ptr is None:
            raise IOError('Failed to open file "%s"' % filename)
        else:
            super(ResdataFile, self).__init__(c_ptr)
            self.global_view = self._get_global_view()
            self.global_view.setParent(self)

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
        if self._writable():
            self._save_kw(kw)
        else:
            raise IOError(
                'save_kw: the file "%s" has been opened read only.' % self.getFilename()
            )

    def __len__(self):
        return len(self.global_view)

    def close(self):
        if self:
            self._close()
            self._invalidateCPointer()

    def free(self):
        self.close()

    def block_view(self, kw, kw_index):
        if not kw in self:
            raise KeyError('No such keyword "%s".' % kw)
        ls = self.global_view.numKeywords(kw)
        idx = kw_index
        if idx < 0:
            idx += ls
        if 0 <= idx < ls:
            return self.global_view.blockView(kw, idx)
        raise IndexError(
            "Index out of range, must be in [0, %d), was %d." % (ls, kw_index)
        )

    def block_view2(self, start_kw, stop_kw, start_index):
        return self.global_view.blockView2(start_kw, stop_kw, start_index)

    def restart_view(
        self, seqnum_index=None, report_step=None, sim_time=None, sim_days=None
    ):
        return self.global_view.restartView(
            seqnum_index, report_step, sim_time, sim_days
        )

    def select_block(self, kw, kw_index):
        raise NotImplementedError(
            "The select_block implementation has been removed - use ResdataFileView"
        )

    def select_global(self):
        raise NotImplementedError(
            "The select_global implementation has been removed - use ResdataFileView"
        )

    def select_restart_section(self, index=None, report_step=None, sim_time=None):
        raise NotImplementedError(
            "The select_restart_section implementation has been removed - use ResdataFileView"
        )
        """
        Will select a restart section as the active section.

        You must specify a report step with the @report_step argument,
        a true time with the @sim_time argument or a plain index to
        select restart block. If none of arguments are given exception
        TypeError will be raised. If present the @sim_time argument
        should be a datetime instance.

        If the restart section you ask for can not be found the method
        will raise a ValueError exeception. To protect against this
        you can query first with the has_report_step(),
        has_sim_time() or num_report_steps() methods.

        This method should be used when you have already loaded the
        complete file; if you only want to load a section from the
        file you can use the classmethod restart_block().

        The method will return 'self' which can be used to aid
        readability.
        """

    def select_last_restart(self):
        raise NotImplementedError(
            "The select_restart_section implementation has been removed - use ResdataFileView"
        )
        """
        Will select the last SEQNUM block in restart file.

        Works by searching for the last SEQNUM keyword; the SEQNUM
        Keywords are only present in unified restart files. If this
        is a non-unified restart file (or not a restart file at all),
        the method will do nothing and return False.
        """

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
        index = self._get_restart_index(CTime(dtime))
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

    def replace_kw(self, old_kw, new_kw):
        """
        Will replace @old_kw with @new_kw in current ResdataFile instance.

        This method can be used to replace one of the ResdataKW instances
        in the current ResdataFile. The @old_kw reference must be to the
        actual ResdataKW instance in the current ResdataFile instance (the
        final comparison is based on C pointer equality!), i.e. it
        must be a reference (not a copy) from one of the ??get_kw??
        methods of the ResdataFile class. In the example below we replace
        the SWAT keyword from a restart file:

           swat = file.iget_named_kw( "SWAT" , 0 )
           new_swat = swat * 0.25
           file.replace_kw( swat , new_swat )


        The C-level rd_file_type structure takes full ownership of
        all installed rd_kw instances; mixing the garbage collector
        into it means that this is quite low level - and potentially
        dangerous!
        """

        # We ensure that this scope owns the new_kw instance; the
        # new_kw will be handed over to the rd_file instance, and we
        # can not give away something we do not alreeady own.
        if not new_kw.data_owner:
            new_kw = ResdataKW.copy(new_kw)

        # The rd_file instance will take responsability for freeing
        # this rd_kw instance.
        new_kw.data_owner = False
        self._replace_kw(old_kw, new_kw, False)

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
        return self.global_view.uniqueSize()

    def keys(self):
        """
        Will return a list of unique kw names - like keys() on a dict.
        """
        header_dict = {}
        for index in range(len(self)):
            kw = self[index]
            header_dict[kw.getName()] = True
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
        seqnum_list = self["SEQNUM"]
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
            # This is a uber-hack; should export the rd_rsthead
            # object as ctypes structure.
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
        return self.global_view.numKeywords(kw)

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
        return self._has_report_step(report_step)

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

        The implementation goes through all the INTEHEAD headers in
        the ResdataFile, i.e. it can be fooled (and probably crash and
        burn) if the ResdataFile instance in question is has INTEHEAD
        keyword(s), but is still not a restart file. The @dtime
        argument should be a normal python datetime instance.
        """
        return self._has_sim_time(CTime(dtime))

    def iget_restart_sim_time(self, index):
        """
        Will locate restart block nr @index and return the true time
        as a datetime instance.
        """
        ct = CTime(self._iget_restart_time(index))
        return ct.datetime()

    def iget_restart_sim_days(self, index):
        """
        Will locate restart block nr @index and return the number of days
        (in METRIC at least ...) since the simulation started.

        """
        return self._iget_restart_days(index)

    def get_filename(self):
        """
        Name of the file currently loaded.
        """
        fn = self._get_src_file()
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
        self._fwrite(fortio, 0)

    def write_index(self, index_file_name):
        if not self or not self._write_index(index_file_name):
            raise IOError("Failed to write index file:%s" % index_file_name)


class ResdataFileContextManager(object):
    def __init__(self, rd_file):
        self.__rd_file = rd_file

    def __enter__(self):
        return self.__rd_file

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.__rd_file.close()
        return False


def openResdataFile(file_name, flags=FileMode.DEFAULT):
    print("The function openResdataFile is deprecated, use open_rd_file.")
    return open_rd_file(file_name, flags)


def open_rd_file(file_name, flags=FileMode.DEFAULT):
    return ResdataFileContextManager(ResdataFile(file_name, flags))


monkey_the_camel(ResdataFile, "getFileType", ResdataFile.get_filetype, staticmethod)
monkey_the_camel(ResdataFile, "blockView", ResdataFile.block_view)
monkey_the_camel(ResdataFile, "blockView2", ResdataFile.block_view2)
monkey_the_camel(ResdataFile, "restartView", ResdataFile.restart_view)
monkey_the_camel(ResdataFile, "getFilename", ResdataFile.get_filename)
