#!/usr/bin/env python
import unittest
import sys
import ecl

punqs3_path = "/d/proj/bg/enkf/jaskje/EnKF_PUNQS3/PUNQS3/"

class test_ecl_summary(unittest.TestCase):
  def setUp(self):
    self.summary = ecl.ecl_summary(punqs3_path + "PUNQS3.DATA")
    self.summary_keywords = ["WOPR:PRO1", "WOPR:PRO4"]
  def test_summary_display(self):
    sys.stdout = open("/dev/null","w")
    self.summary.display(self.summary_keywords)
    sys.stdout = sys.__stdout__
  def test_summary_get_start_time(self):
    tuple = self.summary.get_start_time()
    self.assertEqual(len(tuple), 9)
  def test_summary_get_simulation_case(self):
    str = self.summary.get_simulation_case()
    self.assert_(str != None)

class test_ecl_file(unittest.TestCase):
  def setUp(self):
    self.file = ecl.ecl_file(punqs3_path + "PUNQS3.X0001")
    self.kw = "PRESSURE"
  def test_file_iget_named_kw(self):
    kw_type = self.file.iget_named_kw(self.kw, 0)
    self.assert_(isinstance(kw_type, ecl.ecl_kw))
  def test_file_iget_kw(self):
    index = 1
    kw_type = self.file.iget_kw(index)
    self.assert_(isinstance(kw_type, ecl.ecl_kw))
  def test_file_has_kw(self):
    self.assert_(self.file.has_kw(self.kw))
  def test_file_get_num_distinct_kw(self):
    n = self.file.get_num_distinct_kw()
    self.assert_(n > 0)
  def test_file_iget_distinct_kw(self):
    index = 1
    str = self.file.iget_distinct_kw(index)
    self.assert_(str != None)
  def test_file_get_num_named_kw(self):
    n = self.file.get_num_named_kw(self.kw)
    self.assert_(n > 0)
  def test_file_get_num_kw(self):
    n = self.file.get_num_kw()
    self.assert_(n > 0)

class test_ecl_grid(unittest.TestCase):
  def setUp(self):
    self.grid = ecl.ecl_grid(punqs3_path + "PUNQS3.EGRID")
    self.dims = self.grid.get_dims()
    self.global_size = self.grid.get_global_size()
    self.active_size = self.grid.get_active_size()
  def test_grid_get_name(self):
    self.assert_(self.grid.get_name() != None)
  def test_grid_get_dims(self):
    self.assertEqual(len(self.grid.get_dims()), 4)
  def test_grid_get_ijk1A(self):
    self.assertEqual(len(self.grid.get_ijk1A(self.dims[3] - 1)), 3)
  def test_grid_get_global_index3(self):
    index = self.grid.get_global_index3(self.dims[0] - 1, self.dims[1] - 1, self.dims[2] - 1)
    self.assertEqual(self.global_size - 1, index)


