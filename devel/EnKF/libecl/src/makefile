CC = gcc
FC = ifort
AR = ar
#################################################################
COMPILE_INCLUDE   = -I./ -I../../libhash/include -I../../libutil/include
COMPILE_LIB_PATH  = -L../../libhash/lib -L../../libutil/lib
COMPILE_LIBS      = -lm -lhash -lutil

INSTALL_INC_PATH = ../include
INSTALL_LIB_PATH = ../lib
INSTALL_MOD_PATH = ../mod
#################################################################
CFLAGS  = -O2 -m32 -std=gnu99 -g -Wall $(COMPILE_INCLUDE) 
FFLAGS  = -fpe0 -traceback -warn unused -CB -u -g -real_size 64 -assume byterecl -assume nobuffered_io -convert big_endian
ARFLAGS = -csr
#################################################################
OBJECTS   = ecl_well_vars.o ecl_util.o ecl_kw.o ecl_block.o ecl_fstate.o ecl_sum.o ecl_interC.o fortio.o lsf_jobs.o ecl_parse.o ecl_diag.o 
MOD_FILES = 
INC_FILES = ecl_well_vars.h ecl_util.h ecl_kw.h ecl_block.h ecl_fstate.h ecl_sum.h fortio.h lsf_jobs.h ecl_parse.h ecl_diag.h 
LIB       = libecl.a



INSTALL: $(LIB)
	install -d $(INSTALL_INC_PATH)
	install -d $(INSTALL_LIB_PATH)
	install -d $(INSTALL_MOD_PATH)
	install $(INC_FILES) $(INSTALL_INC_PATH)
	install $(LIB) $(INSTALL_LIB_PATH)
	gcc -m32 -Wall ecl_submit.c -o /d/proj/bg/enkf/bin/ecl_submit.x
	gcc -m32 -Wall -I$(COMPILE_INCLUDE) -I$(INSTALL_INC_PATH) ecl_test.c -o ecl_test.x -L$(INSTALL_LIB_PATH) $(COMPILE_LIB_PATH) -lecl $(COMPILE_LIBS)
	gcc -m32 -Wall -I$(COMPILE_INCLUDE) -I$(INSTALL_INC_PATH) make_SGU.c -o make_SGU.x -L$(INSTALL_LIB_PATH) $(COMPILE_LIB_PATH) -lecl $(COMPILE_LIBS)
	gcc -Wall -g -m32 -O2 $(COMPILE_INCLUDE) -I$(INSTALL_INC_PATH) -o convert.x convert.c -I./ -L./ -L../../libutil/lib -lecl -lm -lutil


clean:
	rm -f *.o *~ 

new:	
	make clean
	make INSTALL

$(LIB): $(OBJECTS)
	$(AR) $(ARFLAGS) $(LIB) $(OBJECTS)

%.o : %.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

%.o : %.f90
	$(FC) -c $(FFLAGS) $< -o $@


ecl_util.o  : ecl_util.c ecl_util.h
lsf_jobs.o  : lsf_jobs.c lsf_jobs.h 
fortio.o    : fortio.c fortio.h 
ecl_kw.o    : ecl_kw.c ecl_kw.h fortio.o
ecl_block.o : ecl_block.c ecl_block.h ecl_kw.o
ecl_fstate.o: ecl_fstate.c ecl_fstate.h ecl_block.o ecl_util.o ecl_kw.o
ecl_sum.o   : ecl_sum.c ecl_sum.h ecl_fstate.o ecl_block.o ecl_kw.o ecl_util.o
ecl_parse.o : ecl_parse.h ecl_parse.h o ecl_fstate.o ecl_util.o ecl_kw.o ecl_block.o
ecl_diag.o  : ecl_diag.c ecl_diag.h ecl_sum.o ecl_fstate.o ecl_util.o
ecl_interC.o: ecl_interC.c ecl_fstate.o ecl_sum.o ecl_parse.o lsf_jobs.o
#ecl_interF.o: ecl_interF.f90 ecl_interC.o

