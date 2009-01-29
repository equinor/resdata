include global_config
#################################################################
COMPILE_INCLUDE   = -I./  
COMPILE_LIB_PATH  = 
INSTALL_INC_PATH = $(LIBUTIL_HOME)/include
INSTALL_LIB_PATH = $(LIBUTIL_HOME)/lib
#################################################################
SDP_ROOT          = $(shell get_sdp_root.py)
OBJECTS           = set.o hash_node.o hash_sll.o hash.o list_node.o list.o node_data.o node_ctype.o util.o thread_pool.o msg.o arg_pack.o str_buffer.o path_fmt.o stringlist.o menu.o subst.o vector.o
INC_FILES_INSTALL = set.h hash.h hash_node.h hash_sll.h list_node.h list.h node_data.h node_ctype.h util.h thread_pool.h msg.h arg_pack.h str_buffer.h path_fmt.h  stringlist.h menu.h subst.h vector.h
LIB               = libutil.a
LIB_LINK          = -lutil -lz -lpthread -lm
LINK_LIB_PATH     = -L./ 

LOCAL: LIB BIN
	install -d $(INSTALL_INC_PATH)
	install -d $(INSTALL_LIB_PATH)
	install $(INC_FILES_INSTALL) $(INSTALL_INC_PATH)
	install $(LIB) $(INSTALL_LIB_PATH)

SDP_INSTALL: LIB BIN 
	install $(LIB) $(SDP_ROOT)/lib/$(LIB)
	install $(BIN_FILES) $(SDP_ROOT)/bin
	install $(INC_FILES_INSTALL) $(SDP_ROOT)/include

clean:
	rm -f *.o *~ 
	rm -f libutil_*
	rm -f $(INSTALL_LIB_PATH)/$(LIB)
	rm -f $(INSTALL_INC_PATH)/*.h

LIB:	$(LIB)

rebuild: clean LOCAL

new: 
	../../Scripts/cdep.py all_include.h 0

$(LIB): $(OBJECTS)
	$(AR) $(ARFLAGS) $(LIB) $(OBJECTS)

%.o : %.c
	$(CC) -c $(CFLAGS) $(COMPILE_INCLUDE) $(CPPFLAGS) $< -o $@

include dependencies

# Binaries
hash_test.o : hash_test.c
util-test.o : util-test.c 
eol-fix.o   : eol-fix.c  
mem_free.o  : mem_free.c 

#################################################################

BIN:	hash.x util.x eol-fix.x mem_free.x 
BIN_FILES = hash.x util.x eol-fix.x mem_free.x 

hash.x: hash_test.o $(LIB)
	$(CC) $(LDFLAGS) -$(MFLAG) hash_test.o -o hash.x     $(LINK_LIB_PATH) $(LIB_LINK) 
 
util.x: util-test.o $(LIB)
	$(LD) -$(MFLAG) $(LDFLAGS) util-test.o -o util.x     $(LINK_LIB_PATH) $(LIB_LINK) 

eol-fix.x: eol-fix.o $(LIB)
	$(LD) -$(MFLAG) $(LDFLAGS) eol-fix.o -o eol-fix.x    $(LINK_LIB_PATH) $(LIB_LINK)

mem_free.x: mem_free.o $(LIB)
	$(LD) -$(MFLAG) $(LDFLAGS) mem_free.o -o mem_free.x  $(LINK_LIB_PATH) $(LIB_LINK)

