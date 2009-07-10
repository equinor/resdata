#!/usr/bin/env python
from ecl import *
import pprint as pp
import time as tt
import os
import sys
from math import *


# TODO:
# - Lese configfil
# - Spytte ut alt i en eclgrd fil.
# - Beregne posissons ratio, akustisk impendans p/s, total reservoir tetthet.

class zone(object):
	def __init__(self, grid_file, restart_file, init_file):
		self.grid = ecl_grid(grid_file)
		self.init = ecl_file(init_file)
		self.restart = ecl_file(restart_file)
		self.dims = self.grid.get_dims()

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
		active_size = self.dims[3]
		grid = self.grid
		l = list()
		for j in xrange(0, grid.get_global_size()):
						ret = grid.get_active_index1(j)
						if ret == -1:
										tuple = (float(0), float(0))
						else:
										tuple = func(self, ret)
						l.append(tuple)

		return l



def gassmann_first_test(z, j):
	# Sand
	k_m   = 18.0e9;
	mu_m  = 3.2e9;
	rho_m = 2.65e3;
	rho_m = 1.85e3;
	
	# The rockphysics handbook p. 242 (critical porosity model)
	# Sandstone critical poro (reuss dry rock moduli)
	poro_c = 0.5

  # Adiabatic index for calculating bulk moduli for gas	
	# C_3 H_8 (Propane) at 16 C
	adiabatic_index = 1.130

	# Fluids
	k_o   = 1.50e9;  # SAE 30 Oil,
	k_w   = 2.34e9;  # Seawater
	
	pressure = z.get_keyword_data("PRESSURE", j)
	sgas = z.get_keyword_data("SGAS", j)
	swat = z.get_keyword_data("SWAT", j)
	poro = z.get_keyword_data("PORO", j)
	rho_o = z.get_keyword_data("OIL_DEN", j)
	rho_g = z.get_keyword_data("GAS_DEN", j)
	rho_w = z.get_keyword_data("WAT_DEN", j)

	# PUNQS3
#	rho_o = 0.89e3
#	rho_w = 1.010e3
#	rho_g = 0.100e3

	#print "sgas: %f, poro: %f, rho_o: %f, rho_w: %f, rho_g: %f, pressure: %f" % (sgas, poro, rho_o, rho_w, rho_g, pressure)
#	print sgas, poro, rho_o, rho_w, rho_g, pressure

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

					# Convert from bar to pascal
					pressure = pressure*1e6
					k_g = pressure*adiabatic_index
					rho_res = poro*((sgas * rho_g) + (swat * rho_w) + (soil * rho_o)) + (1 - poro) * rho_m
					if poro > poro_c: 
									poro_c = 1
									
					k_dry = (1 - poro/poro_c) * k_m
					mu_dry = (1 - poro/poro_c) * k_m
					
					vp_dry = sqrt((k_dry + (4/3) * mu_dry) / (rho_m))
#					print "vp_dry: %f" % vp_dry

					mu_res = (1 - poro/poro_c) * mu_m

					c_fluid = ((1/k_o) * soil) + ((1/k_w) * swat) + ((1/k_g) * sgas)
					k_fluid = 1 / c_fluid
#					k_fluid = 1 / ((soil / k_o) + (swat / k_w) + (sgas / k_g))

					if poro * (k_m - k_fluid) == 0 or k_m - k_dry == 0:
									print "Warning: setting k_reservoir = k_mineral = %f\n" % k_m
									k_res = k_m
					else:
#									b = (k_dry / (k_m - k_dry)) + (k_fluid / (poro * (k_m - k_fluid)))
#									k_res = (b * k_m) / (1 + b)

									b = 1 - (k_dry / k_m)
									k_res = k_dry + ( (b*b) / ( (poro / k_fluid) + ((1 - poro) / k_m) - (k_dry / (k_m * k_m))))

#					print "mu_res: %f, rho_res: %f, k_dry: %f, k_fluid: %f, k_res: %f" % (mu_res, rho_res, k_dry, k_fluid, k_res)
					vp = sqrt((k_res + (4/3) * mu_res) / rho_res)
					vs = sqrt(mu_res / rho_res)

					##print vp, sgas

	except ArithmeticError, e:
					print "ArithmeticError: %s for global index %d" % (e, j)
					raise
	else:
					return (vp, vs)


if __name__ == '__main__':
	path = '/d/felles/bg/scratch/masar/STRUCT/realization-24-step-0-to-358/'
	grid_file = path + 'RG01-STRUCT-24.EGRID'
	restart_files = (path + 'RG01-STRUCT-24.X0050', path + 'RG01-STRUCT-24.X0358')
	init_file = path + 'RG01-STRUCT-24.INIT'
	output_files = ('./test_vp1.GRDECL', './test_vp2.GRDECL', './test_diff.GRDECL')
	output_kws = ('VP1', 'VP2', 'VPDIFF')

#	path = '/d/proj/bg/enkf/jaskje/EnKF_PUNQS3/PUNQS3/'
#	grid_file = path + 'PUNQS3.EGRID'
#	restart_files = (path + 'PUNQS3.X0001', path + 'PUNQS3.X0083')
#	init_file = path + 'PUNQS3.INIT'

	print "[Zone 1] loading"
	z = zone(grid_file, restart_files[0], init_file)
	print "[Zone 1] active size: %d, global size: %d" % (z.grid.get_active_size(), z.grid.get_global_size())
	print "[Zone 1] Iterating grid"
	gm1 = z.iter_grid(gassmann_first_test)
	tt.sleep(2)

	print "[Zone 2] Loading"
	print "[Zone 2] active size: %d, global size: %d" % (z.grid.get_active_size(), z.grid.get_global_size())
	z = zone(grid_file, restart_files[1], init_file)
	print "[Zone 2] Iterating grid"
	gm2 = z.iter_grid(gassmann_first_test)
	print "--------------"
	
	try:
					assert len(gm1) == z.grid.get_global_size()
					assert len(gm2) == len(gm1)
	except AssertionError, e:
					print "Error: Different lengths of the vp/vs list!"
					raise 

	vp1_list = list();
	vp2_list = list();
	diff_list = list();
	vs1_list = list();
	vs2_list = list();
	diff_vs_list = list();
	for j in xrange(0, len(gm1)):
			vp1, vs1 = gm1[j]
			vp1_list.append(vp1)
			vs1_list.append(vs1)
			vp2, vs2 = gm2[j]
			vs2_list.append(vs2)
			vp2_list.append(vp2)
			diff_list.append(vp2 - vp1)
			diff_vs_list.append(vs2 - vs1)

	print "Writing GRDECL file(s) ..."
	k = ecl_kw()
	print "Writing (%s) %s, min/max %f/%f" % (output_kws[0], output_files[0], min(vp1_list), max(vp1_list))
	k.write_new_grdecl(output_files[0], output_kws[0], vp1_list)
	
	print "Writing (%s) %s, min/max %f/%f" % (output_kws[1], output_files[1], min(vp2_list), max(vp2_list))
	k.write_new_grdecl(output_files[1], output_kws[1], vp2_list)
	
	print "Writing (%s) %s, min/max %f/%f" % (output_kws[2], output_files[2], min(diff_list), max(diff_list))
	k.write_new_grdecl(output_files[2], output_kws[2], diff_list)

	k.write_new_grdecl("VS2.GRDECL", "VS2", vs2_list)
	k.write_new_grdecl("VS1.GRDECL", "VS1", vs1_list)
	k.write_new_grdecl("DIFF_VS.GRDECL", "DIFF_VS", diff_vs_list)
