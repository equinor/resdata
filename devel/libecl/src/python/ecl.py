from libecl import *
import pprint as pp
import sys
import time
import os
sys.path.append("/private/masar/numpy/lib64/python2.3/site-packages/")
import numpy as np


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
  finish = AbstractMethod('finish')


####################################################################
##
## For usage example look at gassmann.py
##
####################################################################
 
class Zone:
  ##
  # Initalize the Zone class, adds ecl_file objects for each
  # filename in the arglist and then caches the keywords given
  # in the keywords argument.
  ##
  def __init__(self, grid_file, keywords = list(), *arglist):
    try:
      assert grid_file != None, 'grid file not set!'
    except AssertionError, e:
      print "Error: %s, parameter required for Zone class!" % e
      return

    self.grid_file = grid_file
    self.grid = ecl_grid(grid_file)
    self.cache = dict()
    self.keywords = keywords

    ecl_file_list = list()
    for val in arglist: 
      ecl_file_list.append(ecl_file(val))
    self.cache_list(ecl_file_list)

  ## 
  # Operator overloading for subtraction.
  ##
  def __sub__(self, zone):
    if isinstance(self, Zone) and isinstance(zone, Zone):
      if self.grid_file is not zone.grid_file:
        raise "Grid files are not the same in the substraction!"

      new_zone = Zone(self.grid_file)
      for key in self.shared_keys(self, zone):
        diff = list()
        diff = [a - b for a, b in zip(self.cache[key], zone.cache[key])]
        new_zone.cache[key] = diff

    elif isinstance(self, Zone) and (isinstance(zone, int) 
        or isinstance(zone, float)):

      new_zone = Zone(self.grid_file)
      for key in self.cache.keys():
        diff = list()
        diff = [a - zone for a in self.cache[key]]
        new_zone.cache[key] = diff
    else:
      print "ERROR: One or more of the substraction types are not supported!"
      sys.exit()
    
    return new_zone


  ## 
  # Operator overloading for addition.
  ##
  def __add__(self, zone):
    if self.grid_file is not zone.grid_file:
      raise "Grid files are not the same in the addition!"

    new_zone = Zone(self.grid_file)
    for key in self.shared_keys(self, zone):
      print "Doing add for '%s'" % key
      add = list()
      add = [a + b for a, b in zip(self.cache[key], zone.cache[key])]
      new_zone.cache[key] = add
    return new_zone


  ## 
  # Operator overloading for multiplication.
  ##
  def __mul__(self, zone):
    if self.grid_file is not zone.grid_file:
      raise "Grid files are not the same in the multiplication!"

    new_zone = Zone(self.grid_file)
    for key in self.shared_keys(self, zone):
      print "Doing mult for '%s'" % key
      add = list()
      add = [a * b for a, b in zip(self.cache[key], zone.cache[key])]
      new_zone.cache[key] = add
    return new_zone


  ##
  # Calculate the cumulative sum for all gridcells, based on
  # summing over the columns in k direction.
  ##
  def cumsum_dim_k(self):
    (nx, ny, nz, a_size) = self.grid.get_dims()
    tmp = dict()

    for key in self.cache.keys():
      cells = np.zeros((nx, ny, nz))
      
      # Start by building a numpy matrix for all keywords.
      for index in xrange(0, self.grid.get_global_size()):
        ret = self.grid.get_active_index1(index)
        if ret != -1:
          (i, j, k) = self.grid.get_ijk1A(ret)
          active_index = self.grid.get_active_index1(index)
          cells[i][j][k] = self.cache[key][active_index]
      tmp[key] = cells
      
    for key in self.cache.keys():
      # Calculate the cumulative sum for each cell for each keyword.
      for i in xrange(0, nx):
        for j in xrange(0, ny):
          for k in xrange(0, nz):
            val = tmp[key][i, j, 0:k].sum()
            active_index = self.grid.get_active_index3(i, j, k)

            # Overwrite old keys.
            self.cache[key][active_index] = val

    
  ##
  # Takes two zones and then yields they common keys.
  ##
  def shared_keys(self, zone1, zone2):
    for key in zone1.cache.keys():
      if key in zone2.cache.keys():
        yield key


  ## 
  # Takes an active index list and returns a global index list.
  ##
  def convert_list(self, l_a, dummy_val = -1):
    k = 0
    l_g = list();

    if len(l_a) == self.grid.get_global_size():
      return l_a

    for j in xrange(0, self.grid.get_global_size()):
      ret = self.grid.get_active_index1(j)
      if ret == -1:
        l_g.append(float(dummy_val))
      else:
        l_g.append(l_a[k])
        k += 1
        
    return l_g
 

  ##
  # Takes a keyword list and a list of init- or restartfiles.
  ##
  def load_data_from_file(self, keywords = list(), *arglist):
    if len(self.keywords) > 0:
      print "Load from file warning: already keywords in the zone, may be overwriting!"

    ecl_file_list = list()
    self.keywords = keywords
    for val in arglist: 
      ecl_file_list.append(ecl_file(val))
    self.cache_list(ecl_file_list)

  ## 
  # Load data from grdecl file, currently supports only one 
  # unique kw in the grdecl file.
  ##
  def load_from_grdecl(self, filename, new_kw):
    if self.cache.has_key(new_kw):
      print "Load grdecl warning: keyword '%s' already exists, deleting old!" % new_kw
      self.delete(new_kw)
    
    kw = ecl_kw()
    kw.read_grdecl(self.grid, filename)
    self.cache[new_kw] = kw.get_data()


  ##
  # Takes a list of init- or restartfiles and  adds all the 
  # given keywords into the memory of the form:
  # hash = { keyword => list(val_1 , ... , val_n) }
  #
  # To be able to do this, it has to loop over the entire grid.
  ##
  def cache_list(self, ecl_file_list):
    for f in ecl_file_list:
      for j in xrange(f.get_num_distinct_kw()):
        str_kw = f.iget_distinct_kw(j)
        if str_kw in self.keywords:

          # Only grabbing the first occurance of the keyword
          # here, so this will fail with LGR.
          ith_kw = 0
          kw_type = f.iget_named_kw(str_kw, ith_kw)
          items = list()
          for i in xrange(0, self.grid.get_global_size()):
            ret = self.grid.get_active_index1(i)
            if ret != -1:
              data = kw_type.iget_data(ret)
              items.append(data)
          
            self.cache[str_kw] = items

    self.ecl_file_list = ecl_file_list



  ##
  # Takes an object, which inherits the Rockphysics class before
  # looping the entire grid, applying the apply function and adding
  # the returned new keywords to the cache, in the same strucutre as
  # cache_list() does.
  ##
  def apply_function(self, obj):
    for i in xrange(0, self.grid.get_global_size()):
      ret = self.grid.get_active_index1(i)
      if ret != -1:
        data = obj.apply(self, ret)

        if data != None:
          if not isinstance(data, dict):
            print "Error: the apply function has to return a 'dict' type!"
            sys.exit()
          
          for key in data.keys():
            if not self.cache.has_key(key):
              self.cache[key] = list()
           
            self.cache[key].append(data[key])
    
    # Add keywords if the finish() method returns a dict.
    tmp_cache = obj.finish()
    if isinstance(tmp_cache, dict):
      if len(tmp_cache.keys()) > 0:
        for key in tmp_cache.keys():
          self.cache[key] = tmp_cache[key]

  ##
  # Rename a keyword in the cache.
  ##
  def rename(self, old_kw, new_kw):
    if self.cache.has_key(old_kw):
      l = self.cache[old_kw]
      self.cache.pop(old_kw)
      self.cache[new_kw] = l
      return True
    
    print "Rename warning: no such keyword '%s'" % old_kw
    return False

  ##
  # Delete a keyword in the cache.
  ##
  def delete(self, kw):
    if self.cache.has_key(kw):
      self.cache.pop(kw)
      return True
    
    print "Delete warning: no such keyword '%s'" % kw
    return False


  ##
  # Loads a keyword from another zone and caches it in the current.
  ##
  def load(self, zone, kw):
    if zone.cache.has_key(kw):
      l = zone.get_values(kw)
      if self.cache.has_key(kw):
        self.delete(kw)
        print "Replacing '%s'" % kw
      self.cache[kw] = l
      return True
    
    print "Load warning: no such keyword '%s'" % kw
    return False


  ##
  # Get all the cached keys.
  ##
  def get_keywords(self):
    return self.cache.keys()


  ##
  # Get all the cached values from a key.
  ##
  def get_values(self, kw):
    if self.cache.has_key(kw):
      return self.cache[kw]


  ##
  # Get a specific active_index data value from a specific keyword.
  ## 
  def get_data(self, kw, active_index):
    return self.cache[kw][active_index]


  ##
  # Append a keyword to a grdecl file, if file does not exist
  # the file will be created.
  ##
  def append_keyword_to_grdecl(self, file, kw, new_kw = None):
    if not self.cache.has_key(kw):
      print "Append grdecl warning: no such keyword '%s'" % kw
      return
    
    if os.path.isfile(file): mode = 'a'
    else: mode = 'w'
    if new_kw is None: write_kw = kw
    else: write_kw = new_kw
    
    print "Writing '%s' to '%s'" % (write_kw, file)
    l_g = self.convert_list(self.cache[kw])
    k = ecl_kw()
    k.write_new_grdecl(file, write_kw, l_g, mode)


  ##
  # Write a keyword to a file, written in the .dat format
  ##
  def write_keyword_to_dat(self, file, kw):
    if not self.cache.has_key(kw):
      print "Append dat warning: no such keyword '%s'" % kw
      return
    
    l_a = list()
    for j, val in enumerate(self.cache[kw]):
      q = str(val)
      string = "%s\n" % q
      l_a.append(string)
      
    print "Writing '%s' to '%s'" % (kw, file)
    f = open(file, 'w')
    f.writelines(l_a)
    f.close()


  def write_all_keywords_to_roff(self, file):
    print "Writing %d keywords to '%s'" % (len(self.cache.keys()), file)

    k = list()
    for key in self.cache.keys():
      size = len(self.cache[key]);
      kw_type = ecl_kw_alloc_empty()
      ecl_kw_set_header(kw_type, key, size, "REAL")
      ecl_kw_alloc_float_data(kw_type, self.cache[key])
      k.append(kw_type)
          
    e = rms_export(self.grid_file)
    e.roff_from_keyword(file, k)


  ##
  # Write (append if the file exists) all the cached keywords
  # in the current zone.
  ##
  def write_all_keywords_to_grdecl(self, file):
    print "Writing %d keywords to '%s'" % (len(self.cache.keys()), file)
    if os.path.isfile(file): 
      print "Warning: the file '%s' already exists, appending!" % file

    for key in self.cache.keys():
      l_g = self.convert_list(self.cache[key])
      k = ecl_kw()
      k.write_new_grdecl(file, key, l_g, 'a')





