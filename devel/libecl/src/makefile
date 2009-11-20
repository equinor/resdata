include global_config
SDP_ROOT	= $(shell get_sdp_root.py)
#################################################################
COMPILE_INCLUDE  = -I$(LIBECL_HOME)/src -I$(LIBUTIL_HOME)/include 
INSTALL_INC_PATH =  $(LIBECL_HOME)/include
INSTALL_LIB_PATH =  $(LIBECL_HOME)/lib
#################################################################
LIB_OBJECTS       = ecl_smspec.o ecl_sum_data.o ecl_well_vars.o ecl_util.o ecl_kw.o ecl_sum.o fortio.o ecl_rft_file.o ecl_rft_node.o ecl_grid.o ecl_box.o ecl_io_config.o ecl_file.o ecl_region.o point.o tetrahedron.o 
INC_FILES         = ecl_endian_flip.h ecl_smspec.h ecl_sum_data.h ecl_well_vars.h ecl_util.h ecl_kw.h ecl_sum.h fortio.h ecl_rft_file.h ecl_rft_node.h ecl_box.h ecl_grid.h ecl_io_config.h ecl_file.h ecl_region.h point.h tetrahedron.h 
LIB       = libecl.a


LOCAL: LIB 
	install -d $(INSTALL_INC_PATH)
	install -d $(INSTALL_LIB_PATH)
	install $(INC_FILES) $(INSTALL_INC_PATH)
	install $(LIB) $(INSTALL_LIB_PATH)


SDP_INSTALL: LIB 
	install $(LIB) $(SDP_ROOT)/lib
	install $(INC_FILES) $(SDP_ROOT)/include

clean:
	rm -f *.o *~ 
	rm -f libecl_*
	rm -f $(INSTALL_INC_PATH)/*.h
	rm -f $(INSTALL_LIB_PATH)/$(LIB)

rebuild: clean new LOCAL

LIB:	$(LIB)

$(LIB): $(LIB_OBJECTS)
	$(AR) $(ARFLAGS) $(LIB) $(LIB_OBJECTS)


%.o : %.c
	$(CC) -c $(CFLAGS) $(COMPILE_INCLUDE) $(CPPFLAGS) $< -o $@

new: 
	../../Scripts/cdep.py all_include.h 0

include dependencies


