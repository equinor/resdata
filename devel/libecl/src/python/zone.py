#!/usr/bin/env python
from ecl import *
import pprint as pp
import time
from math import *


## TODO: 
## - Skrive til restartfiler
## - Mulighet for aa lese inn nye restartfiler og erstatte f.eks PRESSURE
## - Forbedre innlesing, teste med numpy der

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

		if len(kw_list) == 0:
			for file in filelist:
				n = file.get_num_kw()
				for i in xrange(0, n):
					print file.iget_distinct_kw(i)
		else:
			for val in kw_list:
				print val
				
			

#		for file in filelist:
#			for j in xrange(file.get_num_distinct_kw()):
#				str_kw = file.iget_distinct_kw(j)
#				if kw_list_len == 0:
#					kw = file.iget_named_kw(str_kw, 0)
#					self.add_keyword(kw)
#				else:
#					try:
#						kw_list.index(str_kw.strip())
#					except:
#						pass
#					else:
#						kw = file.iget_named_kw(str_kw, 0)
#						self.add_keyword(kw)

	def __del__(self):
		print "Del zone object"

	def iterate_grid(self, func, keywords):
		grid = self.grid
		active_size = self.dims[3]

		list = []
		for val in keywords:
			for file in self.filelist:
				m = file.get_num_named_kw(val)
				
				if m == 0: 
					continue

				print "%d occurences of %s in %s" % (m, val, file.filename)

				for iter in xrange(0,m):
					kw_type = file.iget_named_kw(val, iter)
					list.append(kw_type)

		for n in xrange(0, active_size):
			coords = grid.get_ijk1A(n)
			func(grid, list, coords)
#				print grid.get_property(key, coords)
				
#poro = grid.get_property(kw_type, coords)
			#s = grid.get_property(z.h["SGAS"], coords)
		
		

#	def add_keyword(self, kw):
#		k = kw.get_size()
#		str_kw = kw.get_header_ref()
#		if k == self.dims[3]:
#			print "Loaded kw %s (complete on grid) with size %d" % (str_kw, k)
#			self.h[str_kw.strip()] = kw
#			self.kw_arr.append(str_kw.strip())

#	def fill_data(self, func):
#		for key in self.kw_arr:
#			print "Filling for %s" % key
#			start = time.clock()
#			arr = self.h[key].get_data();
#			end = time.clock()
#			print "Time elapsed = ", end - start, "seconds"

#			list = []
#			start = time.clock()
#			for j, val in enumerate(arr):
#				coords = self.grid.get_ijk1(j)
#				val_new = func(val)
#				tmp = {}
#				tmp['coords'] = coords
#				tmp['val'] = val_new
#				tmp['global_index'] = j
#				list.append(tmp)

#			end = time.clock()
#			print "Time elapsed = ", end - start, "seconds (filling list)"
			
#			self.cell[key] = list
#			print "Done filling"
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
	for i in range(0,dims[0]):
		mat.append([0]*(dims[1]))

	test_sum = 0
	for c in z.cell[param]:
		i = c['coords'][0]
		j = c['coords'][1]
		if i == 5 and j == 2:
			pp.pprint(c)
			test_sum += c['val']
		mat[i][j] += c['val']

	print test_sum
	print mat[5][2]

def gassmann_2(grid, list, coords):
	# Sand
	k_m   = 18.0e9;
	mu_m  = 3.2e9;
	rho_m = 2.65e3;

	# Fluids
	k_o   = 1.50e9;  # SAE 30 Oil, source: www.engineeringtoolbox.com
	k_w   = 2.34e9;  # Seawater, source: www.engineeringtoolbox.com
	rho_o = 0.89e3;  # SAE 30 Oil, source: www.engineeringtoolbox.com
	rho_w = 1.010e3; # Seawater, source: www.engineeringtoolbox.com

	for val in list:	
		if val.get_header_ref().strip() == "PORO":
			poro = grid.get_property(val, coords)
		if val.get_header_ref().strip() == "SGAS":
			s = grid.get_property(val, coords)

	print "poro: %f, sgas: %f" % (poro, s)

	rho_res = poro*s*rho_o+poro*(1-s)*rho_w+(1-poro)*rho_m
	mu_res = (1-poro)*mu_m

	if poro == 0:
		print "Poro was zero!"
		k_res = k_m
	else:
		k_rockframe = (1-poro)*k_m
		k_fluid = 1/((s/k_o)+(1-s)/k_w)
		b = k_rockframe/(k_m-k_rockframe)+k_fluid/(poro*(k_m-k_fluid))
		k_res = (b*k_m)/(1 + b)

	vp = sqrt((k_res+(4/3)*mu_res)/rho_res)
	vs = sqrt(mu_res/rho_res)
	print "k_res: %f, mu_res: %f, rho_res: %f, vp: %f, vs: %f" % (k_res, mu_res, rho_res, vp, vs)
	


