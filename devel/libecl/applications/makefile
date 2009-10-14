include global_config
SDP_ROOT	= $(shell get_sdp_root.py)
COMPILE_INCLUDE  = -I$(LIBECL_HOME)/src -I$(LIBUTIL_HOME)/include -D'VIEWER=$(VIEWER)' -I$(LIBPLOT_HOME)/include -I$(LIBCONFIG_HOME)/include -I$(EXTERNAL_INC_PATH) 

LIB_PATH  = -L$(LIBUTIL_HOME)/lib  -L$(LIBECL_HOME)/lib 
LIB_LINK  = -lecl -lutil  -lm -lz -lpthread 


BIN:	APPLICATIONS


APPLICATIONS: utm2ijk troll_aqunum

%.o : %.c
	$(CC) -c $(CFLAGS) $(COMPILE_INCLUDE) $(CPPFLAGS) $< -o $@

utm2ijk.o	: utm2ijk.c
troll_aqunum.o  : troll_aqunum.c    

#################################################################
# Binaries
# Observe that the binaries are built against the libraries / headers
# in the source directory - and not the installed files. This is to


utm2ijk:	utm2ijk.o
	$(LD) -$(MFLAG) $(LDFLAGS)  utm2ijk.o -o utm2ijk $(LIB_PATH) -L$(EXTERNAL_LIB_PATH) $(LIB_LINK) 


troll_aqunum:	troll_aqunum.o
	$(LD) -$(MFLAG) $(LDFLAGS)  troll_aqunum.o -o troll_aqunum $(LIB_PATH) -L$(EXTERNAL_LIB_PATH) $(LIB_LINK) 


