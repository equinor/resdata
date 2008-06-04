include path_config
MFLAG = m32
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
OBJECTS           = pathv.o util.o util_interC.o thread_pool.o msg.o void_arg.o str_buffer.o path_fmt.o config.o
INC_FILES_INSTALL = pathv.h util.h thread_pool.h msg.h void_arg.h str_buffer.h path_fmt.h  config.h
LIB               = libutil_$(MFLAG).a
LIB_LINK          = -lutil_$(MFLAG) -lhash_$(MFLAG) -lz -lpthread -lm
LINK_LIB_PATH     = -L$(LIBUTIL_HOME)/lib -L$(LIBHASH_HOME)/lib
FINAL_INC_PATH    = -I$(LIBUTIL_HOME)/include -I$(LIBHASH_HOME)/include

all: $(LIB)
	install -d $(INSTALL_INC_PATH)
	install -d $(INSTALL_LIB_PATH)
	install $(INC_FILES_INSTALL) $(INSTALL_INC_PATH)
	install $(LIB) $(INSTALL_LIB_PATH)
	gcc $(CFLAGS) $(LDFLAGS) util-test.c -o util.x     $(FINAL_INC_PATH) $(LINK_LIB_PATH) $(LIB_LINK) 
	gcc $(CFLAGS) $(LDFLAGS) eol-fix.c -o eol-fix.x    $(FINAL_INC_PATH) $(LINK_LIB_PATH) $(LIB_LINK)
	gcc $(CFLAGS) $(LDFLAGS) mem_free.c -o mem_free.x  $(FINAL_INC_PATH) $(LINK_LIB_PATH) $(LIB_LINK)

clean:
	rm -f *.o *~ 
	rm -f libutil_*
	rm -f $(INSTALL_LIB_PATH)/$(LIB)
	rm -f $(INSTALL_INC_PATH)/*.h


$(LIB): $(OBJECTS)
	$(AR) $(ARFLAGS) $(LIB) $(OBJECTS)

%.o : %.c
	$(CC) -c $(CFLAGS) $(COMPILE_INCLUDE) $(CPPFLAGS) $< -o $@

str_buffer.o  : str_buffer.c str_buffer.h
void_arg.o    : void_arg.c void_arg.h util.o
msg.o         : msg.c msg.h util.o
thread_pool.o : thread_pool.c thread_pool.h
util.o        : util.c util.h util_path.c
util_interC.o : util_interC.c util.o
pathv.o       : pathv.c pathv.h util.o
path_fmt.o    : path_fmt.c path_fmt.h util.o
config.o      : config.c config.h util.o 
 
