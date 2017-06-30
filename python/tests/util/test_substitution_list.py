from ecl.test import ExtendedTestCase
from ecl.util import SubstitutionList


class SubstitutionListTest(ExtendedTestCase):
    def test_substitution_list(self):
        subst_list = SubstitutionList()

        subst_list.addItem("Key", "Value", "Doc String")

        self.assertEqual(len(subst_list), 1)

        with self.assertRaises(KeyError):
            item = subst_list["NoSuchKey"]

        with self.assertRaises(KeyError):
            item = subst_list.doc("NoSuchKey")

        self.assertTrue("Key" in subst_list)
        self.assertEqual(subst_list["Key"] , "Value")
        self.assertEqual(subst_list.doc("Key") , "Doc String")
        
        subst_list.addItem("Key2", "Value2", "Doc String2")
        self.assertEqual(len(subst_list), 2)

        keys = subst_list.keys( )
        self.assertEqual(keys[0] , "Key")
        self.assertEqual(keys[1] , "Key2")
        
