include global_config
#################################################################
COMPILE_INCLUDE   = -I./  
COMPILE_LIB_PATH  = 
INSTALL_INC_PATH = $(LIBUTIL_HOME)/include
INSTALL_LIB_PATH = $(LIBUTIL_HOME)/lib
#################################################################
SDP_ROOT          = /project/res/$(shell /private/joaho/bin/res_target.py)
OBJECTS           = set.o hash_node.o hash_sll.o hash.o list_node.o list.o node_data.o node_ctype.o util.o thread_pool.o msg.o arg_pack.o str_buffer.o path_fmt.o stringlist.o menu.o subst.o
INC_FILES_INSTALL = set.h hash.h hash_node.h hash_sll.h list_node.h list.h node_data.h node_ctype.h util.h thread_pool.h msg.h arg_pack.h str_buffer.h path_fmt.h  stringlist.h menu.h subst.h
LIB_ROOT          = libutil
LIB               = $(LIB_ROOT)_$(MFLAG).a
LIB_LINK          = -lutil_$(MFLAG) -lz -lpthread -lm
LINK_LIB_PATH     = -L./ 

LOCAL: LIB BIN
	install -d $(INSTALL_INC_PATH)
	install -d $(INSTALL_LIB_PATH)
	install $(INC_FILES_INSTALL) $(INSTALL_INC_PATH)
	install $(LIB) $(INSTALL_LIB_PATH)

SDP_INSTALL: LIB BIN
	install $(LIB) $(SDP_ROOT)/lib/$(LIB_ROOT).a
	install $(BIN_FILES) $(SDP_ROOT)/bin
	install $(INC_FILES_INSTALL) $(SDP_ROOT)/include

clean:
	rm -f *.o *~ 
	rm -f libutil_*
	rm -f $(INSTALL_LIB_PATH)/$(LIB)
	rm -f $(INSTALL_INC_PATH)/*.h

LIB:	$(LIB)

rebuild: clean LOCAL

$(LIB): $(OBJECTS)
	$(AR) $(ARFLAGS) $(LIB) $(OBJECTS)

%.o : %.c
	$(CC) -c $(CFLAGS) $(COMPILE_INCLUDE) $(CPPFLAGS) $< -o $@

#Library source code:
node_ctype.o  : node_ctype.h node_ctype.c
set.o         : set.c set.h hash.o
node_data.o   : node_data.c node_data.h
list_node.o   : list_node.c list_node.h
list.o        : list.c list.h list_node.o node_data.o
hash_node.o   : hash_node.c hash_node.h
hash_sll.o    : hash_sll.c hash_sll.h hash_node.o
hash.o        : hash.c hash.h hash_sll.o node_data.o
stringlist.o  : stringlist.h stringlist.c util.o
str_buffer.o  : str_buffer.c str_buffer.h
void_arg.o    : void_arg.c void_arg.h util.o
msg.o         : msg.c msg.h util.o
thread_pool.o : thread_pool.c thread_pool.h
util.o        : util.c util.h util_path.c
path_fmt.o    : path_fmt.c path_fmt.h util.o
menu.o        : menu.h menu.c util.o
subst.o       : subst.h subst.c util.o

# Binaries
hash_test.o : hash_test.c
util-test.o : util-test.c 
eol-fix.o   : eol-fix.c  
mem_free.o  : mem_free.c 

#################################################################

BIN:	hash.x util.x eol-fix.x mem_free.x 
BIN_FILES = hash.x util.x eol-fix.x mem_free.x 

hash.x: hash_test.o $(LIB)
	gcc $(LDFLAGS) -$(MFLAG) hash_test.o -o hash.x     $(LINK_LIB_PATH) $(LIB_LINK) 
 
util.x: util-test.o $(LIB)
	$(LD) -$(MFLAG) $(LDFLAGS) util-test.o -o util.x     $(LINK_LIB_PATH) $(LIB_LINK) 

eol-fix.x: eol-fix.o $(LIB)
	$(LD) -$(MFLAG) $(LDFLAGS) eol-fix.o -o eol-fix.x    $(LINK_LIB_PATH) $(LIB_LINK)

mem_free.x: mem_free.o $(LIB)
	$(LD) -$(MFLAG) $(LDFLAGS) mem_free.o -o mem_free.x  $(LINK_LIB_PATH) $(LIB_LINK)

