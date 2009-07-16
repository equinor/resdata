from libecl import *
import pprint as pp
import sys
import time

####################################################################
##
## class: Zone
##        - Loads the ECL files, has methods for grabbing ECL
##          data, and iterating the grid.
##        - Contains empty cache variables until they are filled
##          by the Zone_Cache class.
##
## class: Gassmann (inherits Zone)
##        - Defines function pointer doing calculations
##        - Load its own personal configuration options.
##        - The returned dictionary from the function pointer
##          defines what keywords the zone has available.
##  
## class: Zone_Cache
##        - Stores lists of the data, for easy access when comparing
##          different zones and doing arithmetic operations.
##        - The objects stored in the cache is also writeable to
##          either .grdecl format or .dat format.
##
####################################################################

class Zone_Cache:

  def __init__(self, grid_file):
    self.grid = ecl_grid(grid_file)
    self.active_size = self.grid.get_active_size()
    self.global_size = self.grid.get_global_size()

    self.zones = list()
    self.cache = dict()

  def convert_list(self, l_a, dummy_val = 0):
    k = 0
    l_g = list();
    for j in xrange(0, self.global_size):
      ret = self.grid.get_active_index1(j)
      if ret == -1:
        l_g.append(float(dummy_val))
      else:
        l_g.append(l_a[k])
        k += 1

    return l_g

  #############################################################
  ##
  ## The following are methods working with a ZONE.
  ## The keywords from where the data is stored comes from
  ## the dictionary beeing returned form the functions pointer
  ## defined by the child class for the ZONE.
  ##
  #############################################################

  def addzone(self, *zones):
    for zone in zones:
      print "adding ...", zone
      self.zones.append(zone)
      for j in xrange(0, self.global_size):
        ret = zone.grid.get_active_index1(j)
        if ret != -1:
          dic = zone.func(ret)
          zone.active_cache.append(dic)
          if len(zone.active_cache) == 1:
            zone.kw_keys = dic.keys()

  def get_active_list(self, zone, keyword):
    retlist = list()
    for val in zone.active_cache:
      retlist.append(val[keyword])
    return retlist

  def write_zone_grdecl(self, zone, file):
    if zone not in self.zones:
      raise "the zone you asked for has not been added!"

    if len(zone.kw_keys) == 0:
      print "You need to iterate or cache the grid first!"
   
    print "Writing %d ZONE variables to grdecl file '%s'" % (len(zone.kw_keys), file)
    k = ecl_kw()
    for val in zone.kw_keys:
      l_a = self.get_active_list(zone, val)
      l_g = self.convert_list(l_a)
      k.write_new_grdecl(file, val, l_g)
      
  def write_zone_datfiles(self, zone):
    path = 'zone_output/'
    for val in zone.kw_keys:
      file_string = path + "%s.dat" % val
      l_a = self.get_active_list(zone, val)

      for j, val in enumerate(l_a):
        q = str(val)
        l_a[j] = "%s\n" % q
        
      print "Writing Matlab export '%s'" % file_string
      f = open(file_string, 'w')
      f.writelines(l_a)
      f.close()

  ##########################################################
  ##
  ## The following are methods are for working with newly 
  ## calculated objects that only exists in the CACHE class.
  ##
  ############################################################
  
  def add_keyword(self, kw, new_list):
    self.cache[kw] = new_list

  def write_grdecl(self, file):
    print "Writing %d CACHE variables to grdecl file '%s'" % (len(self.cache.keys()), file)
    k = ecl_kw()
    for val in self.cache.keys():
      l_g = self.convert_list(self.cache[val])
      k.write_new_grdecl(file, val, l_g)
      
  def write_datfiles(self):
    path = 'zone_output/'
    for val in self.cache.keys():
      file_string = path + "%s.dat" % val

      l_a = list()
      for val in self.cache[val]:
        q = str(val)
        string = "%s\n" % q
        l_a.append(string)
        
      print "Writing Matlab export '%s'" % file_string
      f = open(file_string, 'w')
      f.writelines(l_a)
      f.close()
    pass
    

class Zone:
  def __init__(self):
    try:
      assert self.grid_file != None, 'grid file not set!'
      assert self.init_file != None, 'init file not set!'
      assert self.restart_file != None, 'restart file not set!'
    except AssertionError, e:
      print "Error: %s, parameter required for Zone class!" % e
      sys.exit()
    self.grid = ecl_grid(self.grid_file)
    self.init = ecl_file(self.init_file)
    self.restart = ecl_file(self.path + self.restart_file)
    self.dims = self.grid.get_dims()
    self.active_size = self.dims[3]
    self.global_size = self.grid.get_global_size()
    
    self.active_cache = list()
    self.global_cache = list()

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

  def iter_grid(self, ret_active = 0, cache = 0):
    print "Start iterating grid."
    for j in xrange(0, self.global_size):
      ret = self.grid.get_active_index1(j)
      if ret == -1:
        if ret_active == 0:
          yield None
      else:
        dic = self.func(ret)
        if cache:
          self.active_cache.append(ret)
          if len(self.active_cache) == 1:
            self.kw_keys = dic.keys()
          
        yield dic





