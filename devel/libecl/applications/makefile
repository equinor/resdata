include global_config
SDP_ROOT	= $(shell get_sdp_root.py)
COMPILE_INCLUDE  = -I$(LIBECL_HOME)/src -I$(LIBUTIL_HOME)/include -D'VIEWER=$(VIEWER)' -I$(LIBPLOT_HOME)/include -I$(LIBCONFIG_HOME)/include -I$(EXTERNAL_INC_PATH) 

LIB_PATH  = -L$(LIBUTIL_HOME)/lib  -L$(LIBECL_HOME)/lib 
LIB_LINK  = -lecl -lconfig -lutil  -lm -lz -lpthread 



%.o : %.c
	$(CC) -c $(CFLAGS) $(COMPILE_INCLUDE) $(CPPFLAGS) $< -o $@

utm2ijk.o	: utm2ijk.c

#################################################################
# Binaries
# Observe that the binaries are built against the libraries / headers
# in the source directory - and not the installed files. This is to


utm2ijk:	utm2ijk.o
	$(LD) -$(MFLAG) $(LDFLAGS)  ens_plot.o -o ens_plot.x $(LIB_PATH) -L$(LIBPLOT_HOME)/lib -L$(EXTERNAL_LIB_PATH)  -lplot $(LIB_LINK)  -lplplotd
