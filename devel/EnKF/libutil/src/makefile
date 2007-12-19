MFLAG = m32
CC = gcc
FC = ifort
AR = ar
#################################################################
COMPILE_INCLUDE   = -I./ 
COMPILE_LIB_PATH  = -L../../libhash/lib
COMPILE_LIBS      = -lm -lhash

INSTALL_INC_PATH = ../include
INSTALL_LIB_PATH = ../lib
#################################################################
CFLAGS  = -$(MFLAG) -std=gnu99 -g -Wall $(COMPILE_INCLUDE) -fPIC
ARFLAGS = -csr
#################################################################
OBJECTS           = pathv.o util.o util_interC.o thread_pool.o msg.o void_arg.o str_buffer.o path_fmt.o
INC_FILES_INSTALL = pathv.h util.h thread_pool.h msg.h void_arg.h str_buffer.h path_fmt.h
LIB               = libutil_$(MFLAG).a
LIB_LINK          = -lutil_$(MFLAG) -lz -lm


INSTALL: $(LIB)
	install -d $(INSTALL_INC_PATH)
	install -d $(INSTALL_LIB_PATH)
	install $(INC_FILES_INSTALL) $(INSTALL_INC_PATH)
	install $(LIB) $(INSTALL_LIB_PATH)
	gcc $(CFLAGS) util-test.c -o util.x -I../include    -L../lib $(LIB_LINK)
	gcc $(CFLAGS) eol-fix.c -o eol-fix.x -I../include   -L../lib $(LIB_LINK)
	gcc $(CFLAGS) mem_free.c -o mem_free.x -I../include -L../lib $(LIB_LINK)

clean:
	rm -f *.o *~ 
	rm -f libutil_*
	rm -f $(INSTALL_LIB_PATH)/$(LIB)
	rm -f $(INSTALL_INC_PATH)/*.h


$(LIB): $(OBJECTS)
	$(AR) $(ARFLAGS) $(LIB) $(OBJECTS)

%.o : %.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

str_buffer.o  : str_buffer.c str_buffer.h
void_arg.o    : void_arg.c void_arg.h util.o
msg.o         : msg.c msg.h util.o
thread_pool.o : thread_pool.c thread_pool.h
util.o        : util.c util.h
util_interC.o : util_interC.c util.o
pathv.o       : pathv.c pathv.h util.o
path_fmt.o    : path_fmt.c path_fmt.h util.o
