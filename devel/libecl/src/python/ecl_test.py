#!/usr/bin/env python
import unittest
import sys
import ecl

enkf_cvs_path = "/private/masar/enkf"

class test_ecl_summary(unittest.TestCase):
  def setUp(self):
    self.summary = ecl.ecl_summary(enkf_cvs_path + "/Testcases/Common/Refcase/EXAMPLE_01_BASE.DATA")
    self.summary_keywords = ["WOPR:OP_4", "WOPR:OP_1"]

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
    self.file = ecl.ecl_file(enkf_cvs_path + "/Testcases/SimpleEnKF/Simulations/Real0/EXAMPLE_01_BASE_0000.X0039")
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
    
    
class test_Zone(unittest.TestCase): 
  def setUp(self):
    path = "/d/proj/bg/enkf/jaskje/EnKF_PUNQS3/PUNQS3/" 
    self.grid_file = path + "PUNQS3.EGRID"
    self.restart_base = path + "PUNQS3.X0001"
    self.restart_mon = path + "PUNQS3.X0083"
    self.restart_third = path + "PUNQS3.X0050"
    init_file = path + "PUNQS3.INIT"

    self.keywords = ("SGAS", "SWAT", "PRESSURE")
    self.zone = ecl.Zone(self.grid_file, self.keywords, self.restart_base)
    self.assert_(isinstance(self.zone, ecl.Zone))
    self.dims = self.zone.grid.get_dims()
    self.assertEqual(len(self.dims), 4)
    self.kw = "PRESSURE"

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
    third = ecl.Zone(self.grid_file, self.keywords, self.restart_third)
    diff = monitor - self.zone
    a =  self.zone.get_data(self.kw, self.dims[3] - 1)
    b = monitor.get_data(self.kw, self.dims[3] - 1)
    c = b - a 
    e =  diff.get_data(self.kw, self.dims[3] - 1)
    self.assertEqual(e, c)
    f = third.get_data(self.kw, self.dims[3] - 1)
    g = b - a - f
    diff = monitor - self.zone - third
    h = diff.get_data(self.kw, self.dims[3] - 1)
    self.assertEqual(g, h)
  def test_Zone_Arithmetic_add_zones(self):
    monitor = ecl.Zone(self.grid_file, self.keywords, self.restart_mon)
    third = ecl.Zone(self.grid_file, self.keywords, self.restart_third)
    diff = monitor + self.zone
    a =  self.zone.get_data(self.kw, self.dims[3] - 1)
    b = monitor.get_data(self.kw, self.dims[3] - 1)
    c = b + a 
    e =  diff.get_data(self.kw, self.dims[3] - 1)
    self.assertEqual(e, c)
    f = third.get_data(self.kw, self.dims[3] - 1)
    g = b + a + f
    diff = monitor + self.zone + third
    h = diff.get_data(self.kw, self.dims[3] - 1)
    self.assertEqual(g, h)
  def test_Zone_Arithmetic_mul_zones(self):
    monitor = ecl.Zone(self.grid_file, self.keywords, self.restart_mon)
    third = ecl.Zone(self.grid_file, self.keywords, self.restart_third)
    diff = monitor * self.zone
    a =  self.zone.get_data(self.kw, self.dims[3] - 1)
    b = monitor.get_data(self.kw, self.dims[3] - 1)
    c = b * a 
    e =  diff.get_data(self.kw, self.dims[3] - 1)
    self.assertEqual(e, c)
    f = third.get_data(self.kw, self.dims[3] - 1)
    g = b * a * f
    diff = monitor * self.zone * third
    h = diff.get_data(self.kw, self.dims[3] - 1)
    self.assertEqual(g, h)

  def test_Zone_Verify_cache(self):
    self.assertEqual(len(self.keywords), len(self.zone.cache.keys()))

    for key in self.zone.cache.keys():
      self.assertEqual(len(self.zone.cache[key]), self.dims[3])
      self.assert_(isinstance(self.zone.cache[key], list))


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
  Now testing the 'Zone' Class
  """
  suite = unittest.TestLoader().loadTestsFromTestCase(test_Zone)
  unittest.TextTestRunner(verbosity=2).run(suite)
  