####################################################################
##
## A thin SWIG wrapper starts from here to bottom!
## 
####################################################################


class ecl_summary:
	def __init__(self, ecl_data_file):
		self.ecl_data_file = ecl_data_file
		self.endian_convert = 1;
		s = ecl_sum_fread_alloc_case(ecl_data_file,self.endian_convert)
		self.s = s
	def __del__(self):
		print "Del ecl_summary object"
		ecl_sum_free(self.s)

	def display(self, kw): 
		report_only = 1
		ecl_sum_fprintf(self.s, sys.stdout, len(kw), kw, report_only)

	def get_start_time(self):
		res = ecl_sum_get_start_time(self.s)
		return time.gmtime(res)
	def get_simulation_case(self):
		return ecl_sum_get_simulation_case(self.s)

	def get_first_ministep(self):
		return ecl_sum_get_first_ministep(self.s)
	def get_last_ministep(self):
		return ecl_sum_get_last_ministep(self.s)
	def has_ministep(self, ministep):
		return ecl_sum_has_ministep(self.s, ministep)
	
	def get_first_report_step(self):
		return ecl_sum_get_first_report_step(self.s)
	def get_last_report_step(self):
		return ecl_sum_get_last_report_step(self.s)
	def has_report_step(self, report_step):
		return ecl_sum_has_report_step(self.s, report_step)

	def get_well_var(self, ministep, well, var):
		return ecl_sum_get_well_var(self.s, ministep, well, var)
	def get_well_var_index(self, well, var):
		return ecl_sum_get_well_var_index(self.s, well, var)
	def has_well_var(self, well, var):
		return ecl_sum_has_well_var(self.s, well, var);

	def get_group_var(self, ministep, group, var):
		return ecl_sum_get_group_var(self.s, ministep, group, var)
	def group_var_index(self, group, var):
		return ecl_sum_get_group_var_index(self.s, group, var)
	def has_group_var(self, group, var):
		return ecl_sum_has_group_var(self.s, group, var)

	def get_field_var(self, ministep, var):
		return ecl_sum_get_field_var(self.s, ministep, var)
	def field_var_index(self, var):
		return ecl_sum_get_field_var_index(self.s, var)
	def has_field_var(self, var):
		return ecl_sum_has_field_var(self.s, var)

	def get_block_var(self, ministep, block_var, block_nr):
		return ecl_sum_get_block_var(self.s, ministep, block_var, block_nr)
	def get_block_var_index(self, block_var, block_nr):
		return ecl_sum_get_block_var_index(self.s, block_var, block_nr)
	def has_block_var(self, block_var, block_nr):
		return ecl_sum_has_block_var(self.s, block_var, block_nr)
	def get_block_var_ijk(self, ministep, block_var, i, j, k):
		return ecl_sum_get_block_var_ijk(self.s, ministep, block_var, i, j, k)
	def get_block_var_index_ijk(self, block_var, i, j, k):
		return ecl_sum_get_block_var_index_ijk(self.s, block_var, i, j, k)
	def has_block_var_ijk(self, block_var, i, j, k):
		return ecl_sum_has_block_var_ijk(self.s, block_var, i, j, k)

	def get_region_var(self, ministep, region_nr, var):
		return ecl_sum_get_region_var(self.s, ministep, region_nr, var)
	def get_region_var_index(self,  region_nr, var):
		return ecl_sum_get_region_var_index(self.s,  region_nr, var)
	def has_region_var(self, region_nr, var):
		return ecl_sum_has_region_var(self.s, region_nr, var)

	def get_misc_var(self, ministep, var):
		return ecl_sum_get_misc_var(self.s, ministep, var)
	def get_misc_var_index(self, var):
		return ecl_sum_get_misc_var_index(self.s, var)
	def has_misc_var(self, var):
		ecl_sum_has_misc_var(self.s, var)
		
	def get_well_completion_var(self, ministep, kw):
		return ecl_sum_get_well_completion_var(self.s, ministep, kw)
	def get_well_completion_var_index(self, well, var, cell_nr):
		return ecl_sum_get_well_completion_var_index(self.s,well,var,cell_nr)
	def has_well_completion_var(self, well, var, cell_nr):
		return ecl_sum_has_well_completion_var(self.s,well,var,cell_nr)

	def get_general_var(self, ministep, kw):
		return ecl_sum_get_general_var(self.s, ministep, kw)
	def get_general_var_index(self, kw):
		return ecl_sum_get_general_var_index(self.s, kw)
	def has_general_var(self, kw):
		return ecl_sum_has_general_var(self.s, kw);