class test_Zone(unittest.TestCase): 
  def setUp(self):
    self.grid_file = punqs3_path + "PUNQS3.EGRID"
    self.restart_base = punqs3_path + "PUNQS3.X0001"
    self.restart_mon = punqs3_path + "PUNQS3.X0083"
    self.restart_third = punqs3_path + "PUNQS3.X0050"
    init_file = punqs3_path + "PUNQS3.INIT"

    self.keywords = ("SGAS", "SWAT", "PRESSURE")
    self.zone = ecl.Zone(self.grid_file, self.keywords, self.restart_base)
    self.assert_(isinstance(self.zone, ecl.Zone))
    self.dims = self.zone.grid.get_dims()
    self.assertEqual(len(self.dims), 4)
    self.kw = "PRESSURE"
    self.index = self.dims[3] - 1
  def test_Zone_get_keywords(self):
    self.assertEqual(len(self.zone.get_keywords()), len(self.keywords))
  def test_Zone_get_values(self):
    self.assertEqual(len(self.zone.get_values(self.kw)), self.dims[3])
  def test_Zone_get_data(self):
    self.assert_(self.zone.get_data(self.kw, self.dims[3] - 1))
  def test_Zone_delete(self):
    self.assert_(self.zone.delete(self.kw))
  def test_Zone_rename(self):
    self.assert_(self.zone.rename(self.kw, "TEST"))
  def test_Zone_load(self):
    monitor = ecl.Zone(self.grid_file, self.keywords, self.restart_mon)
    self.zone.delete(self.kw)
    self.assert_(self.zone.load(monitor, self.kw))
  def test_Zone_Arithmetic_sub_zones(self):
    monitor = ecl.Zone(self.grid_file, self.keywords, self.restart_mon)
    monitor_scalar = monitor.get_data(self.kw, self.index)
    third = ecl.Zone(self.grid_file, self.keywords, self.restart_third)
    third_scalar = third.get_data(self.kw, self.index)
    zone_scalar = self.zone.get_data(self.kw, self.index)
    ans = monitor - self.zone
    ans_scalar = monitor_scalar - zone_scalar
    self.assertEqual(ans_scalar, ans.get_data(self.kw, self.index))
    ans = monitor - self.zone - third
    ans_scalar = monitor_scalar - zone_scalar - third_scalar
    self.assertEqual(ans_scalar, ans.get_data(self.kw, self.index))
  def test_Zone_Arithmetic_add_zones(self):
    monitor = ecl.Zone(self.grid_file, self.keywords, self.restart_mon)
    monitor_scalar = monitor.get_data(self.kw, self.index)
    third = ecl.Zone(self.grid_file, self.keywords, self.restart_third)
    third_scalar = third.get_data(self.kw, self.index)
    zone_scalar = self.zone.get_data(self.kw, self.index)
    ans = monitor + self.zone
    ans_scalar = monitor_scalar + zone_scalar
    self.assertEqual(ans_scalar, ans.get_data(self.kw, self.index))
    ans = monitor + self.zone + third
    ans_scalar = monitor_scalar + zone_scalar + third_scalar
    self.assertEqual(ans_scalar, ans.get_data(self.kw, self.index))
  def test_Zone_Arithmetic_mul_zones(self):
    monitor = ecl.Zone(self.grid_file, self.keywords, self.restart_mon)
    monitor_scalar = monitor.get_data(self.kw, self.index)
    third = ecl.Zone(self.grid_file, self.keywords, self.restart_third)
    third_scalar = third.get_data(self.kw, self.index)
    zone_scalar = self.zone.get_data(self.kw, self.index)
    ans = monitor * self.zone
    ans_scalar = monitor_scalar * zone_scalar
    self.assertEqual(ans_scalar, ans.get_data(self.kw, self.index))
    ans = monitor * self.zone * third
    ans_scalar = monitor_scalar * zone_scalar * third_scalar
    self.assertEqual(ans_scalar, ans.get_data(self.kw, self.index))
  def test_Zone_Arithmetic_mix(self):
    monitor = ecl.Zone(self.grid_file, self.keywords, self.restart_mon)
    monitor_scalar = monitor.get_data(self.kw, self.index)
    third = ecl.Zone(self.grid_file, self.keywords, self.restart_third)
    third_scalar = third.get_data(self.kw, self.index)
    zone_scalar = self.zone.get_data(self.kw, self.index)
    ans = monitor - 2
    ans_scalar = monitor_scalar - 2
    self.assertEqual(ans_scalar, ans.get_data(self.kw, self.index))
    ans = monitor * third - monitor + self.zone
    ans_scalar = monitor_scalar * third_scalar - monitor_scalar + zone_scalar
    self.assertEqual(ans_scalar, ans.get_data(self.kw, self.index))
    ans = -monitor 
    ans_scalar = -monitor_scalar
    self.assertEqual(ans_scalar, ans.get_data(self.kw, self.index))
    ans = -monitor * self.zone - third - 99 
    ans_scalar = -monitor_scalar * zone_scalar - third_scalar - 99
    self.assertEqual(ans_scalar, ans.get_data(self.kw, self.index))
    ans = 100 - monitor
    ans_scalar = 100 - monitor_scalar
    self.assertEqual(ans_scalar, ans.get_data(self.kw, self.index))
    ans = abs(monitor)
    ans_scalar = abs(monitor_scalar)
    self.assertEqual(ans_scalar, ans.get_data(self.kw, self.index))
    ans = 244 * monitor * 10 + 10 + self.zone - 40 - third
    ans_scalar = 244 * monitor_scalar * 10 + 10 + zone_scalar - 40 - third_scalar
    self.assertEqual(ans_scalar, ans.get_data(self.kw, self.index))
    ans = 244 * monitor * 10 - 5 + self.zone + abs(third)
    ans_scalar = 244 * monitor_scalar * 10 - 5 + zone_scalar + abs(third_scalar)
    self.assertEqual(ans_scalar, ans.get_data(self.kw, self.index))
    ans = pow(monitor, 2)
    ans_scalar = pow(monitor_scalar, 2)
    self.assertEqual(ans_scalar, ans.get_data(self.kw, self.index))
    ans = 3 * self.zone * pow(2, monitor)
    ans_scalar = 3 * zone_scalar * pow(2, monitor_scalar)
    self.assertEqual(ans_scalar, ans.get_data(self.kw, self.index))
    ans = abs((self.zone + 4 * monitor) * (pow((third + 5), 2) + 4))
    ans_scalar = abs((zone_scalar + 4 * monitor_scalar) * (pow((third_scalar + 5), 2) + 4))
    self.assertEqual(ans_scalar, ans.get_data(self.kw, self.index))
  def test_Zone_Verify_cache(self):
    self.assertEqual(len(self.keywords), len(self.zone.cache.keys()))
    for key in self.zone.cache.keys():
      self.assertEqual(len(self.zone.cache[key]), self.dims[3])
      self.assert_(isinstance(self.zone.cache[key], list))
  def test_Zone_Slicing_get(self):
    i = 10
    j = 20
    d = j - i
    new_zone = self.zone[i:j]
    self.assertEqual(len(new_zone.get_values(self.kw)), d)
    self.assertEqual(len(new_zone.get_keywords()), len(self.zone.get_keywords()))


if __name__ == '__main__':
  print """
  Now testing the 'ecl_summary' Class
  """
  suite = unittest.TestLoader().loadTestsFromTestCase(test_ecl_summary)
  unittest.TextTestRunner(verbosity=2).run(suite)
  
  print """
  Now testing the 'ecl_file' Class
  """
  suite = unittest.TestLoader().loadTestsFromTestCase(test_ecl_file)
  unittest.TextTestRunner(verbosity=2).run(suite)
  
  print """
  Now testing the 'ecl_grid' Class
  """
  suite = unittest.TestLoader().loadTestsFromTestCase(test_ecl_grid)
  unittest.TextTestRunner(verbosity=2).run(suite)
  
  print """
  Now testing the 'Zone' Class
  """
  suite = unittest.TestLoader().loadTestsFromTestCase(test_Zone)
  unittest.TextTestRunner(verbosity=2).run(suite)
  

