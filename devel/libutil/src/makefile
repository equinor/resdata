include path_config
MFLAG = m64
CC = gcc
AR = ar
#################################################################
COMPILE_INCLUDE   = -I./  -I$(LIBHASH_HOME)/include
COMPILE_LIB_PATH  = -L$(LIBHASH_HOME)/lib

INSTALL_INC_PATH = $(LIBUTIL_HOME)/include
INSTALL_LIB_PATH = $(LIBUTIL_HOME)/lib
#################################################################
CFLAGS  = -$(MFLAG) -std=gnu99 -g -Wall -fPIC
ARFLAGS = -csr
LDFLAGS = -rdynamic
#################################################################
SDP_ROOT          = /project/res/$(shell /private/joaho/bin/res_target.py)
OBJECTS           = util.o util_interC.o thread_pool.o msg.o void_arg.o str_buffer.o path_fmt.o config.o
INC_FILES_INSTALL = util.h thread_pool.h msg.h void_arg.h str_buffer.h path_fmt.h  config.h
LIB_ROOT          = libutil
LIB               = $(LIB_ROOT)_$(MFLAG).a
LIB_LINK          = -lutil_$(MFLAG) -lhash_$(MFLAG) -lz -lpthread -lm
LINK_LIB_PATH     = -L./ -L$(LIBHASH_HOME)/lib

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


$(LIB): $(OBJECTS)
	$(AR) $(ARFLAGS) $(LIB) $(OBJECTS)

%.o : %.c
	$(CC) -c $(CFLAGS) $(COMPILE_INCLUDE) $(CPPFLAGS) $< -o $@

#Library source code:
str_buffer.o  : str_buffer.c str_buffer.h
void_arg.o    : void_arg.c void_arg.h util.o
msg.o         : msg.c msg.h util.o
thread_pool.o : thread_pool.c thread_pool.h
util.o        : util.c util.h util_path.c
util_interC.o : util_interC.c util.o
path_fmt.o    : path_fmt.c path_fmt.h util.o
config.o      : config.c config.h util.o 


# Binaries
util-test.o : util-test.c 
eol-fix.o   : eol-fix.c  
mem_free.o  : mem_free.c 

#################################################################

BIN:	util.x eol-fix.x mem_free.x
BIN_FILES = util.x eol-fix.x mem_free.x
 
util.x: util-test.o
	gcc $(LDFLAGS) util-test.o -o util.x     $(LINK_LIB_PATH) $(LIB_LINK) 

eol-fix.x: eol-fix.o
	gcc $(LDFLAGS) eol-fix.o -o eol-fix.x    $(LINK_LIB_PATH) $(LIB_LINK)

mem_free.x: mem_free.o
	gcc $(LDFLAGS) mem_free.o -o mem_free.x  $(LINK_LIB_PATH) $(LIB_LINK)

