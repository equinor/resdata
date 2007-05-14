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
ARFLAGS = -csr
#################################################################
OBJECTS           = rms_interC.o rms_file.o rms_util.o rms_tag.o rms_type.o rms_tagkey.o rms_stats.o
INC_FILES_INSTALL = rms_file.h rms_util.h rms_tag.h rms_type.h rms_tagkey.h rms_stats.h
LIB               = librms.a



INSTALL: $(LIB)
	install -d $(INSTALL_INC_PATH)
	install -d $(INSTALL_LIB_PATH)
	install -d $(INSTALL_MOD_PATH)
	install $(INC_FILES_INSTALL) $(INSTALL_INC_PATH)
	install $(LIB) $(INSTALL_LIB_PATH)
	gcc -Wall -g -m32 -I./ -I../../libhash/include -I../../libutil/include rms_test.c -o rms.x -L./ -L../../libutil/lib -L../../libhash/lib -lrms -lutil -util -lhash -lm


$(LIB): $(OBJECTS)
	$(AR) $(ARFLAGS) $(LIB) $(OBJECTS)

%.o : %.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@



rms_stats.o  : rms_stats.c rms_stats.h
rms_type.o   : rms_type.c rms_type.h
rms_util.o   : rms_util.c rms_util.h
rms_tagkey.o : rms_tagkey.c rms_tagkey.h rms_type.o rms_util.o
rms_tag.o    : rms_tag.c rms_tag.h rms_tagkey.o rms_util.o
rms_file.o   : rms_file.c rms_file.h rms_util.o rms_tag.o rms_tagkey.o
rms_interC.o : rms_interC.c rms_file.o

