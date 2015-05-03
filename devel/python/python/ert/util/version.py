from ert.cwrap import CNamespace, CWrapper
from ert.util import UTIL_LIB


class Version(object):
    __namespace = CNamespace("Version")

    @classmethod
    def getBuildTime(cls):
        return Version.cNamespace().build_time()

    @classmethod
    def getGitCommit(cls, short=False):
        if not short:
            return Version.cNamespace().git_commit()
        else:
            return Version.cNamespace().git_commit_short()


    @classmethod
    def getVersion(cls):
        major = Version.cNamespace().major_version()
        minor = Version.cNamespace().minor_version()
        micro = Version.cNamespace().micro_version()
        return "%d.%d.%s" % (major , minor , micro)
        

    @classmethod
    def cNamespace(cls):
        return Version.__namespace


cwrapper = CWrapper(UTIL_LIB)

Version.cNamespace().build_time  = cwrapper.prototype("char* version_get_build_time()")
Version.cNamespace().git_commit  = cwrapper.prototype("char* version_get_git_commit()")
Version.cNamespace().git_commit_short  = cwrapper.prototype("char* version_get_git_commit_short()")
Version.cNamespace().major_version = cwrapper.prototype("int version_get_major_ert_version()")
Version.cNamespace().minor_version = cwrapper.prototype("int version_get_minor_ert_version()")
Version.cNamespace().micro_version = cwrapper.prototype("char* version_get_micro_ert_version()")
Version.cNamespace().micro_version = cwrapper.prototype("bool version_is_ert_devel_version()")
