#!/usr/bin/env python

from ecl import *
from time import *
import pprint

arr = ["WOPR:OP_4", "WOPR:OP_1"]
file = '/private/masar/enkf/devel/Testcases/Common/Refcase/EXAMPLE_01_BASE.DATA'
file2 = '/private/masar/enkf/devel/Testcases/Common/ECLIPSE/EXAMPLE_01_BASE.EGRID'
file3 = '/private/masar/enkf/devel/Testcases/SimpleEnKF/Simulations/Real0/EXAMPLE_01_BASE_0000.X0039'
file4 = '/private/masar/enkf/devel/Testcases/Common/ECLIPSE/EXAMPLE_01_BASE.EGRID'
kw = 'WOPR:OP_4'

print '############# ECL_UTIL #############'
u = ecl_util()
u.get_file_type(file2)
print "file_type: %d, fmt_file: %d, report_nr: %d" % (u.file_type, u.fmt_file, u.report_nr)

print '############# ECL_SUMMARY #############'
s = ecl_summary(file)
s.display(arr)
print "Data file: %s" % s.ecl_data_file
print "Has ministep: %d" % s.has_ministep(123)
print "First ministep: %d" % s.get_first_ministep()
print "Last ministep: %d" % s.get_last_ministep()
print "Has report_step: %d" % s.has_report_step(123)
print "First report_step: %d" % s.get_first_report_step()
print "Last report_step: %d" % s.get_last_report_step()
print "Has general var: %d" % s.has_general_var(kw)
print "Get general var index: %d" % s.get_general_var_index(kw)
print "Get general var: %f" % s.get_general_var(48, kw)
print "Get start time %s" % s.get_start_time()
print "Formatted time: %s" % strftime("%a, %d %b %Y %H:%M:%S +0000", s.get_start_time())
print "Get simulation case: %s" % s.get_simulation_case()

print '############# ECL_GRID #############'
g = ecl_grid(file2)
print "Grid name: %s" % g.get_name()
g.summarize()
print g.get_dims()
print g.get_ijk1(132)

print '############# ECL_FILE #############'
f = ecl_file(file3)
print "Filename: %s" % f.filename
k = f.iget_named_kw('PRESSURE', 0)
k2 = f.iget_named_kw('SWAT', 0)

list = k.get_data();
#list2 = k2.get_data();
print "List length: %d" % len(list)
#print "List 2 length: %d" % len(list2)

print k.get_header_ref()
print k.get_str_type_ref()

for j in xrange(f.get_num_distinct_kw()):
	str_kw = f.iget_distinct_kw(j)
	m = f.get_num_named_kw(str_kw)
	print "The file contains: %d occurences of '%s'" % (m, str_kw)


print '############# FORTIO #############'
fort = ecl_fortio(file3)
fort.read("PRESSURE")
fort.close()
