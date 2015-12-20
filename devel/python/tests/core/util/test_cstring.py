from unittest import TestCase
from ert.cwrap import clib, CWrapper

test_lib  = clib.ert_load("libert_util") 
cwrapper =  CWrapper(test_lib)
alloc_string_copy = cwrapper.prototype("cstring_obj util_alloc_string_copy(char*)")


class CStringTest(TestCase):

    def test_get(self):
        s1 = "String123"
        s2 = alloc_string_copy( s1 )
        self.assertEqual( s1 , s2 )

        

