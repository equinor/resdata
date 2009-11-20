include global_config
SVN_VERSION 	   = $(shell svnversion ./)
COMPILE_TIME_STAMP = $(shell date)

#################################################################
COMPILE_INCLUDE   = -I$(LIBUTIL_HOME)/include
SDP_ROOT          = $(shell get_sdp_root.py)
LIB_LINK          = -lutil -lz -lpthread -llapack -lblas -lm
LINK_LIB_PATH     = -L$(LIBUTIL_HOME)/lib

LOCAL: BIN
	install -d $(INSTALL_BIN_PATH)
	install $(BIN_FILES) $(INSTALL_BIN_PATH)

BIN: $(BIN_FILES)


clean:
	rm -f *.o 
	rm -f *~


# This is a list of ALL the binaries.
BIN_FILES     = util.x eol-fix.x mem_free.x parser_test.x grdecl_to_ascii.x replace.x bls.x bfs_extract file_open.x ens_tar.x grane_fix replace.x


# These are the binaries which should be installed in SDP
SDP_BIN_FILES = bls.x bfs_extract ens_tar.x 




%.o : %.c
	$(CC) -c $(CFLAGS) $(COMPILE_INCLUDE) $(CPPFLAGS) $< -o $@


BIN:	util.x eol-fix.x mem_free.x parser_test.x grdecl_to_ascii.x replace.x bls.x bfs_extract file_open.x ens_tar.x grane_fix


util.x: util-test.o $(LIB)
	$(LD) -$(MFLAG) $(LDFLAGS) util-test.o -o util.x     $(LINK_LIB_PATH) $(LIB_LINK) 

eol-fix.x: eol-fix.o $(LIB)
	$(LD) -$(MFLAG) $(LDFLAGS) eol-fix.o -o eol-fix.x    $(LINK_LIB_PATH) $(LIB_LINK)

mem_free.x: mem_free.o $(LIB)
	$(LD) -$(MFLAG) $(LDFLAGS) mem_free.o -o mem_free.x  $(LINK_LIB_PATH) $(LIB_LINK)

parser_test.x: parser_test.o $(LIB)
	$(LD) -$(MFLAG) $(LDFLAGS) parser_test.o -o parser_test.x  $(LINK_LIB_PATH) $(LIB_LINK)

grdecl_to_ascii.x: grdecl_to_ascii.o $(LIB)
	$(LD) -$(MFLAG) $(LDFLAGS) grdecl_to_ascii.o -o grdecl_to_ascii.x  $(LINK_LIB_PATH) $(LIB_LINK)

replace.x: replace.o $(LIB)
	$(LD) -$(MFLAG) $(LDFLAGS) replace.o -o replace.x  -L./ -lutil -lz -lpthread -lm

bls.x: bls.o $(LIB)
	$(LD) -$(MFLAG) $(LDFLAGS) bls.o -o bls.x  -L./ -lutil -lz -lpthread -lm

bfs_extract: bfs_extract.o $(LIB)
	$(LD) -$(MFLAG) $(LDFLAGS) bfs_extract.o -o bfs_extract  -L./ -lutil -lz -lpthread -lm

grane_fix: grane_fix.o $(LIB)
	$(LD) -$(MFLAG) $(LDFLAGS) grane_fix.o -o grane_fix  -L./ -lutil -lz -lpthread -lm

file_open.x: file_open.o $(LIB)
	$(LD) -$(MFLAG) $(LDFLAGS) file_open.o -o file_open.x  -L./ -lutil -lz -lpthread -lm

ens_tar.x: ens_tar.o $(LIB)
	$(LD) -$(MFLAG) $(LDFLAGS) ens_tar.o -o ens_tar.x  -L./ -lutil -lz -lpthread -lm
