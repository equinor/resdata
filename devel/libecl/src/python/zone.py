#!/usr/bin/env python
from ecl import *
import pprint as pp
import time
from math import *


## TODO: 
## - Skrive til restartfiler
## - Mulighet for aa lese inn nye restartfiler og erstatte f.eks PRESSURE
## - Forbedre innlesing, teste med numpy der
## - Stoette for INTE

class zone(object):
	kw_arr = []
	## Keyword => ecl_kw instance
	h = {}

	## Keyword => List of dicts with keys
	##			  i: .., j: .., k: .. 
	##			  val: ..., global_index: ...
	##
	cell = {}

	def __init__(self, grid_file, restart_file, init_file, kw_list = []):
		self.grid = ecl_grid(grid_file)
		filelist = [];
		filelist.append(ecl_file(restart_file))
		filelist.append(ecl_file(init_file))
		self.filelist = filelist
		self.dims = self.grid.get_dims()
		self.kw_list = kw_list;

		for file in filelist:
			for j in xrange(file.get_num_distinct_kw()):
				str_kw = file.iget_distinct_kw(j)
				try:
					kw_list.index(str_kw.strip())
				except:
					pass
				else:
					kw = file.iget_named_kw(str_kw, 0)
					self.add_keyword(kw)

		pp.pprint(self.h)

	def __del__(self):
		print "Del zone object"
		for kw in self.kw_list:
			print self.h[kw]
		for file in self.filelist:
			print file

	def add_keyword(self, kw):
		k = kw.get_size()
		str_kw = kw.get_header_ref()
		if k == self.dims['active_size']:
			print "Loaded kw %s (complete on grid) with size %d" % (str_kw, k)
			self.h[str_kw.strip()] = kw
			self.kw_arr.append(str_kw.strip())

	def fill_data(self, func):
		for key in self.kw_arr:
			print "Filling for %s" % key
			start = time.clock()
			arr = self.h[key].get_data();
			end = time.clock()
			print "Time elapsed = ", end - start, "seconds"

			list = []
			start = time.clock()
			for j, val in enumerate(arr):
				coords = self.grid.get_ijk1(j)
				val_new = func(val)
				coords['val'] = val_new
				coords['global_index'] = j
				list.append(coords)

			end = time.clock()
			print "Time elapsed = ", end - start, "seconds (filling list)"

			self.cell[key] = list
			print "Done filling"
#		print pp.pformat(self.cell)

	def fill_data_numpy(self, func):
		print "filling narray matrix"
		# mat[i][j][k] = coords['val']
		


def square(item):
	return pow(item,2)

def volume_map(z, param):
	print "Creating 'volume map'"
	dims = z.grid.get_dims();
	# Create an i x j matrix
	mat = []
	for i in range(0,dims['i']):
		mat.append([0]*(dims['j']))

	test_sum = 0
	for c in z.cell[param]:
		i = c['i']
		j = c['j']
		if i == 5 and j == 2:
			pp.pprint(c)
			test_sum += c['val']
		mat[i][j] += c['val']

	print test_sum
	print mat[5][2]

			
if __name__ == '__main__':
	grid_file = '/private/masar/enkf/devel/Testcases/Common/ECLIPSE/EXAMPLE_01_BASE.EGRID'
	restart_file = '/private/masar/enkf/devel/Testcases/SimpleEnKF/Simulations/Real0/EXAMPLE_01_BASE_0000.X0039'
	init_file = "/private/masar/enkf/devel/Testcases/SimpleEnKF/Simulations/Real0/EXAMPLE_01_BASE_0000.INIT"
	z = zone(grid_file, restart_file, init_file, ["SGAS", "PORO", "SWAT"])
	z.fill_data(square)
	volume_map(z, "PORO")
