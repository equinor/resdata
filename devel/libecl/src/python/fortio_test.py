#!/usr/bin/env python

from ecl import *
from time import *
import pprint

file = '/private/masar/enkf/devel/Testcases/SimpleEnKF/Simulations/Real0/EXAMPLE_01_BASE_0000.X0039'

print '############# FORTIO APPROACH 1 #############'
fort = fortio(file)
d = fort.read_data("PRESSURE")
print d[100]
fort.close()

print '############# FORTIO, APPROACH 2 #############'
fortio = fortio()
fort = fortio.open(file)

print fort
kw_obj = ecl_kw()
print kw_obj
kw_type = kw_obj.fread_alloc(fort)
print kw_type
kw_obj.fseek_kw("PRESSURE", fort)
kw_obj.fread_realloc(kw_type, fort)
d = kw_obj.get_data()
print d[100]

print '############# FORTIO, APPROACH 3 #############'

fmt_src = ecl_util_fmt_file(file)
fortio = fortio_fopen(file, "r", 1, fmt_src)
kw_type = ecl_kw_fread_alloc(fortio)
if ecl_kw_fseek_kw("PRESSURE", 1, 0, fortio):
	ecl_kw_fread_realloc(kw_type, fortio)
w = ecl_kw_get_data_wrap_void(kw_type)
list = get_ecl_kw_data_wrapper_void_list(w)
ecl_kw_free(kw_type)
print list[100]
fortio_fclose(fortio)

print '############# FINISHED #############'