def gassmann(z):
	grid = z.grid

	# Sand
	k_m   = 18.0e9;
	mu_m  = 3.2e9;
	rho_m = 2.65e3;

	# Fluids
	k_o   = 1.50e9;  # SAE 30 Oil, source: www.engineeringtoolbox.com
	k_w   = 2.34e9;  # Seawater, source: www.engineeringtoolbox.com
	rho_o = 0.89e3;  # SAE 30 Oil, source: www.engineeringtoolbox.com
	rho_w = 1.010e3; # Seawater, source: www.engineeringtoolbox.com

	active_size = z.dims[3]
	print "Active size: %d" % active_size
	print "Grid active size: %d" % z.grid.get_active_size()
	print "Grid global size: %d" % z.grid.get_global_size()

	print z.h["PORO"].get_size()
	print z.h["SGAS"].get_size()

	start = time.clock()
	for n in xrange(0, active_size):
		coords = grid.get_ijk1A(n)
		poro = grid.get_property(z.h["PORO"], coords)
	# Sand
	k_m   = 18.0e9;
	mu_m  = 3.2e9;
	rho_m = 2.65e3;

	# Fluids
	k_o   = 1.50e9;  # SAE 30 Oil, source: www.engineeringtoolbox.com
	k_w   = 2.34e9;  # Seawater, source: www.engineeringtoolbox.com
	rho_o = 0.89e3;  # SAE 30 Oil, source: www.engineeringtoolbox.com
	rho_w = 1.010e3; # Seawater, source: www.engineeringtoolbox.com
	s = grid.get_property(z.h["SGAS"], coords)

	rho_res = poro*s*rho_o+poro*(1-s)*rho_w+(1-poro)*rho_m
	mu_res = (1-poro)*mu_m

	if poro == 0:
		print "Poro was zero!"
		k_res = k_m
	else:
		k_rockframe = (1-poro)*k_m
		k_fluid = 1/((s/k_o)+(1-s)/k_w)
		b = k_rockframe/(k_m-k_rockframe)+k_fluid/(poro*(k_m-k_fluid))
		k_res = (b*k_m)/(1 + b)

	vp = sqrt((k_res+(4/3)*mu_res)/rho_res)
	vs = sqrt(mu_res/rho_res)
#		print "k_res: %f, mu_res: %f, rho_res: %f, vp: %f, vs: %f" % (k_res, mu_res, rho_res, vp, vs)
	
	print "Finished calculating vp/vs for grid!"
	print "Time elapsed = ", time.clock() - start, "seconds"


			
if __name__ == '__main__':
	grid_file = '/private/masar/enkf/devel/Testcases/Common/ECLIPSE/EXAMPLE_01_BASE.EGRID'
	restart_file = '/private/masar/enkf/devel/Testcases/SimpleEnKF/Simulations/Real0/EXAMPLE_01_BASE_0000.X0039'
	init_file = "/private/masar/enkf/devel/Testcases/SimpleEnKF/Simulations/Real0/EXAMPLE_01_BASE_0000.INIT"
	z = zone(grid_file, restart_file, init_file, ["SGAS", "PORO", "SWAT"])
##	z = zone(grid_file, restart_file, init_file)
##	z.fill_data(square)
#	volume_map(z, "PORO")
	#gassmann(z)
	z.iterate_grid(gassmann_2, ["SGAS", "PORO"])
	z.grid.summarize()
