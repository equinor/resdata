MFLAG = m32
CC = gcc
AR = ar
#################################################################
COMPILE_INCLUDE   = -I./ -I../../libhash/include -I../../libutil/include -I../../libecl/include 
COMPILE_LIB_PATH  = -L../../libhash/lib -L../../libutil/lib  -L../../libecl/lib
COMPILE_LIBS      = -lm -lhash -lutil -lecl

INSTALL_INC_PATH = ../include
INSTALL_LIB_PATH = ../lib
#################################################################
CFLAGS  = -O2 -$(MFLAG) -std=gnu99 -g -Wall $(COMPILE_INCLUDE) 
FFLAGS  = -fpe0 -traceback -warn unused -CB -u -g -real_size 64 -assume byterecl -assume nobuffered_io -convert big_endian
ARFLAGS = -csr
#################################################################
OBJECTS   = history.o rate_node.o date_node.o sched_interC.o sched_file.o sched_util.o sched_kw.o sched_kw_compdat.o sched_kw_wconhist.o sched_kw_dates.o sched_kw_untyped.o sched_kw_tstep.o
INC_FILES = date_node.h rate_node.h history.h sched_file.h sched_util.h sched_kw.h sched_kw_compdat.h sched_kw_wconhist.h sched_kw_dates.h sched_kw_untyped.h sched_kw_tstep.h
LIB       = libsched_$(MFLAG).a
LIB_LINK  = -lsched_$(MFLAG) -lecl_$(MFLAG) -lhash_$(MFLAG) -lutil_$(MFLAG) -lm -lz
LIB_PATH  = -L../../libhash/lib -L../../libutil/lib -L../../libecl/lib -L$(INSTALL_LIB_PATH)

INSTALL: $(LIB)
	install -d $(INSTALL_INC_PATH)
	install -d $(INSTALL_LIB_PATH)
	install $(INC_FILES) $(INSTALL_INC_PATH)
	install $(LIB) $(INSTALL_LIB_PATH)
	gcc $(CFLAGS) sched_test.c -o sched.x $(LIB_PATH) $(LIB_LINK)
	gcc $(CFLAGS) ecl_rft.c    -o rft.x $(LIB_PATH) $(LIB_LINK)

clean:
	rm -f *.o *~ 
	rm -f libsched_$(MFLAG).a
	rm -f $(INSTALL_LIB_PATH)/libsched_$(MFLAG).a
	rm -f $(INSTALL_INC_PATH)/*.h



$(LIB): $(OBJECTS)
	$(AR) $(ARFLAGS) $(LIB) $(OBJECTS)

%.o : %.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

%.o : %.f90
	$(FC) -c $(FFLAGS) $< -o $@


history.o           : history.c history.h rate_node.o date_node.o sched_file.o
rate_node.o         : rate_node.c rate_node.h
sched_util.o        : sched_util.c sched_util.h 
sched_kw_wconhist.o : sched_kw_wconhist.h sched_kw_wconhist.c sched_util.o rate_node.o
sched_kw_compdat.o  : sched_kw_compdat.h sched_kw_compdat.c sched_util.o
sched_kw_dates.o    : sched_kw_dates.h sched_kw_dates.c sched_util.o date_node.o
sched_kw_untyped.o  : sched_kw_untyped.h sched_kw_untyped.c sched_util.o
sched_kw_tstep.o    : sched_kw_tstep.h sched_kw_tstep.c sched_util.o
sched_kw.o          : sched_kw.c sched_kw.h sched_kw_wconhist.o sched_kw_untyped.o sched_kw_compdat.o sched_kw_dates.o sched_util.o sched_kw_tstep.o
sched_file.o        : sched_file.c sched_file.h sched_util.o sched_kw.o sched_kw_compdat.o 
sched_interC.o      : sched_interC.c sched_file.o
date_node.o         : date_node.c date_node.h