class ecl_file:
	def __init__(self, filename):
		self.filename = filename
		self.endian_flip = 1
		f = ecl_file_fread_alloc(filename, self.endian_flip)
		self.f = f
	def __del__(self):
		ecl_file_free(self.f)

	def iget_named_kw(self, kw, ith):
		ret = ecl_file_iget_named_kw(self.f, kw, ith)
		return ecl_kw(ret)
	def iget_kw(self, index):
		ret = ecl_file_iget_kw(self.f, index)
		return ecl_kw(ret)
	def has_kw(self, kw):
		return ecl_file_has_kw(self.f, kw)
	def get_num_distinct_kw(self):
		return ecl_file_get_num_distinct_kw(self.f)
	def iget_distinct_kw(self, index):
		return ecl_file_iget_distinct_kw(self.f, index)
	def get_num_named_kw(self, kw):
		return ecl_file_get_num_named_kw(self.f, kw)
	def get_num_kw(self):
		return ecl_file_get_num_kw(self.f)
	
		
class ecl_grid:
	def __init__(self, grid_file):
		endian_flip = 1
		self.grid_file = grid_file
		self.g = ecl_grid_alloc(grid_file, endian_flip);
	def __del__(self):
		ecl_grid_free(self.g)

	def get_name(self):
		return ecl_grid_get_name(self.g)
	def get_dims(self):
		# (i, j, k, active_size)
		return ecl_grid_get_dims(self.g)
	def get_ijk1(self, global_index):
		# (i, j, k)
		return ecl_grid_get_ijk1(self.g, global_index)
	def get_ijk1A(self, active_index):
		return ecl_grid_get_ijk1A(self.g, active_index)
	def get_global_index3(self, i, j, k):
		return ecl_grid_get_global_index3(self.g, i, j, j)
	def get_property(self, kw_obj, ijk):
		return ecl_grid_get_property(self.g, kw_obj.k, ijk[0], ijk[1], ijk[2])
	def get_active_size(self):
		return ecl_grid_get_active_size(self.g)
	def get_global_size(self):
		return ecl_grid_get_global_size(self.g);
	def get_active_index1(self, global_index):
		return ecl_grid_get_active_index1(self.g, global_index)

	def summarize(self):
		ecl_grid_summarize(self.g);
		
class ecl_kw:
	def __init__(self, k = None):
		self.k = k
		self.w = None
	def get_header_ref(self):
		return ecl_kw_get_header_ref(self.k)
	def get_str_type_ref(self):
		return ecl_kw_get_str_type_ref(self.k)
	def get_size(self):
		return ecl_kw_get_size(self.k);
	def get_data(self):
		self.w = ecl_kw_get_data_wrap_void(self.k)
		list = get_ecl_kw_data_wrapper_void_list(self.w)
		return list
	def iget_data(self, index):
		return ecl_kw_iget_as_double(self.k, index);
#		return ecl_kw_iget_ptr_wrap(self.k, index)
	def write_new_grdecl(self, filename, kw, list):
		size = len(list);
		mode = "w"

		if self.k == None:
			self.k = ecl_kw_alloc_empty()
		else:
			mode = "a"
			ecl_kw_free_data(self.k)

		ecl_kw_set_header(self.k, kw, size, "REAL")
		ecl_kw_alloc_float_data(self.k, list)
		
		f = open(filename, mode)
		ecl_kw_fprintf_grdecl(self.k, f)
		f.close
	def fread_alloc(self, fortio_type):
		return ecl_kw_fread_alloc(fortio_type)
	def fseek_kw(self, kw, fortio_type):
		return ecl_kw_fseek_kw(kw, 1, 0, fortio_type)
	def fread_realloc(self, kw_type, fortio_type):
		self.k = kw_type
		return ecl_kw_fread_realloc(kw_type, fortio_type)


class fortio:
	def __init__(self, fortio_file = None):
		self.fds = []

		if fortio_file is not None:
			print "Gikk her"
			self.fortio_file = fortio_file
			self.fortio = self.open(fortio_file)

	def __del__(self):
		print "Deleting ecl_fortio object"

	def open(self, fortio_file):
		endian_convert = 1
		self.fmt_src = ecl_util_fmt_file(fortio_file)

		fortio = fortio_fopen(fortio_file, "rw", endian_convert, self.fmt_src)
		return fortio
	def close(self):
		fortio_fclose(self.fortio)

	def read_data(self, kw):
		kw_type = ecl_kw_fread_alloc(self.fortio)
		if ecl_kw_fseek_kw(kw, 1, 0, self.fortio):
			ecl_kw_fread_realloc(kw_type, self.fortio)
		w = ecl_kw_get_data_wrap_void(kw_type)
		list = get_ecl_kw_data_wrapper_void_list(w)
		ecl_kw_free(kw_type)
		return list
	def write_data(self, kw, data):
		kw_type = ecl_kw_alloc_empty()
		ecl_kw_set_header_alloc(kw_type, kw, len(kw), "INTE")
		print ecl_kw_get_header_ref(kw_type)
		print ecl_kw_get_str_type_ref(kw_type)

		print "Writing data to file"
	
class ecl_util:
	def get_file_type(self, file):
		e = ecl_util_get_file_type(file);
		self.file_type = e[0]
		self.fmt_file = e[1]
		self.report_nr = e[2]



