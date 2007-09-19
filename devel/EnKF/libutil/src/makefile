CC = gcc
FC = ifort
AR = ar
#################################################################
COMPILE_INCLUDE   = -I./ 
COMPILE_LIB_PATH  = -L../../libhash/lib
COMPILE_LIBS      = -lm -lhash

INSTALL_INC_PATH = ../include
INSTALL_LIB_PATH = ../lib
INSTALL_MOD_PATH = ../mod
#################################################################
CFLAGS  = -m32 -std=gnu99 -g -Wall $(COMPILE_INCLUDE) 
ARFLAGS = -csr
#################################################################
OBJECTS           = pathv.o util.o util_interC.o thread_pool.o msg.o void_arg.o
INC_FILES_INSTALL = pathv.h util.h thread_pool.h msg.h void_arg.h
LIB               = libutil.a



INSTALL: $(LIB)
	install -d $(INSTALL_INC_PATH)
	install -d $(INSTALL_LIB_PATH)
	install -d $(INSTALL_MOD_PATH)
	install $(INC_FILES_INSTALL) $(INSTALL_INC_PATH)
	install $(LIB) $(INSTALL_LIB_PATH)
	gcc -g -Wall -m32 util-test.c -o util.x -I../include -L../lib -lutil
	gcc -g -Wall -m32 eol-fix.c -o eol-fix.x -I../include -L../lib -lutil

new:	
	make clean
	make INSTALL

clean:
	rm -f *.o *~ 

$(LIB): $(OBJECTS)
	$(AR) $(ARFLAGS) $(LIB) $(OBJECTS)

%.o : %.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@


void_arg.o    : void_arg.c void_arg.h
msg.o         : msg.c msg.h util.o
thread_pool.o : thread_pool.c thread_pool.h
util.o        : util.c util.h
util_interC.o : util_interC.c util.o
pathv.o       : pathv.c pathv.h util.o

