include global_config
SDP_ROOT	= $(shell get_sdp_root.py)
#################################################################
COMPILE_INCLUDE  = -I$(LIBECL_HOME)/src -I$(LIBUTIL_HOME)/include 
INSTALL_INC_PATH =  $(LIBECL_HOME)/include
INSTALL_LIB_PATH =  $(LIBECL_HOME)/lib
#################################################################
LIB_OBJECTS   = ecl_well_vars.o ecl_util.o ecl_kw.o ecl_block.o ecl_fstate.o ecl_sum.o fortio.o ecl_rft_vector.o ecl_rft_node.o ecl_grid.o ecl_box.o restart_kw_list.o ecl_io_config.o 
MOD_FILES = 
INC_FILES = ecl_well_vars.h ecl_util.h ecl_kw.h ecl_block.h ecl_fstate.h ecl_sum.h fortio.h ecl_rft_vector.h ecl_rft_node.h ecl_box.h ecl_grid.h restart_kw_list.h  ecl_io_config.h
LIB       = libecl.a
LIB_PATH  = -L$(LIBUTIL_HOME)/lib -L./
LIB_LINK  = -lecl -lutil -lm -lz -lpt


LOCAL: LIB BIN
	install -d $(INSTALL_INC_PATH)
	install -d $(INSTALL_LIB_PATH)
	install $(INC_FILES) $(INSTALL_INC_PATH)
	install $(LIB) $(INSTALL_LIB_PATH)


SDP_INSTALL: LIB BIN
	install $(LIB) $(SDP_ROOT)/lib/$(LIB_ROOT).a
	install $(BIN_FILES_SDP_INSTALL) $(SDP_ROOT)/bin
	install $(INC_FILES) $(SDP_ROOT)/include

clean:
	rm -f *.o *~ 
	rm -f libecl_*
	rm -f $(INSTALL_INC_PATH)/*.h
	rm -f $(INSTALL_LIB_PATH)/$(LIB)

rebuild: clean LOCAL

LIB:	$(LIB)

$(LIB): $(LIB_OBJECTS)
	$(AR) $(ARFLAGS) $(LIB) $(LIB_OBJECTS)

%.o : %.c
	$(CC) -c $(CFLAGS) $(COMPILE_INCLUDE) $(CPPFLAGS) $< -o $@

new: 
	../../Scripts/cdep.py all_include.h 0

include dependencies

# Source code for binaries - based on library
view_rft.o     : view_rft.c 	
ecl_test.o     : ecl_test.c 	
convert.o      : convert.c  	
kw_list.o      : kw_list.c  	
kw_extract.o   : kw_extract.c   
grid_info.o    : grid_info.c    
view_summary.o : view_summary.c 
view_restart.o : view_restart.c 


#################################################################
# Binaries
# Observe that the binaries are built against the libraries / headers
# in the source directory - and not the installed files. This is to


BIN_FILES  = ecl_test.x convert.x kw_list.x kw_extract.x grid_info.x summary.x rft.x restart.x
BIN	:  $(BIN_FILES) 
BIN_FILES_SDP_INSTALL = ecl_test.x convert.x kw_list.x kw_extract.x grid_info.x summary.x rft.x

ecl_test.x: $(LIB) ecl_test.o
	$(LD) -$(MFLAG) $(LDFLAGS) ecl_test.o -o ecl_test.x $(LIB_PATH) $(LIB_LINK)

convert.x: $(LIB) convert.o
	$(LD) -$(MFLAG) $(LDFLAGS)  convert.o -o convert.x $(LIB_PATH)  $(LIB_LINK)

kw_list.x: $(LIB) kw_list.o
	$(LD) -$(MFLAG) $(LDFLAGS)  kw_list.o -o kw_list.x $(LIB_PATH)  $(LIB_LINK)

kw_extract.x: $(LIB) kw_extract.o
	$(LD) -$(MFLAG) $(LDFLAGS)  kw_extract.o -o kw_extract.x $(LIB_PATH)  $(LIB_LINK)

grid_info.x: $(LIB) grid_info.o
	$(LD) -$(MFLAG) $(LDFLAGS)  grid_info.o -o grid_info.x $(LIB_PATH)  $(LIB_LINK)

summary.x: $(LIB) view_summary.o
	$(LD) -$(MFLAG) $(LDFLAGS)  view_summary.o -o summary.x $(LIB_PATH)  $(LIB_LINK)

restart.x: $(LIB) view_restart.o
	$(LD) -$(MFLAG) $(LDFLAGS)  view_restart.o -o restart.x $(LIB_PATH)  $(LIB_LINK)

rft.x: $(LIB) view_rft.o
	$(LD) -$(MFLAG) $(LDFLAGS)  view_rft.o -o rft.x $(LIB_PATH)  $(LIB_LINK)


