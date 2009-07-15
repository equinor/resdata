from libecl import *
import pprint as pp
import sys
import time


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



