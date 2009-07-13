#!/usr/bin/env python
from ecl import *
import pprint as pp
import time as tt
import os
import sys
from math import *
import ConfigParser
sys.path.append('/private/masar/numpy/lib64/python2.3/site-packages')
import numpy as np
from itertools import *


class zone(object):
	def __init__(self, config, restart_file):
		try:
			path = config.get('project', 'path')
			grid_file = path + config.get('project', 'grid_file')
			init_file = path + config.get('project', 'init_file')
			self.output = config.getboolean('script', 'output')
		except (ConfigParser.NoSectionError, ConfigParser.NoOptionError), e:
			raise e
		
		if self.output: print "Loading zone with %s." % restart_file
		self.grid = ecl_grid(grid_file)
		self.init = ecl_file(init_file)
		self.restart = ecl_file(path + restart_file)
		self.dims = self.grid.get_dims()
		self.global_size = self.grid.get_global_size()
		self.config = config

	def get_keyword_data(self, kw, index):
		init = self.init
		restart = self.restart
					
		if init.has_kw(kw):
			kw_type = init.iget_named_kw(kw, 0)
			return kw_type.iget_data(index)
		
		if restart.has_kw(kw):
			kw_type = restart.iget_named_kw(kw, 0)
			return kw_type.iget_data(index)
	
		raise "Error: could not find keyword: '%s'" % kw

	def iter_grid(self, func):
		grid = self.grid
		global_size = self.global_size
		elements_from_func = 3
		
		self.k_m = config.getfloat('gassmann', 'k_m')
		self.mu_m = config.getfloat('gassmann', 'mu_m')
		self.rho_m = config.getfloat('gassmann', 'rho_m')
		self.poro_c = config.getfloat('gassmann', 'poro_critical')
		self.adiabatic_index = config.getfloat('gassmann', 'adiabatic_index')
		self.k_o = config.getfloat('gassmann', 'k_o')
		self.k_w = config.getfloat('gassmann', 'k_w')

		for j in xrange(0, global_size):
						ret = grid.get_active_index1(j)
						if ret == -1:
										yield ([float(0)]*elements_from_func)
						else:
										yield func(self, ret)



def gassmann(z, j):
	pressure = z.get_keyword_data("PRESSURE", j)
	sgas = z.get_keyword_data("SGAS", j)
	swat = z.get_keyword_data("SWAT", j)
	poro = z.get_keyword_data("PORO", j)
	rho_o = z.get_keyword_data("OIL_DEN", j)
	rho_g = z.get_keyword_data("GAS_DEN", j)
	rho_w = z.get_keyword_data("WAT_DEN", j)
	
	k_m = z.k_m
	mu_m = z.mu_m
	rho_m = z.rho_m
	poro_c = z.poro_c
	adiabatic_index = z.adiabatic_index
	k_o = z.k_o
	k_w = z.k_w

	try:
					if swat < 0:
									swat = 0
					if sgas < 0:
									sgas = 0
					if swat + sgas > 1:
									sat_sum = (swat + sgas)
									swat = swat / sat_sum
									sgas = sgas / sat_sum
					soil = 1 - (swat + sgas)

					pressure = pressure*1e6
					k_g = pressure*adiabatic_index
					rho_res = poro*((sgas * rho_g) + (swat * rho_w) + (soil * rho_o)) + (1 - poro) * rho_m
					if poro > poro_c: 
									poro_c = 1
					k_dry = (1 - poro/poro_c) * k_m
					mu_dry = (1 - poro/poro_c) * k_m
					mu_res = (1 - poro/poro_c) * mu_m
					c_fluid = ((1/k_o) * soil) + ((1/k_w) * swat) + ((1/k_g) * sgas)
					k_fluid = 1 / c_fluid

					if poro * (k_m - k_fluid) == 0 or k_m - k_dry == 0:
									print "Warning: setting k_reservoir = k_mineral = %f\n" % k_m
									k_res = k_m
					else:
									b = (k_dry / (k_m - k_dry)) + (k_fluid / (poro * (k_m - k_fluid)))
									k_res = (b * k_m) / (1 + b)

					v_p = sqrt((k_res + (4/3) * mu_res) / rho_res)
					v_s = sqrt(mu_res / rho_res)
					z_p = rho_res * v_p


	except ArithmeticError, e:
					print "ArithmeticError: %s for global index %d" % (e, j)
					raise
	else:
					return (v_p, v_s, z_p)


if __name__ == '__main__':
	try:
					assert (len(sys.argv) == 2) 
	except AssertionError:
					print 'Usage: %s <config>' % (sys.argv[0]) 
					sys.exit()
	try:
					config_file = sys.argv[1]
					fp = open(config_file)
	except IOError, e:
					print "Unable to open '%s': %s" % (config_file, e)
					sys.exit()

	config = ConfigParser.ConfigParser()
	config.readfp(fp)
	restart_base_file = config.get('project', 'restart_base')
	restart_mon_file = config.get('project', 'restart_mon')
	output_file = config.get('project', 'output_file')

	base = zone(config, restart_base_file)
	mon = zone(config, restart_mon_file)

	keywords = ('VP_BASE', 'VS_BASE', 'VP_MON', 'VS_MON', 'VP_DIFF', 'VS_DIFF', 'Z_P_DIFF')
	narr = np.zeros((base.global_size, len(keywords)));


	print "Iterating grids..."
	k = 0
	for t_base, t_mon in izip(base.iter_grid(gassmann), 
									mon.iter_grid(gassmann)):
					narr[k, 0] = t_base[0]
					narr[k, 1] = t_base[1]
					narr[k, 2] = t_mon[0]
					narr[k, 3] = t_mon[1]
					narr[k, 4] = t_mon[0] - t_base[0]
					narr[k, 5] = t_mon[1] - t_base[1]
					narr[k, 6] = t_mon[2] - t_base[2]
					k += 1

	print "Writing output file: '%s'" % output_file
	k = ecl_kw()
	for j, val in enumerate(keywords):
					k.write_new_grdecl(output_file, val, narr[:,j].tolist())
	print "Complete!"


	
