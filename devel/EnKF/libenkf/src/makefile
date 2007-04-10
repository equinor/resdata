CC = gcc
FC = ifort
AR = ar
#################################################################
COMPILE_INCLUDE   = -I./ -I../../libutil/include
COMPILE_LIB_PATH  = -L../../libhash/lib
COMPILE_LIBS      = -lm -lhash

INSTALL_INC_PATH = ../include
INSTALL_LIB_PATH = ../lib
INSTALL_MOD_PATH = ../mod
#################################################################
CFLAGS  = -m32 -std=gnu99 -g -Wall $(COMPILE_INCLUDE) 
ARFLAGS = -csr
#################################################################
OBJECTS           = multflt_config.o ecl_config.o mem_config.o ens_config.o multz.o multz_config.o enkf_node.o
INC_FILES_INSTALL = multflt_config.h ecl_config.h mem_config.h ens_config.h multz.h multz_config.h enkf_node.h
LIB               = libenkf.a



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



ecl_config.o   : ecl_config.c ecl_config.h
multz_config.o : multz_config.c multz_config.h
multz.o        : multz.c multz.h multz_config.o
enkf_node.o    : enkf_node.h enkf_node.c




