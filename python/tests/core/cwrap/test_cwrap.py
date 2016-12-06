import ert
from cwrap import BaseCClass
from ert.test  import ExtendedTestCase

from cwrap import Prototype

def stringlistProtoFactory(signature):
    lib  = ert.load("libert_util")
    return Prototype(lib, signature, bind = False)

class StringListTest(BaseCClass):
    TYPE_NAME = 'stringlisttest'
    _alloc     = stringlistProtoFactory("void* stringlist_alloc_new( )")
    _alloc_obj = stringlistProtoFactory("stringlisttest_obj stringlist_alloc_new( )")
    _alloc_ref = stringlistProtoFactory("stringlisttest_ref stringlist_alloc_new( )")
    _free      = stringlistProtoFactory("void stringlist_free(stringlisttest )")
    _dateStamp = stringlistProtoFactory("char* util_alloc_date_stamp_utc()")

    def __init__(self):
        c_ptr = self._alloc()
        if c_ptr:
            super(StringListTest, self).__init__(c_ptr)
        else:
            raise Exception('Internal error!  Unable to construct StringListType.')

    @classmethod
    def alloc_obj(cls):
        return cls._alloc_obj()

    @classmethod
    def alloc_ref(cls):
        return cls._alloc_ref()

    def free(self):
        self._free(self)



class CWrapTest(ExtendedTestCase):

    def setUp(self):
        self._sl = StringListTest()
        self._sl_obj = StringListTest.alloc_obj()
        self._sl_ref = StringListTest.alloc_ref()

    def test_return_type(self):
        self.assertIsInstance(self._sl, StringListTest)
        self.assertIsInstance(self._sl_obj, StringListTest)
        self.assertIsInstance(self._sl_ref, StringListTest)

        self.assertFalse(self._sl.isReference())
        self.assertFalse(self._sl_obj.isReference())
        self.assertTrue( self._sl_ref.isReference())

    def test_from_param(self):
        p1 = BaseCClass.from_param(self._sl)
        p2 = BaseCClass.from_param(self._sl_obj)
        p3 = BaseCClass.from_param(self._sl_ref)

        self.assertNotEqual(p1, p2)
        self.assertNotEqual(p2, p3)
        self.assertNotEqual(p1, p3)
