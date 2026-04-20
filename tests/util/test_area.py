import os.path

from cwrap import BaseCClass
from resdata import ResdataPrototype


class TestArea(BaseCClass):
    __test__ = False

    _test_area_alloc = ResdataPrototype(
        "void* test_work_area_alloc__( char*, bool )", bind=False
    )
    _free = ResdataPrototype("void test_work_area_free( test_area )")
    _install_file = ResdataPrototype(
        "void test_work_area_install_file( test_area , char* )"
    )
    _copy_directory = ResdataPrototype(
        "void test_work_area_copy_directory( test_area , char* )"
    )
    _copy_file = ResdataPrototype("void test_work_area_copy_file( test_area , char* )")
    _copy_directory_content = ResdataPrototype(
        "void test_work_area_copy_directory_content( test_area , char* )"
    )
    _copy_parent_directory = ResdataPrototype(
        "void test_work_area_copy_parent_directory( test_area , char* )"
    )
    _copy_parent_content = ResdataPrototype(
        "void test_work_area_copy_parent_content( test_area , char* )"
    )
    _get_cwd = ResdataPrototype("char* test_work_area_get_cwd( test_area )")
    _get_original_cwd = ResdataPrototype(
        "char* test_work_area_get_original_cwd( test_area )"
    )

    def __init__(self, test_name, store_area=False, c_ptr=None):
        if c_ptr is None:
            c_ptr = self._test_area_alloc(test_name, store_area)

        super(TestArea, self).__init__(c_ptr)

    def get_original_cwd(self):
        return self._get_original_cwd()

    def get_cwd(self):
        return self._get_cwd()

    def orgPath(self, path):
        if os.path.isabs(path):
            return path
        else:
            return os.path.abspath(os.path.join(self.get_original_cwd(), path))

    # All the methods install_file() , copy_directory(),
    # copy_parent_directory(), copy_parent_content(),
    # copy_directory_content() and copy_file() expect an input
    # argument which is relative to the original CWD - or absolute.

    def install_file(self, filename):
        if os.path.isfile(self.orgPath(filename)):
            self._install_file(filename)
        else:
            raise IOError("No such file:%s" % filename)

    def copy_directory(self, directory):
        if os.path.isdir(self.orgPath(directory)):
            self._copy_directory(directory)
        else:
            raise IOError("No such directory: %s" % directory)

    def copy_parent_directory(self, path):
        if os.path.exists(self.orgPath(path)):
            self._copy_parent_directory(path)
        else:
            raise IOError("No such file or directory: %s" % path)

    def copy_parent_content(self, path):
        if os.path.exists(self.orgPath(path)):
            self._copy_parent_content(path)
        else:
            raise IOError("No such file or directory: %s" % path)

    def copy_directory_content(self, directory):
        if os.path.isdir(self.orgPath(directory)):
            self._copy_directory_content(directory)
        else:
            raise IOError("No such directory: %s" % directory)

    def copy_file(self, filename):
        if os.path.isfile(self.orgPath(filename)):
            self._copy_file(filename)
        else:
            raise IOError("No such file:%s" % filename)

    def free(self):
        self._free()

    def getFullPath(self, path):
        if not os.path.exists(path):
            raise IOError("Path not found:%s" % path)

        if os.path.isabs(path):
            raise IOError("Path:%s is already absolute" % path)

        return os.path.join(self.get_cwd(), path)


class TestAreaContext(object):
    __test__ = False

    def __init__(self, test_name, store_area=False):
        self.test_name = test_name
        self.store_area = store_area

    def __enter__(self):
        """
        @rtype: TestArea
        """
        self.test_area = TestArea(self.test_name, store_area=self.store_area)
        return self.test_area

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.test_area.free()  # free the TestData object (and cd back to the original dir)
        self.test_area.free = None  # avoid double free
        del self.test_area
        return False
