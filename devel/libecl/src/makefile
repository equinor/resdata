include path_config
MFLAG = m64
CC = gcc
AR = ar
#################################################################
COMPILE_INCLUDE  = -I$(LIBECL_HOME)/src -I$(LIBUTIL_HOME)/include -I$(LIBHASH_HOME)/include 
INSTALL_INC_PATH =  $(LIBECL_HOME)/include
INSTALL_LIB_PATH =  $(LIBECL_HOME)/lib
#################################################################
CFLAGS  = -O2 -$(MFLAG)  -std=gnu99 -g -Wall -fPIC
LDFLAGS = 
ARFLAGS = -csr
#################################################################
__SDP_ROOT  	   = $(SDP_ROOT)/$(shell res_target.py)
LIB_OBJECTS   = ecl_well_vars.o ecl_util.o ecl_kw.o ecl_block.o ecl_fstate.o ecl_sum.o ecl_interC.o fortio.o lsf_jobs.o ecl_parse.o ecl_rft_vector.o ecl_rft_node.o ecl_grid.o ecl_box.o restart_kw_list.o 
MOD_FILES = 
INC_FILES = ecl_well_vars.h ecl_util.h ecl_kw.h ecl_block.h ecl_fstate.h ecl_sum.h fortio.h lsf_jobs.h ecl_parse.h ecl_rft_vector.h ecl_rft_node.h ecl_box.h ecl_grid.h restart_kw_list.h 
LIB_ROOT  = libecl
LIB       = $(LIB_ROOT)_$(MFLAG).a
LIB_PATH  = -L$(LIBUTIL_HOME)/lib -L$(LIBHASH_HOME)/lib -L./
LIB_LINK  = -lecl_$(MFLAG) -lutil_$(MFLAG) -lhash_$(MFLAG) -lm -lz


LOCAL: LIB BIN
	install -d $(INSTALL_INC_PATH)
	install -d $(INSTALL_LIB_PATH)
	install $(INC_FILES) $(INSTALL_INC_PATH)
	install $(LIB) $(INSTALL_LIB_PATH)


SDP_INSTALL: LIB BIN
	install $(LIB) $(__SDP_ROOT)/lib/$(LIB_ROOT).a
	install $(BIN_FILES_SDP_INSTALL) $(__SDP_ROOT)/bin
	install $(INC_FILES) $(__SDP_ROOT)/include

clean:
	rm -f *.o *~ 
	rm -f libecl_*
	rm -f $(INSTALL_INC_PATH)/*.h
	rm -f $(INSTALL_LIB_PATH)/libecl_$(MFLAG).a


LIB:	$(LIB)

$(LIB): $(LIB_OBJECTS)
	$(AR) $(ARFLAGS) $(LIB) $(LIB_OBJECTS)

%.o : %.c
	$(CC) -c $(CFLAGS) $(COMPILE_INCLUDE) $(CPPFLAGS) $< -o $@


# Library source code
ecl_rft_vector.o   : ecl_rft_vector.h ecl_rft_vector.c ecl_util.h ecl_kw.o ecl_fstate.o ecl_block.o ecl_rft_node.o
ecl_util.o   : ecl_util.c ecl_util.h
lsf_jobs.o   : lsf_jobs.c lsf_jobs.h 
fortio.o     : fortio.c fortio.h 
ecl_kw.o     : ecl_kw.c ecl_kw.h fortio.o
ecl_block.o  : ecl_block.c ecl_block.h ecl_kw.o
ecl_fstate.o : ecl_fstate.c ecl_fstate.h ecl_block.o ecl_util.o ecl_kw.o
ecl_sum.o    : ecl_sum.c ecl_sum.h ecl_fstate.o ecl_block.o ecl_kw.o ecl_util.o
ecl_parse.o  : ecl_parse.h ecl_parse.h ecl_fstate.o ecl_util.o ecl_kw.o ecl_block.o
ecl_interC.o : ecl_interC.c ecl_fstate.o ecl_sum.o ecl_parse.o lsf_jobs.o
ecl_grid.o   : ecl_grid.c ecl_grid.h ecl_kw.o fortio.o
ecl_interC.o : ecl_sum.o ecl_kw.o ecl_fstate.o ecl_block.o 
ecl_box.o    : ecl_box.c ecl_box.h


# Source code for binaries - based on library
ecl_test.o     : ecl_test.c 	
convert.o      : convert.c  	
kw_list.o      : kw_list.c  	
kw_extract.o   : kw_extract.c   
grid_info.o    : grid_info.c    
view_summary.o : view_summary.c 


#################################################################
# Binaries
# Observe that the binaries are built against the libraries / headers
# in the source directory - and not the installed files. This is to


BIN_FILES  = ecl_test.x convert.x kw_list.x kw_extract.x grid_info.x summary.x
BIN	:  $(BIN_FILES) 
BIN_FILES_SDP_INSTALL = ecl_test.x convert.x kw_list.x kw_extract.x grid_info.x summary.x

ecl_test.x: $(LIB) ecl_test.o
	gcc -$(MFLAG) $(LDFLAGS) ecl_test.o -o ecl_test.x $(LIB_PATH) $(LIB_LINK)

convert.x: $(LIB) convert.o
	gcc -$(MFLAG) $(LDFLAGS)  convert.o -o convert.x $(LIB_PATH)  $(LIB_LINK)

kw_list.x: $(LIB) kw_list.o
	gcc -$(MFLAG) $(LDFLAGS)  kw_list.o -o kw_list.x $(LIB_PATH)  $(LIB_LINK)

kw_extract.x: $(LIB) kw_extract.o
	gcc -$(MFLAG) $(LDFLAGS)  kw_extract.o -o kw_extract.x $(LIB_PATH)  $(LIB_LINK)

grid_info.x: $(LIB) grid_info.o
	gcc -$(MFLAG) $(LDFLAGS)  grid_info.o -o grid_info.x $(LIB_PATH)  $(LIB_LINK)

summary.x: $(LIB) view_summary.o
	gcc -$(MFLAG) $(LDFLAGS)  view_summary.o -o summary.x $(LIB_PATH)  $(LIB_LINK)


