include path_config
MFLAG = m32
CC = gcc
AR = ar
#################################################################
COMPILE_INCLUDE  = -I$(LIBECL_HOME)/src -I$(LIBUTIL_HOME)/include -I$(LIBHASH_HOME)/include 
INSTALL_INC_PATH =  $(LIBECL_HOME)/include
INSTALL_LIB_PATH =  $(LIBECL_HOME)/lib
#################################################################
CFLAGS  = -O2 -$(MFLAG)  -std=gnu99 -g -Wall -fPIC
ARFLAGS = -csr
#################################################################
OBJECTS   = ecl_well_vars.o ecl_util.o ecl_kw.o ecl_block.o ecl_fstate.o ecl_sum.o ecl_interC.o fortio.o lsf_jobs.o ecl_parse.o ecl_diag.o ecl_rft_vector.o ecl_rft_node.o ecl_grid.o ecl_box.o restart_kw_list.o 
MOD_FILES = 
INC_FILES = ecl_well_vars.h ecl_util.h ecl_kw.h ecl_block.h ecl_fstate.h ecl_sum.h fortio.h lsf_jobs.h ecl_parse.h ecl_diag.h ecl_rft_vector.h ecl_rft_node.h ecl_box.h ecl_grid.h restart_kw_list.h 
LIB       = libecl_$(MFLAG).a
LIB_PATH  = -L$(LIBUTIL_HOME)/lib -L$(LIBHASH_HOME)/lib -L$(LIBECL_HOME)/lib
LIB_LINK  = -lecl_$(MFLAG) -lhash_$(MFLAG) -lutil_$(MFLAG) -lm -lz
TARGET_INCLUDE  = -I$(LIBECL_HOME)/include -I$(LIBUTIL_HOME)/include -I$(LIBHASH_HOME)/include

INSTALL: $(LIB)
	install -d $(INSTALL_INC_PATH)
	install -d $(INSTALL_LIB_PATH)
	install $(INC_FILES) $(INSTALL_INC_PATH)
	install $(LIB) $(INSTALL_LIB_PATH)
#gcc      -Wall -g $(COMPILE_INCLUDE) ecl_submit.c -o /d/proj/bg/enkf/bin/ecl_submit.x 
	gcc $(CFLAGS) $(TARGET_INCLUDE) ecl_test.c         -o ecl_test.x          $(LIB_PATH)  $(LIB_LINK)
	gcc $(CFLAGS) $(TARGET_INCLUDE) summary_get_last.c -o summary_get_last.x  $(LIB_PATH)  $(LIB_LINK)
	gcc $(CFLAGS) $(TARGET_INCLUDE) make_SGU.c         -o make_SGU.x  	  $(LIB_PATH)  $(LIB_LINK)
	gcc $(CFLAGS) $(TARGET_INCLUDE) convert.c          -o convert.x   	  $(LIB_PATH)  $(LIB_LINK)
	gcc $(CFLAGS) $(TARGET_INCLUDE) kw_list.c          -o kw_list.x 	  $(LIB_PATH)  $(LIB_LINK)
	gcc $(CFLAGS) $(TARGET_INCLUDE) kw_cmp.c           -o kw_cmp.x  	  $(LIB_PATH)  $(LIB_LINK)
	gcc $(CFLAGS) $(TARGET_INCLUDE) combine_grdecl.c   -o combine_grdecl.x    $(LIB_PATH)  $(LIB_LINK)
	gcc $(CFLAGS) $(TARGET_INCLUDE) kw_extract.c       -o kw_extract.x        $(LIB_PATH)  $(LIB_LINK)
	gcc $(CFLAGS) $(TARGET_INCLUDE) grid_info.c        -o grid_info.x         $(LIB_PATH)  $(LIB_LINK)

###     This only links to 64 bit libraries.
#	gcc -O2 -m64 -std=gnu99 -g -Wall  -I$(INSTALL_INC_PATH) $(COMPILE_INCLUDE) ecl_rft.c  -o ecl_rft.x  $(LIB_PATH)  -lecl_m64 -lutil_m64 -lhash_m64 -lreadline -lncurses -lm


clean:
	rm -f *.o *~ 
	rm -f libecl_*
	rm -f $(INSTALL_INC_PATH)/*.h
	rm -f $(INSTALL_LIB_PATH)/libecl_$(MFLAG).a


$(LIB): $(OBJECTS)
	$(AR) $(ARFLAGS) $(LIB) $(OBJECTS)

%.o : %.c
	$(CC) -c $(CFLAGS) $(COMPILE_INCLUDE) $(CPPFLAGS) $< -o $@


ecl_rft_vector.o   : ecl_rft_vector.h ecl_rft_vector.c ecl_util.h ecl_kw.o ecl_fstate.o ecl_block.o ecl_rft_node.o
ecl_util.o   : ecl_util.c ecl_util.h
lsf_jobs.o   : lsf_jobs.c lsf_jobs.h 
fortio.o     : fortio.c fortio.h 
ecl_kw.o     : ecl_kw.c ecl_kw.h fortio.o
ecl_block.o  : ecl_block.c ecl_block.h ecl_kw.o
ecl_fstate.o : ecl_fstate.c ecl_fstate.h ecl_block.o ecl_util.o ecl_kw.o
ecl_sum.o    : ecl_sum.c ecl_sum.h ecl_fstate.o ecl_block.o ecl_kw.o ecl_util.o
ecl_parse.o  : ecl_parse.h ecl_parse.h ecl_fstate.o ecl_util.o ecl_kw.o ecl_block.o
ecl_diag.o   : ecl_diag.c ecl_diag.h ecl_sum.o ecl_fstate.o ecl_util.o
ecl_interC.o : ecl_interC.c ecl_fstate.o ecl_sum.o ecl_parse.o lsf_jobs.o
ecl_grid.o   : ecl_grid.c ecl_grid.h ecl_kw.o fortio.o
ecl_interC.o : ecl_sum.o ecl_kw.o ecl_fstate.o ecl_block.o
ecl_box.o    : ecl_box.c ecl_box.h
