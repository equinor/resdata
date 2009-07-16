#!/usr/bin/env python
from ecl import *
from math import *
import sys
import ConfigParser
from itertools import *


class Gassmann(Zone):
  def __init__(self, config, restart_file):
    self.restart_file = restart_file
    self.grid_file = None
    self.init_file = None
    self.func = self.gassmann
    
    try:
      self.path = config.get('project', 'path')
      self.grid_file = self.path + config.get('project', 'grid_file')
      self.init_file = self.path + config.get('project', 'init_file')
			
      self.k_m = config.getfloat('gassmann', 'k_m')
      self.mu_m = config.getfloat('gassmann', 'mu_m')
      self.rho_m = config.getfloat('gassmann', 'rho_m')
      self.poro_c = config.getfloat('gassmann', 'poro_critical')
      self.adiabatic_index = config.getfloat('gassmann', 'adiabatic_index')
      self.k_o = config.getfloat('gassmann', 'k_o')
      self.k_w = config.getfloat('gassmann', 'k_w')

    except (ConfigParser.NoSectionError, ConfigParser.NoOptionError), e:
      raise e
    
    Zone.__init__(self)
    
  def gassmann(self, active_index):
    pressure = self.get_keyword_data("PRESSURE", active_index)
    sgas = self.get_keyword_data("SGAS", active_index)
    swat = self.get_keyword_data("SWAT", active_index)
    poro = self.get_keyword_data("PORO", active_index)
    rho_o = self.get_keyword_data("OIL_DEN", active_index)
    rho_g = self.get_keyword_data("GAS_DEN", active_index)
    rho_w = self.get_keyword_data("WAT_DEN", active_index)
    k_m = self.k_m
    mu_m = self.mu_m
    rho_m = self.rho_m
    poro_c = self.poro_c
    adiabatic_index = self.adiabatic_index
    k_o = self.k_o
    k_w = self.k_w
    
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

    except ArithmeticError, e:
			print "ArithmeticError: %s for global index %d" % (e, j)
			raise
    else:
      ret = dict(GM_VP = v_p, GM_VS = v_s, SGAS = float(sgas))

      return ret


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
  try:
    restart_base_file = config.get('project', 'restart_base')
    restart_mon_file = config.get('project', 'restart_mon')
    output_file = config.get('project', 'output_file')
    matlab_export = config.getboolean('script', 'matlab_export')
  except (ConfigParser.NoSectionError, ConfigParser.NoOptionError), e:
    raise e

  base = Gassmann(config, restart_base_file)
  mon = Gassmann(config, restart_mon_file)
  another = Gassmann(config, "RG01-STRUCT-24.X0250")

  cache = Zone_Cache(base.grid_file)
  cache.addzone(base, mon, another)

  a = cache.get_active_list(base, 'GM_VP')
  b = cache.get_active_list(mon, 'GM_VP')
  c = cache.get_active_list(mon, 'GM_VP')

  diff = [tmp2 - tmp1 for tmp1, tmp2 in zip(a, b)]
  mult = [tmp2 + tmp1 + tmp3 for tmp1, tmp2, tmp3 in zip(a, b, c)]
  mult2 = [tmp2 + tmp1 for tmp1, tmp2 in zip(a, b)]
  
  a = cache.get_active_list(base, 'SGAS')
  b = cache.get_active_list(mon, 'SGAS')
  sgas_diff = [tmp2 - tmp1 for tmp1, tmp2 in zip(a, b)]
  
  cache.add_keyword("GM_VP_D", diff)
  cache.add_keyword("GM_SG_D", sgas_diff)
  cache.add_keyword("GM_VP_M", mult)
  cache.add_keyword("GM_VP_M2", mult2)
  
  cache.write_grdecl("grane_cache.GRDECL")
  cache.write_datfiles()

  cache.write_zone_grdecl(base, output_file)
  cache.write_zone_datfiles(base)

