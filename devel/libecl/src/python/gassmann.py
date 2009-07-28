#!/usr/bin/env python
from math import *
import sys
import ConfigParser
from itertools import *
import getopt
from ecl import *
sys.path.append("/private/masar/numpy/lib64/python2.3/site-packages/")
import numpy as np

class Timeshift(Rockphysics):
  def __init__(self):
    self.nx = 0
    self.ny = 0
    self.nz = 0

  def apply(self, zone, active_index):
    dz = zone.get_data("DZ", active_index)
    vp = zone.get_data("VP", active_index)
    t = dz / vp
    return dict(TIME = t)

  def finish(self):
    pass


class Gassmann(Rockphysics):
  def __init__(self, config):
    try:
      self.k_m = config.getfloat('gassmann', 'k_m')
      self.rho_m = config.getfloat('gassmann', 'rho_m')
      self.k_o = config.getfloat('gassmann', 'k_o')
      self.k_w = config.getfloat('gassmann', 'k_w')
      self.k_g = config.getfloat('gassmann', 'k_g')
    except (ConfigParser.NoSectionError, ConfigParser.NoOptionError), e:
      raise e

  def apply(self, zone, active_index):
    sgas = zone.get_data("SGAS", active_index)
    swat = zone.get_data("SWAT", active_index)
    poro = zone.get_data("PORO", active_index)
    rho_o = zone.get_data("OIL_DEN", active_index)
    rho_g = zone.get_data("GAS_DEN", active_index)
    rho_w = zone.get_data("WAT_DEN", active_index)

    rho_m = self.rho_m
    k_m = self.k_m
    k_w   = self.k_w
    k_o   = self.k_o
    k_g  = self.k_g

    if poro > 0.4:
      print "Poro higher then critical poro for sand", poro

    (k_dry, mu_dry) = self.calculate_dry_velocities(poro)

    return self.calculate_saturated_velocities(swat, sgas, poro, rho_m, rho_o, rho_g, 
        rho_w, k_m, k_o, k_w, k_g, k_dry, mu_dry)
    

  def finish(self):
    print "Finished"

  def calculate_dry_velocities(self, poro):
    vp_dry = -4280 * poro + 4353
    vs_dry = -1993 * poro + 2449
    rho_dry = -2650 * poro + 2650
    k_dry = rho_dry * (pow(vp_dry, 2) - (4/3) * pow(vs_dry, 2))
    mu_dry = rho_dry * pow(vs_dry, 2)

    return (k_dry, mu_dry)


  def calculate_saturated_velocities(self, swat, sgas, poro, rho_m, rho_o, rho_g, 
      rho_w, k_m, k_o, k_w, k_g, k_dry, mu_dry):
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
      
      rho_sat = poro*((sgas * rho_g) + (swat * rho_w) + (soil * rho_o)) + (1 - poro) * rho_m
      mu_sat = mu_dry
      
      c_fluid = ((1/k_o) * soil) + ((1/k_w) * swat) + ((1/k_g) * sgas)
      k_fluid = 1 / c_fluid
      if poro * (k_m - k_fluid) == 0 or k_m - k_dry == 0:
        print "Warning: setting k_reservoir = k_mineral = %f\n" % k_m
        k_sat = k_m
      else:
        b = (k_dry / (k_m - k_dry)) + (k_fluid / (poro * (k_m - k_fluid)))
        k_sat = (b * k_m) / (1 + b)

      v_p = sqrt((k_sat + (4/3) * mu_sat) / rho_sat)
      v_s = sqrt(mu_sat / rho_sat)

    except ArithmeticError, e:
      print "ArithmeticError: %s for global index %d" % (e, j)
      raise
    else:
      return dict(VP = v_p, VS = v_s, SOIL = soil)

    
def display_usage():
    print 'Usage: %s --config=<config_file>' % (sys.argv[0]) 

    print """
    Options: 
    """

if __name__ == '__main__':
  try:
    opts, args = getopt.getopt(sys.argv[1:], "hc:i:f:t:", ["help", "config=", "iens=", "from=", "to="])
  except getopt.GetoptError, err:
    print str(err)
    display_usage()
    sys.exit(2)

  iens, report_from, report_to = None, None, None
  for o, a in opts:
    if o in ("-h", "--help"):
      display_usage();
      sys.exit(2)
    elif o in ("-c", "--config"):
      config_file = a
    elif o in ("-i", "--iens"):
      iens = int(a)
    elif o in ("-f", "--from"):
      report_from = int(a)
    elif o in ("-t", "--to"):
      report_to = int(a)
    else:
      assert False, "unhandled option"
  try:
    fp = open(config_file)
  except IOError, e:
    print "Unable to open '%s': %s" % (config_file, e)
    sys.exit()
  
  config = ConfigParser.ConfigParser()
  config.readfp(fp)
  try:
    path = config.get('project', 'path')
    grid_file = path + config.get('project', 'grid_file')
    init_file = path + config.get('project', 'init_file')
    restartfile_base = path + config.get('project', 'restart_base')
    restartfile_mon = path + config.get('project', 'restart_mon')
  except (ConfigParser.NoSectionError, ConfigParser.NoOptionError), e:
    raise e

  print "Loading the zones."
  keywords = ('PORO', 'PRESSURE', 'SGAS', 'SWAT', 'OIL_DEN', 'GAS_DEN', 'WAT_DEN')
  base = Zone(grid_file, keywords, init_file, restartfile_base)
  print "Applying Gassmann base calculation."
  base.apply_function(Gassmann(config))

  base.append_keyword_to_grdecl("VP.grdecl", "VP")
  base.load_from_grdecl("VP.grdecl", "VP_FOO")
  base.append_keyword_to_grdecl("VP_new.grdecl", "VP_FOO")

  
#  mon = Zone(grid_file, keywords, init_file, restartfile_mon)
#  print "Applying Gassmann monitor calculation."
#  mon.apply_function(Gassmann(config))
#  mon.append_keyword_to_grdecl("VP.grdecl", "VP", "VP_MON")

#  diff = mon - base
#  diff.append_keyword_to_grdecl("VP.grdecl", "VP", "VP_DIFF")

#  mon.write_keyword_to_dat("SGAS.dat", "SGAS")
#  mon.write_keyword_to_dat("SOIL.dat", "SOIL")
#  mon.write_keyword_to_dat("SWAT.dat", "SWAT")
#  mon.write_keyword_to_dat("PRESSURE.dat", "PRESSURE")
#  mon.write_keyword_to_dat("PORO.dat", "PORO")
#

# diff.write_all_keywords_to_roff("diff.roff")
#
#  keywords = ['DZ']
#  time_base = Zone(grid_file, keywords, init_file)
#  time_base.load(base, "VP")
#  time_mon = Zone(grid_file, keywords, init_file)
#  time_mon.load(mon, "VP")
#
#  print "Applying timeshift calculation."
#  time_base.apply_function(Timeshift())
#  time_mon.apply_function(Timeshift())
#
#  time_diff = time_mon - time_base
#
#  time_diff.delete("DZ")
#  # Load DZ that has not beeb subtracted
#  time_diff.load(time_base, "DZ")
#  
#  print "calculating cumsum for time zone"
#  time_diff.cumsum_dim_k()
#  
#  time_diff.write_all_keywords_to_roff("timeshift.roff")
#  time_diff.write_all_keywords_to_grdecl("timeshift.grdecl")
  
  



