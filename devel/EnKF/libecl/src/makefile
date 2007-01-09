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
FFLAGS  = -fpe0 -traceback -warn unused -CB -u -g -real_size 64 -assume byterecl -assume nobuffered_io -convert big_endian
ARFLAGS = -csr
#################################################################
OBJECTS   = ecl_kw.o ecl_block.o ecl_fstate.o ecl_sum.o ecl_interC.o ecl_interF.o fortio.o
MOD_FILES = ecl_inter.mod
INC_FILES = ecl_kw.h ecl_block.h ecl_fstate.h ecl_sum.h fortio.h
LIB       = libecl.a



INSTALL: $(LIB)
	install -d $(INSTALL_INC_PATH)
	install -d $(INSTALL_LIB_PATH)
	install -d $(INSTALL_MOD_PATH)
	install $(MOD_FILES) $(INSTALL_MOD_PATH)
	install $(INC_FILES) $(INSTALL_INC_PATH)
	install $(LIB) $(INSTALL_LIB_PATH)


$(LIB): $(OBJECTS)
	$(AR) $(ARFLAGS) $(LIB) $(OBJECTS)

%.o : %.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

%.o : %.f90
	$(FC) -c $(FFLAGS) $< -o $@


fortio.o    : fortio.c fortio.h
ecl_kw.o    : ecl_kw.c ecl_kw.h fortio.o
ecl_block.o : ecl_block.c ecl_block.h ecl_kw.o
ecl_fstate.o: ecl_fstate.c ecl_fstate.h ecl_block.o
ecl_sum.o   : ecl_sum.c ecl_sum.h ecl_fstate.o
ecl_interC.o: ecl_interC.c ecl_fstate.o
ecl_interF.o: ecl_interF.f90 ecl_interC.o

