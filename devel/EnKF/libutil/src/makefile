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
OBJECTS           = util.o util_interC.o
INC_FILES_INSTALL = util.h
LIB               = libutil.a



INSTALL: $(LIB)
	install -d $(INSTALL_INC_PATH)
	install -d $(INSTALL_LIB_PATH)
	install -d $(INSTALL_MOD_PATH)
	install $(INC_FILES_INSTALL) $(INSTALL_INC_PATH)
	install $(LIB) $(INSTALL_LIB_PATH)


$(LIB): $(OBJECTS)
	$(AR) $(ARFLAGS) $(LIB) $(OBJECTS)

%.o : %.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@


util.o        : util.c util.h
util_interC.o : util_interC.c util.o

