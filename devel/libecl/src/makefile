include global_config
SDP_ROOT	= $(shell get_sdp_root.py)
#################################################################
VIEWER        = "/usr/bin/display"
COMPILE_INCLUDE  = -I$(LIBECL_HOME)/src -I$(LIBUTIL_HOME)/include -D'VIEWER=$(VIEWER)' -I$(LIBPLOT_HOME)/include -I$(EXTERNAL_INC_PATH)
INSTALL_INC_PATH =  $(LIBECL_HOME)/include
INSTALL_LIB_PATH =  $(LIBECL_HOME)/lib
#################################################################
LIB_OBJECTS   = ecl_smspec.o ecl_sum_data.o ecl_well_vars.o ecl_util.o ecl_kw.o ecl_sum.o fortio.o ecl_rft_file.o ecl_rft_node.o ecl_grid.o ecl_box.o ecl_io_config.o ecl_file.o
MOD_FILES = 
INC_FILES = ecl_smspec.h ecl_sum_data.h ecl_well_vars.h ecl_util.h ecl_kw.h ecl_sum.h fortio.h ecl_rft_file.h ecl_rft_node.h ecl_box.h ecl_grid.h ecl_io_config.h ecl_file.h
LIB       = libecl.a
LIB_PATH  = -L$(LIBUTIL_HOME)/lib -L./ -L$(LIBPLOT_HOME)/lib -L$(EXTERNAL_LIB_PATH)
LIB_LINK  = -lecl -lutil -lplot -lplplotd -lm -lz -lpthread 


LOCAL: LIB BIN
	install -d $(INSTALL_INC_PATH)
	install -d $(INSTALL_LIB_PATH)
	install $(INC_FILES) $(INSTALL_INC_PATH)
	install $(LIB) $(INSTALL_LIB_PATH)


SDP_INSTALL: LIB BIN
	install $(LIB) $(SDP_ROOT)/lib
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
block_owc.o    : block_owc.c 
extract_contact.o : extract_contact.c
extract_IJ.o   : extract_IJ.c
ecl_pack.o     : ecl_pack.c
ecl_unpack.o   : ecl_unpack.c
run_gravity.o  : run_gravity.c
ens_plot.o     : ens_plot.c
#################################################################
# Binaries
# Observe that the binaries are built against the libraries / headers
# in the source directory - and not the installed files. This is to


BIN_FILES  = ecl_test.x convert.x kw_list.x kw_extract.x grid_info.x summary.x rft.x restart.x block_owc.x extract_owc.x ecl_pack.x ecl_unpack.x extract_IJ.x run_gravity.x rft_test.x ens_plot.x
BIN	:  $(BIN_FILES) 
BIN_FILES_SDP_INSTALL = ecl_test.x convert.x kw_list.x kw_extract.x grid_info.x summary.x rft.x extract_owc.x block_owc.x ecl_pack.x ecl_unpack.x extract_IJ.x run_gravity.x

ecl_pack.x: $(LIB) ecl_pack.o
	$(LD) -$(MFLAG) $(LDFLAGS) ecl_pack.o -o ecl_pack.x $(LIB_PATH) $(LIB_LINK)

rft_test.x: $(LIB) rft_test.o
	$(LD) -$(MFLAG) $(LDFLAGS) rft_test.o -o rft_test.x $(LIB_PATH) $(LIB_LINK)

ecl_unpack.x: $(LIB) ecl_unpack.o
	$(LD) -$(MFLAG) $(LDFLAGS) ecl_unpack.o -o ecl_unpack.x $(LIB_PATH) $(LIB_LINK)

extract_IJ.x: $(LIB) extract_IJ.o
	$(LD) -$(MFLAG) $(LDFLAGS) extract_IJ.o -o extract_IJ.x $(LIB_PATH) $(LIB_LINK)

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

block_owc.x: $(LIB) block_owc.o
	$(LD) -$(MFLAG) $(LDFLAGS)  block_owc.o -o block_owc.x $(LIB_PATH)  $(LIB_LINK)

extract_owc.x: $(LIB) extract_contact.o
	$(LD) -$(MFLAG) $(LDFLAGS)  extract_contact.o -o extract_owc.x $(LIB_PATH)  $(LIB_LINK)

summary.x: $(LIB) view_summary.o
	$(LD) -$(MFLAG) $(LDFLAGS)  view_summary.o -o summary.x $(LIB_PATH)  $(LIB_LINK)

restart.x: $(LIB) view_restart.o
	$(LD) -$(MFLAG) $(LDFLAGS)  view_restart.o -o restart.x $(LIB_PATH)  $(LIB_LINK)

rft.x: $(LIB) view_rft.o
	$(LD) -$(MFLAG) $(LDFLAGS)  view_rft.o -o rft.x $(LIB_PATH)  $(LIB_LINK)

run_gravity.x: $(LIB) run_gravity.o
	$(LD) -$(MFLAG) $(LDFLAGS)  run_gravity.o -o run_gravity.x $(LIB_PATH)  $(LIB_LINK)

ens_plot.x: ens_plot.o
	$(LD) -$(MFLAG) $(LDFLAGS)  ens_plot.o -o ens_plot.x $(LIB_PATH)  $(LIB_LINK)


