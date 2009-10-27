include global_config
SDP_ROOT	= $(shell get_sdp_root.py)
COMPILE_INCLUDE  = -I$(LIBECL_HOME)/src -I$(LIBUTIL_HOME)/include -D'VIEWER=$(VIEWER)' -I$(LIBPLOT_HOME)/include -I$(LIBCONFIG_HOME)/include -I$(EXTERNAL_INC_PATH) 

LIB_PATH  = -L$(LIBUTIL_HOME)/lib  -L$(LIBECL_HOME)/lib 
LIB_LINK  = -lecl -lutil  -lm -lz -lpthread -llapack


BIN:	APPLICATIONS


APPLICATION_LIST  = utm2ij utm2ijk troll_aqunum vprofile grid_layer
APPLICATIONS: $(APPLICATION_LIST)

clean:
	rm -f *.o
	rm -f $(APPLICATION_LIST)

%.o : %.c
	$(CC) -c $(CFLAGS) $(COMPILE_INCLUDE) $(CPPFLAGS) $< -o $@

utm2ijk.o	: utm2ijk.c
utm2ij.o	: utm2ij.c
troll_aqunum.o  : troll_aqunum.c    
vprofile.o      : vprofile.c
grid_layer.o    : grid_layer.c

#################################################################
# Binaries
# Observe that the binaries are built against the libraries / headers
# in the source directory - and not the installed files. This is to


utm2ijk:	utm2ijk.o
	$(LD) -$(MFLAG) $(LDFLAGS)  utm2ijk.o -o utm2ijk $(LIB_PATH) -L$(EXTERNAL_LIB_PATH) $(LIB_LINK) 


utm2ij:	utm2ij.o
	$(LD) -$(MFLAG) $(LDFLAGS)  utm2ij.o -o utm2ij $(LIB_PATH) -L$(EXTERNAL_LIB_PATH) $(LIB_LINK) 


troll_aqunum:	troll_aqunum.o
	$(LD) -$(MFLAG) $(LDFLAGS)  troll_aqunum.o -o troll_aqunum $(LIB_PATH) -L$(EXTERNAL_LIB_PATH) $(LIB_LINK) 


vprofile:	vprofile.o
	$(LD) -$(MFLAG) $(LDFLAGS)  vprofile.o -o vprofile $(LIB_PATH) -L$(EXTERNAL_LIB_PATH) $(LIB_LINK) 

grid_layer:	grid_layer.o
	$(LD) -$(MFLAG) $(LDFLAGS)  grid_layer.o -o grid_layer $(LIB_PATH) -L$(EXTERNAL_LIB_PATH) $(LIB_LINK) 


