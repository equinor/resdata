CC = gcc
FC = ifort
AR = ar
#################################################################
COMPILE_INCLUDE   = -I./ -I../../libhash/include
COMPILE_LIB_PATH  = -L../../libhash/lib
COMPILE_LIBS      = -lm -lhash

INSTALL_INC_PATH = ../include
INSTALL_LIB_PATH = ../lib
INSTALL_MOD_PATH = ../mod
#################################################################
CFLAGS  = -O2 -m32 -std=gnu99 -g -Wall $(COMPILE_INCLUDE) 
ARFLAGS = -csr
#################################################################
OBJECTS   = rms_roff.o rms_interC.o
MOD_FILES = 
INC_FILES = rms_roff.h
LIB       = librms.a



INSTALL: $(LIB)
	install -d $(INSTALL_INC_PATH)
	install -d $(INSTALL_LIB_PATH)
	install -d $(INSTALL_MOD_PATH)
	install $(INC_FILES) $(INSTALL_INC_PATH)
	install $(LIB) $(INSTALL_LIB_PATH)


$(LIB): $(OBJECTS)
	$(AR) $(ARFLAGS) $(LIB) $(OBJECTS)

%.o : %.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@



rms_roff.o   : rms_roff.c rms_roff.h
rms_interC.o : rms_interC.c rms_roff.o
