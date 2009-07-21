from libecl import *
import pprint as pp
import sys
import time
import os

####################################################################
##
## Here is a small overview of the classes and how it works together.
##
##
####################################################################

class AbstractMethod (object):
  def __init__(self, func):
    self._function = func

  def __get__(self, obj, type):
    return self.AbstractMethodHelper(self._function, type)

  class AbstractMethodHelper (object):
    def __init__(self, func, cls):
      self._function = func
      self._class = cls

    def __call__(self, *args, **kwargs):
      raise TypeError('Abstract method `' + self._class.__name__ \
          + '.' + self._function + '\' called')
  
class Rockphysics(object):
  apply = AbstractMethod('apply')

            
class Zone:
  def __init__(self, grid_file, keywords = None, *arglist):
    try:
      assert grid_file != None, 'grid file not set!'
    except AssertionError, e:
      print "Error: %s, parameter required for Zone class!" % e
      sys.exit()
      
    self.grid_file = grid_file
    self.grid = ecl_grid(grid_file)
    ecl_file_list = list()
    for val in arglist:
      ecl_file_list.append(ecl_file(val))
    print self 
    self.keywords = keywords
    self.cache = dict()
    self.cache_list(ecl_file_list)

  def __sub__(self, zone):
    if self.grid_file is not zone.grid_file:
      raise "Grid files are not the same in the substraction!"

    print self, zone

    mon = self
    base = zone
   
    new_zone = Zone(self.grid_file)
    for key in mon.cache.keys():
      if key in base.cache.keys():
        print "Doing sub for '%s'" % key
        diff = list()
        diff = [b - a for a, b in zip(base.cache[key], mon.cache[key])]
#        new_key = "DIFF_%s" % key
        new_zone.cache[key] = diff

    return new_zone

  ## This functions takes an active idnex list and 
  ## returns a global index list.
  def convert_list(self, l_a, dummy_val = 0):
    k = 0
    l_g = list();
    for j in xrange(0, self.grid.get_global_size()):
      ret = self.grid.get_active_index1(j)
      if ret == -1:
        l_g.append(float(dummy_val))
      else:
        l_g.append(l_a[k])
        k += 1

    return l_g

  def cache_list(self, ecl_file_list):
    for f in ecl_file_list:
      for j in xrange(f.get_num_distinct_kw()):
        str_kw = f.iget_distinct_kw(j)
        if str_kw in self.keywords:
          print "Adding keyword: '%s'" % str_kw
          kw_type = f.iget_named_kw(str_kw, 0)
          items = list()
          for i in xrange(0, self.grid.get_global_size()):
            ret = self.grid.get_active_index1(i)
            if ret != -1:
              data = kw_type.iget_data(ret)
              items.append(data)
          self.cache[str_kw] = items

  def get_data(self, kw, active_index):
    return self.cache[kw][active_index]

  def apply_function(self, obj):
    print "Applying function", obj
    for i in xrange(0, self.grid.get_global_size()):
      ret = self.grid.get_active_index1(i)
      if ret != -1:
        data = obj.apply(self, ret)
        for key in data.keys():
          if not self.cache.has_key(key):
            self.cache[key] = list()
           
          self.cache[key].append(data[key])

  def compute_differences(self, mon, base):
    for key in base.cache.keys():
      if key in mon.cache.keys():
        diff = list()
        diff = [b - a for a, b in zip(base.cache[key], mon.cache[key])]
        self.cache[key] = diff

  def get_keywords(self):
    return self.cache.keys()

  def append_keyword_to_grdecl(self, file, kw, new_kw = None):
    if os.path.isfile(file): mode = 'a'
    else: mode = 'w'
    if new_kw is None: write_kw = kw
    else: write_kw = new_kw
    
    print "Writing '%s' to '%s'" % (write_kw, file)
    l_g = self.convert_list(self.cache[kw])
    k = ecl_kw()
    k.write_new_grdecl(file, write_kw, l_g, mode)

  def append_keyword_to_dat(self, file, kw, new_kw = None):
    if os.path.isfile(file): mode = 'a'
    else: mode = 'w'
    if new_kw is None: write_kw = kw
    else: write_kw = new_kw

    l_a = list()
    for j, val in enumerate(self.cache[kw]):
      q = str(val)
      string = "%s\n" % q
      l_a.append(string)
      
    print "Writing '%s' to '%s'" % (write_kw, file)
    f = open(file, mode)
    f.writelines(l_a)
    f.close()

  def write_all_keywords_to_grdecl(self, file):
    print "Writing %d keywords to '%s'" % (len(self.cache.keys()), file)
    for key in self.cache.keys():
      l_g = self.convert_list(self.cache[key])
      k = ecl_kw()
      k.write_new_grdecl(file, key, l_g, 'a')


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
    return ecl_kw_get_size(self.k)
  def get_data(self):
    self.w = ecl_kw_get_data_wrap_void(self.k)
    list = get_ecl_kw_data_wrapper_void_list(self.w)
    return list
  def iget_data(self, index):
    return ecl_kw_iget_as_double(self.k, index)
  def write_new_grdecl(self, filename, kw, list, mode):
    size = len(list);
    self.k = ecl_kw_alloc_empty()
    
    # TODO: This currently only supports one type

    # The grdecl files will be of the format 
    # 0.15235971158633D+04    "DOUB"
    # 0.15235972E+04          "REAL"

		#ecl_kw_set_header(self.k, kw, size, "DOUB")
		#ecl_kw_alloc_double_data(self.k, list)
    ecl_kw_set_header(self.k, kw, size, "REAL")
    ecl_kw_alloc_float_data(self.k, list)
    f = open(filename, mode)
    ecl_kw_fprintf_grdecl(self.k, f)
    f.close
    ecl_kw_free_data(self.k)
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
    ## FIXME
    ## UNDER CONSTRUCTION AREA!
    ## 
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



