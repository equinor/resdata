CC = gcc
FC = ifort
AR = ar
#################################################################
COMPILE_INCLUDE   = -I./ -I../../libhash/include -I../../libutil/include
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
OBJECTS   = sched.o ecl_kw.o ecl_block.o ecl_fstate.o ecl_sum.o ecl_interC.o fortio.o ext_job.o lsf_jobs.o ecl_parse.o ecl_diag.o str_buffer.o
MOD_FILES = 
INC_FILES = sched.h ecl_kw.h ecl_block.h ecl_fstate.h ecl_sum.h fortio.h ext_job.h lsf_jobs.h ecl_parse.h ecl_diag.h str_buffer.h
LIB       = libecl.a



INSTALL: $(LIB)
	install -d $(INSTALL_INC_PATH)
	install -d $(INSTALL_LIB_PATH)
	install -d $(INSTALL_MOD_PATH)
#	install $(MOD_FILES) $(INSTALL_MOD_PATH)
	install $(INC_FILES) $(INSTALL_INC_PATH)
	install $(LIB) $(INSTALL_LIB_PATH)
	gcc -m32 -Wall ecl_submit.c -o /d/proj/bg/enkf/bin/ecl_submit.x


$(LIB): $(OBJECTS)
	$(AR) $(ARFLAGS) $(LIB) $(OBJECTS)

%.o : %.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

%.o : %.f90
	$(FC) -c $(FFLAGS) $< -o $@


str_buffer.o: str_buffer.c str_buffer.h
ext_job.o   : ext_job.c ext_job.h 
lsf_jobs.o  : lsf_jobs.c lsf_jobs.h ext_job.h
fortio.o    : fortio.c fortio.h 
ecl_kw.o    : ecl_kw.c ecl_kw.h fortio.o
ecl_block.o : ecl_block.c ecl_block.h ecl_kw.o
ecl_fstate.o: ecl_fstate.c ecl_fstate.h ecl_block.o 
ecl_sum.o   : ecl_sum.c ecl_sum.h ecl_fstate.o
ecl_parse.o : ecl_parse.h ecl_parse.h str_buffer.o
ecl_diag.o  : ecl_diag.c ecl_diag.h ecl_sum.o
ecl_interC.o: ecl_interC.c ecl_fstate.o ext_job.o ecl_sum.o ecl_parse.o lsf_jobs.o
#ecl_interF.o: ecl_interF.f90 ecl_interC.o