####################################################################
##
## A thin SWIG wrapper starts from here to bottom!
## 
####################################################################

class rms_export:
  def __init__(self, grid_file):
    self.grid = ecl_grid(grid_file)

  def roff_from_keyword(self, filename, kw_type_list):
    rms_export_roff_from_keyword(filename, self.grid.g, kw_type_list, len(kw_type_list))
    

class ecl_summary:
  def __init__(self, ecl_data_file):
    self.ecl_data_file = ecl_data_file
    self.endian_convert = 1;
    s = ecl_sum_fread_alloc_case(ecl_data_file,self.endian_convert)
    self.s = s
  def __del__(self):
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
    return ecl_sum_has_misc_var(self.s, var)    
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
    return ecl_grid_get_global_index3(self.g, i, j, k)
  def get_active_index3(self, i, j, k):
    return ecl_grid_get_active_index3(self.g, i, j, k)
  def get_property(self, kw_obj, ijk):
    return ecl_grid_get_property(self.g, kw_obj.k, ijk[0], ijk[1], ijk[2])
  def get_active_size(self):
    return ecl_grid_get_active_size(self.g)
  def get_global_size(self):
    return ecl_grid_get_global_size(self.g)
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
  def read_grdecl(self, grid, filename):
    (nx, ny, nz, active_size) = grid.get_dims()
    fd = open(filename, "r")
    self.k = ecl_kw_fscanf_alloc_grdecl_data_wrap(fd, nx*ny*nz)
    fd.close


class fortio:
  def __init__(self, fortio_file = None):
    self.fds = []
    if fortio_file is not None:
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
