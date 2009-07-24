#!/usr/bin/env python
from ecl import *
import sys

try:	
	assert (len(sys.argv) == 5)
except AssertionError:
	print '\nUsage: %s <grid_file> <in_file.dat> <out_file.grdecl> <keyword>' % (sys.argv[0])
	print """
	Converts a Matlab .dat file with active indexes to a grdecl file with global
	indexes - ready for loading into RMS with the same grid.
	
	Example of a correct Matlab output command: 
	save variable.dat variable -ASCII -DOUBLE
	"""

        sys.exit()

f = open(sys.argv[2], 'r')
farr = map(float, f.readlines())
f.close()

grid = ecl_grid(sys.argv[1])
global_size = grid.get_global_size()

li = list();
k = 0
for j in xrange(0, global_size):
	ret = grid.get_active_index1(j)
	if ret == -1:
		li.append(float(0))
	else:
		li.append(farr[k])
		k += 1
		
ecl_kw().write_new_grdecl(sys.argv[3], sys.argv[4], li, "w")
