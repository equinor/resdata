import copy
import datetime
from unittest import TestCase

import six
from resdata.util.util import (
    CTime,
    IntVector,
    PermutationVector,
)


class UtilTest(TestCase):
    def setUp(self):
        pass

    def dotest_slicing(self, vec):
        self.assertEqual(10, len(vec))
        self.assertEqual(vec[-1], vec[9])
        self.assertEqual(8, len(vec[:8]))
        self.assertEqual(9, len(vec[1:]))
        self.assertEqual(3, len(vec[1:8:3]))
        odds = vec[1::2]
        self.assertEqual(4, len(vec[1:8:2]))
        for i in range(4):
            self.assertEqual(vec[2 * i + 1], odds[i])

    def test_slicing(self):
        iv = IntVector(initial_size=10)
        for i in range(10):
            iv[i] = i**3
        self.dotest_slicing(iv)

    def test_vector_operations_with_exceptions(self):
        iv1 = IntVector()
        iv1.append(1)
        iv1.append(2)
        iv1.append(3)

        iv2 = IntVector()
        iv2.append(4)
        iv2.append(5)

        # Size mismatch
        with self.assertRaises(ValueError):
            iv3 = iv1 + iv2

        # Size mismatch
        with self.assertRaises(ValueError):
            iv3 = iv1 * iv2

    def test_activeList(self):
        active_list = IntVector.active_list("1,10,100-105")
        self.assertTrue(len(active_list) == 8)
        self.assertTrue(active_list[0] == 1)
        self.assertTrue(active_list[2] == 100)
        self.assertTrue(active_list[7] == 105)
        self.assertEqual(active_list.count(100), 1)
        active_list.append(100)
        active_list.append(100)
        self.assertEqual(active_list.count(100), 3)

        active_list = IntVector.active_list("1,10,100-105X")
        self.assertFalse(active_list)

    def test_value_list(self):
        list2 = IntVector.valueList("3,10-12,0,1")
        self.assertTrue(len(list2) == 6)
        expected = [3, 10, 11, 12, 0, 1]
        for v1, v2 in zip(list2, expected):
            self.assertEqual(v1, v2)

    def test_contains_int(self):
        iv = IntVector()
        iv[0] = 1
        iv[1] = 10
        iv[2] = 100
        iv[3] = 1000

        self.assertTrue(1 in iv)
        self.assertTrue(10 in iv)
        self.assertTrue(88 not in iv)
        self.assertTrue(99 not in iv)

    def test_pop(self):
        a = IntVector()
        a.append(1)
        a.append(2)

        self.assertEqual(a.pop(), 2)
        self.assertEqual(len(a), 1)
        self.assertEqual(a.pop(), 1)
        self.assertEqual(len(a), 0)
        with self.assertRaises(ValueError):
            a.pop()

    # ----

    def test_shift(self):
        a = IntVector()
        a.append(1)
        a.append(2)
        a.append(3)
        a.append(4)
        a.append(5)

        with self.assertRaises(ValueError):
            a >> -1

        with self.assertRaises(ValueError):
            a << -1

        with self.assertRaises(ValueError):
            a << -6

        b = a << 2
        self.assertEqual(list(b), [3, 4, 5])

        a <<= 2
        self.assertEqual(list(a), [3, 4, 5])

        b = a >> 2
        self.assertEqual(list(b), [0, 0, 3, 4, 5])

        a >>= 2
        self.assertEqual(list(a), [0, 0, 3, 4, 5])

    def test_int_vector(self):
        a = IntVector()
        a.append(1)
        a.append(2)
        a.append(3)
        a.append(4)
        a.append(5)

        self.assertEqual(list(a), [1, 2, 3, 4, 5])

        a.sort(reverse=True)
        self.assertEqual(list(a), [5, 4, 3, 2, 1])

        self.assertTrue(a.max(), 5)
        self.assertTrue(a.min(), 1)
        self.assertTrue(a.minIndex(), 4)

        self.assertEqual(a.maxIndex(reverse=True), 0)
        self.assertEqual(a.maxIndex(reverse=False), 0)

        a[4] = 5
        self.assertTrue(a[4] == 5)

        a_plus_one = a + 1
        self.assertEqual(list(a_plus_one), [6, 5, 4, 3, 6])

        sliced = a[0:3]
        self.assertEqual(list(sliced), [5, 4, 3])

        with self.assertRaises(IndexError):
            item = a[6]

        copy_of_a = a.copy()
        self.assertEqual(list(a), list(copy_of_a))

        another_copy_of_a = copy_of_a.copy()
        self.assertEqual(list(a), list(another_copy_of_a))

        a.sort()
        self.assertEqual(list(a), [2, 3, 4, 5, 5])
        self.assertEqual(a.safeGetByIndex(0), 2)
        a.setReadOnly(False)
        self.assertFalse(a.getReadOnly())

    # ---

    def test_div(self):
        v = IntVector()
        v[0] = 100
        v[1] = 10
        v[2] = 1
        v /= 10

        self.assertEqual(list(v), [10, 1, 0])

    def test_true(self):
        iv = IntVector()
        self.assertFalse(
            iv
        )  # Will invoke the __len__ function; could override with __nonzero__
        iv[0] = 1
        self.assertTrue(iv)

    def test_unique(self):
        iv = IntVector()
        iv.append(1)
        iv.append(1)
        iv.append(1)
        iv.append(0)
        iv.append(1)
        iv.append(2)
        iv.append(2)
        iv.append(0)
        iv.append(3)
        iv.selectUnique()
        self.assertEqual(len(iv), 4)
        self.assertEqual(iv[0], 0)
        self.assertEqual(iv[1], 1)
        self.assertEqual(iv[2], 2)
        self.assertEqual(iv[3], 3)
        self.assertEqual(iv.first_eq(iv), 0)

        iv *= 2
        self.assertEqual(list(iv), [0, 2, 4, 6])

    def test_element_sum(self):
        iv = IntVector()
        for i in range(10):
            iv.append(i + 1)

        self.assertEqual(iv.elementSum(), 55)

    def test_asList(self):
        v = IntVector()
        v[0] = 100
        v[1] = 10
        v[2] = 1

        l = v.asList()
        self.assertListEqual(l, [100, 10, 1])

    def test_true_false(self):
        v = IntVector(default_value=77)
        self.assertFalse(v)
        v[10] = 77
        self.assertTrue(v)

    def test_count_equal(self):
        v = IntVector(default_value=77)
        v[0] = 1
        v[10] = 1
        v[20] = 1
        self.assertEqual(v.countEqual(1), 3)

    def range_test(self, v, a, b, d):
        v.initRange(a, b, d)
        r = range(a, b, d)

        self.assertEqual(len(v), len(r))
        for a, b in zip(v, r):
            self.assertEqual(a, b)

    def create_range_test(self, v, a, b, d):
        v = IntVector.createRange(a, b, d)
        r = range(a, b, d)

        self.assertEqual(len(v), len(r))
        for a, b in zip(v, r):
            self.assertEqual(a, b)

    def test_range(self):
        v = IntVector()
        v[10] = 99

        with self.assertRaises(ValueError):
            v.initRange(1, 2, 0)

        self.range_test(v, 0, 5, 1)
        self.range_test(v, 0, 100, 3)
        self.range_test(v, 0, 100, -3)

        self.create_range_test(v, 0, 5, 1)
        self.create_range_test(v, 0, 100, 3)
        self.create_range_test(v, 0, 100, -3)

    def test_perm_vector(self):
        v = IntVector.createRange(11, 0, -1)
        perm = v.permutationSort()
        self.assertEqual(perm[0], 10)
        self.assertEqual(perm[5], 5)
        self.assertEqual(perm[10], 0)

    def test_init_linear(self):
        with self.assertRaises(ValueError):
            v = IntVector.create_linear(0, 10, 1)

        v = IntVector.create_linear(0, 10, 11)
        for i in range(len(v)):
            self.assertEqual(v[i], i)

        v = IntVector.create_linear(10, 0, 11)
        for i in range(len(v)):
            self.assertEqual(v[i], 10 - i)

    def test_equal(self):
        v1 = IntVector()
        v1[3] = 99

        v2 = IntVector()
        self.assertNotEqual(v1, v2)
        v2[3] = 99
        self.assertEqual(v1, v2)
